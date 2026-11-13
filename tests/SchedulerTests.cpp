#include <map>

#include "TestFramework.h"
#include "core/PatternGenerator.h"
#include "dsp/NoteScheduler.h"

using namespace gfb;

namespace
{
struct AbsEvent { int64_t sample; SynthEvent ev; };

struct Sim
{
    double sr = 48000, bpm = 124;
    NoteScheduler sch;
    NoteSchedule schedule;
    std::vector<AbsEvent> events;
    int64_t samplePos = 0;
    double ppq = 0;

    explicit Sim (const Pattern& p, double sampleRate = 48000, double tempo = 124) : sr (sampleRate), bpm (tempo)
    {
        sch.prepare (sr);
        schedule.fromPattern (p, 1);
    }
    void block (int n, bool playing = true)
    {
        SynthEvent out[NoteScheduler::kMaxEvents];
        TransportState t { playing, ppq, bpm };
        const int k = sch.process (schedule, t, n, out, NoteScheduler::kMaxEvents);
        for (int i = 0; i < k; ++i)
        {
            CHECK (out[i].sampleOffset >= 0 && out[i].sampleOffset < n);
            if (i > 0) CHECK (out[i].sampleOffset >= out[i - 1].sampleOffset);
            events.push_back ({ samplePos + out[i].sampleOffset, out[i] });
        }
        samplePos += n;
        if (playing) ppq += n * bpm / 60.0 / sr;
    }
    int count (SynthEvent::Type t) const
    {
        int c = 0;
        for (auto& e : events) c += e.ev.type == t ? 1 : 0;
        return c;
    }
    /** checks every note-on has a matching note-off later (no stuck notes after stop) */
    bool balanced() const
    {
        std::map<int, int> open;
        for (auto& e : events)
        {
            if (e.ev.type == SynthEvent::Type::NoteOn) open[e.ev.note]++;
            else if (e.ev.type == SynthEvent::Type::NoteOff) { if (open[e.ev.note] <= 0) return false; open[e.ev.note]--; }
        }
        for (auto& [k, v] : open) if (v != 0) return false;
        return true;
    }
};

Pattern testPattern (int bars = 2, int feel = 4)
{
    GeneratorSettings s;
    s.bars = bars; s.feel = (Feel) feel; s.family = Family::Acid; s.styleSeed = 4242; s.varSeed = 7;
    return generatePattern (s);
}
} // namespace

TEST_CASE (Scheduler, NoteOnsAtExpectedSamplesAcrossBlockSizes)
{
    const auto p = testPattern();
    for (int bs : { 32, 64, 100, 128, 256, 333, 512, 1024 })
    {
        Sim sim (p);
        const int64_t loops = 3;
        const double samplesPerTick = 48000.0 * 60.0 / (124.0 * kPPQ);
        const int64_t total = (int64_t) (loops * p.lengthTicks() * samplesPerTick) + 10;
        while (sim.samplePos < total) sim.block (bs);
        std::vector<int64_t> ons;
        for (auto& e : sim.events) if (e.ev.type == SynthEvent::Type::NoteOn) ons.push_back (e.sample);
        REQUIRE ((int64_t) ons.size() == loops * (int64_t) p.notes.size());
        for (size_t i = 0; i < ons.size(); ++i)
        {
            const auto& n = p.notes[i % p.notes.size()];
            const double expected = ((double) (i / p.notes.size()) * (double) p.lengthTicks() + (double) n.startTick) * samplesPerTick;
            CHECK_MSG (std::abs ((double) ons[i] - expected) <= 1.0, "bs=" + std::to_string (bs) + " i=" + std::to_string (i) + " got=" + std::to_string (ons[i]) + " exp=" + std::to_string (expected));
        }
    }
}

TEST_CASE (Scheduler, BoundaryEventFiresExactlyOnce)
{
    Pattern p;
    p.bars = 1;
    p.notes = { { 0, 240, 40, 100, false, false }, { 960, 240, 43, 100, false, false } };
    // 120 BPM @ 48k: 1 tick = 25 samples; beat = 24000 samples. Blocks of 2400 align exactly with ticks of note starts.
    Sim sim (p, 48000, 120);
    for (int i = 0; i < 80; ++i) sim.block (2400);
    CHECK_EQ (sim.count (SynthEvent::Type::NoteOn), 2 * 2); // 80*2400 = 192000 samples = 2 bars => 2 loops
}

TEST_CASE (Scheduler, StopAndSeekLeaveNoStuckNotes)
{
    const auto p = testPattern (4, 2);
    Sim sim (p);
    for (int i = 0; i < 300; ++i) sim.block (256);
    sim.ppq = 37.3; // seek
    for (int i = 0; i < 100; ++i) sim.block (256);
    sim.ppq = 2.0;  // loop back
    for (int i = 0; i < 100; ++i) sim.block (256);
    sim.block (256, false); // stop
    CHECK (sim.balanced());
    CHECK_EQ (sim.sch.activeNotes(), 0);
}

TEST_CASE (Scheduler, PatternSwapFlushesAndContinues)
{
    const auto a = testPattern (2, 2), b = testPattern (4, 3);
    Sim sim (a);
    for (int i = 0; i < 77; ++i) sim.block (128);
    sim.schedule.fromPattern (b, 2);
    for (int i = 0; i < 400; ++i) sim.block (128);
    sim.block (128, false);
    CHECK (sim.balanced());
}

TEST_CASE (Scheduler, TempoChangesFollowHostPosition)
{
    const auto p = testPattern (1, 2);
    Sim sim (p, 44100, 100);
    for (int i = 0; i < 200; ++i) { sim.bpm = 100 + (i % 50); sim.block (512); }
    sim.block (512, false);
    CHECK (sim.balanced());
    // each note-on happens when host ppq crosses the note start: verify count matches crossed loops
    const double loopsCovered = sim.ppq * kPPQ / (double) p.lengthTicks();
    int expected = 0;
    for (int64_t L = 0; L < (int64_t) loopsCovered + 1; ++L)
        for (auto& n : p.notes) expected += ((double) (L * p.lengthTicks() + n.startTick) < sim.ppq * kPPQ) ? 1 : 0;
    CHECK (std::abs (sim.count (SynthEvent::Type::NoteOn) - expected) <= 1);
}

TEST_CASE (Scheduler, LongRunNoDrift)
{
    const auto p = testPattern (8, 3);
    Sim sim (p, 48000, 128);
    const int64_t tenMinutes = 48000LL * 600;
    while (sim.samplePos < tenMinutes) sim.block (128);
    const double samplesPerTick = 48000.0 * 60.0 / (128.0 * kPPQ);
    int64_t lastOn = -1; size_t idx = 0;
    for (auto& e : sim.events) if (e.ev.type == SynthEvent::Type::NoteOn) { lastOn = e.sample; ++idx; }
    REQUIRE (idx > 0);
    const size_t i = idx - 1;
    const double expected = ((double) (i / p.notes.size()) * (double) p.lengthTicks() + (double) p.notes[i % p.notes.size()].startTick) * samplesPerTick;
    CHECK (std::abs ((double) lastOn - expected) <= 1.0);
}

TEST_CASE (Scheduler, LegatoFlagFollowsSlide)
{
    Pattern p;
    p.bars = 1;
    p.notes = { { 0, 270, 40, 110, true, true }, { 240, 200, 43, 100, false, false } };
    Sim sim (p);
    for (int i = 0; i < 40; ++i) sim.block (256);
    bool sawLegato = false;
    for (auto& e : sim.events) if (e.ev.type == SynthEvent::Type::NoteOn && e.ev.note == 43) sawLegato = e.ev.legato;
    CHECK (sawLegato);
    // overlap: the note-on of 43 arrives before the note-off of 40
    int64_t on43 = -1, off40 = -1;
    for (auto& e : sim.events)
    {
        if (on43 < 0 && e.ev.type == SynthEvent::Type::NoteOn && e.ev.note == 43) on43 = e.sample;
        if (off40 < 0 && e.ev.type == SynthEvent::Type::NoteOff && e.ev.note == 40) off40 = e.sample;
    }
    CHECK (on43 >= 0 && off40 > on43);
}

TEST_CASE (Scheduler, StoppedTransportIsSilent)
{
    Sim sim (testPattern());
    for (int i = 0; i < 100; ++i) sim.block (256, false);
    CHECK (sim.events.empty());
}

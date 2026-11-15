#include "core/PhraseComposer.h"

#include <algorithm>

#include "core/ScaleTheory.h"

namespace gfb::phrase
{
namespace
{
// Templates and weights. Since algorithmVersion 3 the repetitive forms found in
// the user's dataset (A B A B, A A A A', A A' A A') carry the most weight.
struct Template { std::vector<std::string> labels; int weight; };

const std::vector<Template>& templatesFor (int bars)
{
    static const std::vector<Template> t1 { { { "A*" }, 1 } };
    static const std::vector<Template> t2 { { { "A", "A'" }, 3 }, { { "A", "B" }, 3 } };
    static const std::vector<Template> t4 {
        { { "A", "A'", "B", "F" }, 2 }, { { "A", "B", "A'", "F" }, 1 }, { { "A", "A'", "A", "F" }, 2 },
        { { "A", "B", "A", "B" }, 4 }, { { "A", "A", "A", "A'" }, 3 }, { { "A", "A'", "A", "A'" }, 3 } };
    static const std::vector<Template> t8 {
        { { "A", "A'", "B", "F", "A", "A''", "B'", "F2" }, 1 },
        { { "A", "A'", "A", "B", "A", "A'", "B'", "F2" }, 1 },
        { { "A", "B", "A'", "F", "A", "B'", "A''", "F2" }, 1 },
        { { "A", "B", "A", "B", "A", "B", "A", "B'" }, 4 },
        { { "A", "A", "A", "A'", "A", "A", "A", "F" }, 3 },
        { { "A", "A'", "A", "A'", "A", "A'", "A", "F2" }, 3 } };
    switch (bars)
    {
        case 1: return t1;
        case 2: return t2;
        case 4: return t4;
        default: return t8;
    }
}
} // namespace

int templateCount (int bars) noexcept { return (int) templatesFor (bars).size(); }

std::vector<std::string> phraseTemplate (int bars, int index)
{
    const auto& t = templatesFor (bars);
    return t[(size_t) (((index % (int) t.size()) + (int) t.size()) % (int) t.size())].labels;
}

int pickTemplateIndex (int bars, Rng& rng)
{
    const auto& t = templatesFor (bars);
    std::vector<int> w;
    for (const auto& x : t) w.push_back (x.weight);
    return rng.pickWeighted (w.data(), (int) w.size());
}

std::vector<int> carryPitches (const std::vector<NoteSlot>& oldSlots, const std::vector<int>& oldPitches,
                               const std::vector<NoteSlot>& newSlots, const MotifVocabulary& vocab, Rng& rng,
                               const PitchContext& ctx)
{
    auto fresh = pitch::assignPitches (newSlots, vocab, rng, ctx, BarFunction::Variation, oldPitches.empty() ? 0 : oldPitches.front());
    for (size_t i = 0; i < newSlots.size(); ++i)
        for (size_t j = 0; j < oldSlots.size(); ++j)
            if (oldSlots[j].step == newSlots[i].step && newSlots[i].role != Role::Ghost)
                fresh[i] = oldPitches[j];
    for (size_t i = 1; i < newSlots.size(); ++i)
        if (newSlots[i].role == Role::Ghost) fresh[i] = fresh[i - 1];
    return fresh;
}

int diatonicStepsFromRoot (int semis, ScaleType s) noexcept
{
    const int pc = ((semis % 12) + 12) % 12;
    const int deg = theory::degreeOfInterval (s, pc);
    if (deg < 0) return 0;
    const int oct = (semis - pc) / 12;
    return oct * theory::degreeCount (s) + deg;
}

Pattern renderBars (const std::vector<BarRealisation>& bars, const ArticulationParams& ap)
{
    Pattern p;
    p.bars = (int) bars.size();
    const auto& fp = feelProfile (ap.feel);
    const int swingOffset = std::max (0, (ap.swingPercent - 50) * 2 * kTicksPerSixteenth / 100);
    const int rootMidi = theory::referenceRootMidi (ap.root, ap.family) + 12 * ap.octave;

    struct Tmp { int64_t start; int lenSteps; uint8_t accent; Role role; bool slide; int midi; int absStep; };
    std::vector<Tmp> t;
    for (size_t b = 0; b < bars.size(); ++b)
    {
        const auto& bar = bars[b];
        for (size_t i = 0; i < bar.slots.size(); ++i)
        {
            const auto& s = bar.slots[i];
            int64_t start = (int64_t) b * kTicksPerBar + (int64_t) s.step * kTicksPerSixteenth;
            if (s.step % 2 == 1) start += swingOffset; // swing only the off-16ths
            t.push_back ({ start, s.length, s.accent, s.role, s.slide, rootMidi + bar.pitches[i], (int) b * kStepsPerBar + s.step });
        }
    }

    // octave folding of the whole line so every note is a valid MIDI note
    if (! t.empty())
    {
        int lo = 1000, hi = -1000;
        for (auto& x : t) { lo = std::min (lo, x.midi); hi = std::max (hi, x.midi); }
        int shift = 0;
        while (lo + shift < 0) shift += 12;
        while (hi + shift > 127) shift -= 12;
        for (auto& x : t) x.midi = std::clamp (x.midi + shift, 0, 127);
    }

    const int64_t total = p.lengthTicks();
    for (size_t i = 0; i < t.size(); ++i)
    {
        const auto& x = t[i];
        const int64_t nextStart = i + 1 < t.size() ? t[i + 1].start : total;
        int gate = x.lenSteps == 1 ? fp.gateShort : fp.gateLong;
        if (ap.family == Family::Sub) gate = std::min (990, gate + 30);
        else if (x.lenSteps == 1) gate = std::max (250, gate - 40);
        int64_t dur = (int64_t) x.lenSteps * kTicksPerSixteenth * gate / 1000;
        if (x.role == Role::Ghost) dur = std::min<int64_t> (dur, kTicksPerSixteenth * 3 / 4);
        NoteEvent e;
        e.startTick = x.start;
        e.midiNote = (uint8_t) x.midi;
        e.accent = x.accent >= 2;
        const bool slideOk = x.slide && i + 1 < t.size() && t[i + 1].midi != x.midi && t[i + 1].absStep == x.absStep + x.lenSteps;
        e.slideToNext = slideOk;
        dur = std::min (dur, nextStart - x.start);
        e.durationTicks = std::max<int64_t> (std::min<int64_t> (kMinNoteDurationTicks, nextStart - x.start), dur);
        int vel;
        if (ap.family == Family::Sub) vel = x.accent >= 2 ? 112 : (x.accent == 0 ? 60 : 94);
        else vel = x.accent >= 2 ? 120 : (x.accent == 0 ? 58 : 86);
        if (x.role == Role::Anchor) vel += 4;
        e.velocity = (uint8_t) std::clamp (vel, 1, 127);
        p.notes.push_back (e);
    }
    // slide overlaps (after all starts are known)
    for (size_t i = 0; i + 1 < p.notes.size(); ++i)
    {
        auto& e = p.notes[i];
        if (! e.slideToNext) continue;
        const auto& nx = p.notes[i + 1];
        const int64_t overlap = std::min<int64_t> (kSlideOverlapTicks, nx.durationTicks - 1);
        e.durationTicks = nx.startTick - e.startTick + std::max<int64_t> (0, overlap);
    }
    if (! p.notes.empty()) p.notes.back().slideToNext = false;
    // the last note must not ring past the loop end
    if (! p.notes.empty())
    {
        auto& last = p.notes.back();
        last.durationTicks = std::max<int64_t> (1, std::min (last.durationTicks, total - last.startTick));
    }
    return p;
}
} // namespace gfb::phrase

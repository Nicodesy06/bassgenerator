#include "dsp/NoteScheduler.h"

#include <algorithm>
#include <cmath>

namespace gfb
{
void NoteSchedule::fromPattern (const Pattern& p, uint64_t newVersion) noexcept
{
    count = 0;
    lengthTicks = std::max<int64_t> (kTicksPerBar, p.lengthTicks());
    for (const auto& n : p.notes)
    {
        if (count >= (int) notes.size()) break;
        notes[(size_t) count++] = { n.startTick, n.endTick(), n.midiNote, n.velocity, n.accent, n.slideToNext };
    }
    version = newVersion;
}

void NoteScheduler::addActive (uint8_t n) noexcept
{
    for (int i = 0; i < activeCount; ++i)
        if (active[(size_t) i] == n) return;
    if (activeCount < kMaxActive) active[(size_t) activeCount++] = n;
}

bool NoteScheduler::removeActive (uint8_t n) noexcept
{
    for (int i = 0; i < activeCount; ++i)
        if (active[(size_t) i] == n)
        {
            active[(size_t) i] = active[(size_t) --activeCount];
            return true;
        }
    return false;
}

int NoteScheduler::flush (SynthEvent* out, int maxOut, int offset) noexcept
{
    int w = 0;
    while (activeCount > 0 && w < maxOut)
    {
        SynthEvent e;
        e.sampleOffset = offset;
        e.type = SynthEvent::Type::NoteOff;
        e.note = active[(size_t) --activeCount];
        out[w++] = e;
    }
    return w;
}

int NoteScheduler::process (const NoteSchedule& sched, const TransportState& t, int numSamples, SynthEvent* out, int maxOut) noexcept
{
    int w = 0;
    if (! t.playing || numSamples <= 0 || ! (t.bpm > 0.0) || ! std::isfinite (t.ppqPosition))
    {
        if (wasPlaying) w += flush (out + w, maxOut - w);
        wasPlaying = false;
        return w;
    }

    const double tps = std::clamp (t.bpm, 1.0, 1000.0) / 60.0 * kPPQ / sr; // ticks per sample
    double tick0 = t.ppqPosition * kPPQ;
    if (wasPlaying)
    {
        const double tol = std::max (1.0, tps * 2.0);
        if (std::abs (tick0 - lastEndTick) <= tol) tick0 = lastEndTick; // continuous: no gaps, no double fire
        else w += flush (out + w, maxOut - w);                         // seek / loop jump
    }
    if (sched.version != lastVersion)
    {
        if (wasPlaying) w += flush (out + w, maxOut - w); // pattern swapped while running
        lastVersion = sched.version;
    }
    const double tick1 = tick0 + numSamples * tps;
    const double L = (double) sched.lengthTicks;

    // Event inclusion is decided in the sample domain with a tiny epsilon so an
    // event landing exactly on a block boundary belongs to exactly one block.
    constexpr double eps = 1.0e-6;
    auto firstOccurrence = [&] (double t) {
        double k = std::ceil ((tick0 - t) / L - 1.0e-9);
        double at = t + k * L;
        if ((at - tick0) / tps < -eps) at += L;
        return at;
    };
    int np = 0;
    for (int i = 0; i < sched.count; ++i)
    {
        const auto& n = sched.notes[(size_t) i];
        const auto& prev = sched.notes[(size_t) ((i + sched.count - 1) % sched.count)];
        const bool legato = sched.count > 1 && prev.slide;
        for (double s = firstOccurrence ((double) n.start); (s - tick0) / tps < numSamples - eps && np < kMaxEvents; s += L)
        {
            SynthEvent e;
            e.type = SynthEvent::Type::NoteOn;
            e.note = n.note;
            e.velocity = n.velocity;
            e.accent = n.accent;
            e.legato = legato;
            pending[(size_t) np++] = { s, 1, e };
        }
        for (double o = firstOccurrence ((double) n.end); (o - tick0) / tps < numSamples - eps && np < kMaxEvents; o += L)
        {
            SynthEvent e;
            e.type = SynthEvent::Type::NoteOff;
            e.note = n.note;
            pending[(size_t) np++] = { o, 0, e };
        }
    }
    std::sort (pending.begin(), pending.begin() + np, [] (const Pending& a, const Pending& b) {
        if (a.tick != b.tick) return a.tick < b.tick;
        return a.order < b.order; // off before on at identical tick
    });
    for (int i = 0; i < np && w < maxOut; ++i)
    {
        auto e = pending[(size_t) i].ev;
        e.sampleOffset = std::clamp ((int) std::floor ((pending[(size_t) i].tick - tick0) / tps + eps), 0, numSamples - 1);
        if (e.type == SynthEvent::Type::NoteOff)
        {
            if (! removeActive (e.note)) continue; // never emit orphan note-offs
        }
        else
        {
            if (activeCount >= kMaxActive) { // defensive: should not happen for mono lines
                SynthEvent off; off.sampleOffset = e.sampleOffset; off.type = SynthEvent::Type::NoteOff; off.note = active[0];
                removeActive (active[0]);
                out[w++] = off;
                if (w >= maxOut) break;
            }
            addActive (e.note);
        }
        out[w++] = e;
    }
    lastEndTick = tick1;
    wasPlaying = true;
    return w;
}
} // namespace gfb

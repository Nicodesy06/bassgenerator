#include "core/PatternTypes.h"

#include <algorithm>

namespace gfb
{
namespace
{
struct Fnv
{
    uint64_t h = 1469598103934665603ULL;
    void byte (uint8_t b) noexcept { h ^= b; h *= 1099511628211ULL; }
    void u64 (uint64_t v) noexcept { for (int i = 0; i < 8; ++i) byte ((uint8_t) (v >> (8 * i))); }
};
} // namespace

bool isValidBars (int bars) noexcept { return bars == 1 || bars == 2 || bars == 4 || bars == 8; }

uint64_t computePatternHash (const Pattern& p)
{
    Fnv f;
    f.u64 (p.algorithmVersion);
    f.u64 (p.ticksPerQuarter);
    f.u64 ((uint64_t) p.bars);
    f.u64 ((uint64_t) p.notes.size());
    for (const auto& n : p.notes)
    {
        f.u64 ((uint64_t) n.startTick);
        f.u64 ((uint64_t) n.durationTicks);
        f.byte (n.midiNote);
        f.byte (n.velocity);
        f.byte ((uint8_t) ((n.accent ? 1 : 0) | (n.slideToNext ? 2 : 0)));
    }
    return f.h;
}

std::string validatePattern (const Pattern& p)
{
    if (! isValidBars (p.bars)) return "invalid bar count";
    if (p.ticksPerQuarter != (uint32_t) kPPQ) return "invalid PPQ";
    if (p.notes.size() > (size_t) kMaxNotesPerPattern) return "too many notes";
    const int64_t len = p.lengthTicks();
    for (size_t i = 0; i < p.notes.size(); ++i)
    {
        const auto& n = p.notes[i];
        const auto idx = std::to_string (i);
        if (n.startTick < 0) return "negative start at " + idx;
        if (n.durationTicks <= 0) return "non-positive duration at " + idx;
        if (n.endTick() > len) return "note exceeds pattern length at " + idx;
        if (n.midiNote > 127) return "midi note out of range at " + idx;
        if (n.velocity < 1 || n.velocity > 127) return "velocity out of range at " + idx;
        if (i + 1 < p.notes.size())
        {
            const auto& nx = p.notes[i + 1];
            if (nx.startTick <= n.startTick) return "notes not strictly sorted at " + idx;
            if (n.endTick() > nx.startTick)
            {
                if (! n.slideToNext) return "overlap without slide at " + idx;
                if (nx.midiNote == n.midiNote) return "slide overlap onto same pitch at " + idx;
                if (n.endTick() - nx.startTick > kMaxSlideOverlapTicks) return "slide overlap too long at " + idx;
                if (n.endTick() > nx.endTick()) return "slide note outlives next note at " + idx;
            }
        }
        else if (n.slideToNext)
        {
            return "last note has slideToNext";
        }
    }
    return {};
}

void normalisePattern (Pattern& p)
{
    if (! isValidBars (p.bars)) p.bars = 1;
    p.ticksPerQuarter = kPPQ;
    const int64_t len = p.lengthTicks();
    auto& v = p.notes;
    v.erase (std::remove_if (v.begin(), v.end(), [len] (const NoteEvent& n) { return n.startTick < 0 || n.startTick >= len; }), v.end());
    std::stable_sort (v.begin(), v.end(), [] (const NoteEvent& a, const NoteEvent& b) { return a.startTick < b.startTick; });
    // unique starts: keep the first (louder wins ties deterministically by order)
    v.erase (std::unique (v.begin(), v.end(), [] (const NoteEvent& a, const NoteEvent& b) { return a.startTick == b.startTick; }), v.end());
    if (v.size() > (size_t) kMaxNotesPerPattern) v.resize ((size_t) kMaxNotesPerPattern);
    for (size_t i = 0; i < v.size(); ++i)
    {
        auto& n = v[i];
        n.velocity = (uint8_t) std::clamp<int> (n.velocity, 1, 127);
        n.midiNote = (uint8_t) std::min<int> (n.midiNote, 127);
        int64_t limit = len;
        if (i + 1 < v.size())
        {
            const auto& nx = v[i + 1];
            if (n.slideToNext && nx.midiNote != n.midiNote)
                limit = std::min<int64_t> (nx.startTick + kMaxSlideOverlapTicks, nx.endTick());
            else
            {
                n.slideToNext = n.slideToNext && nx.midiNote != n.midiNote;
                limit = nx.startTick;
            }
        }
        else
        {
            n.slideToNext = false;
        }
        n.durationTicks = std::clamp<int64_t> (n.durationTicks, 1, std::max<int64_t> (1, limit - n.startTick));
        if (n.slideToNext && i + 1 < v.size() && n.endTick() <= v[i + 1].startTick)
            n.slideToNext = true; // slide flag kept (legato hint) even without overlap
    }
    p.hash = computePatternHash (p);
}
} // namespace gfb

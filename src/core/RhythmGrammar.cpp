#include "core/RhythmGrammar.h"

#include <algorithm>
#include <cassert>

namespace gfb
{
namespace
{
using M = Mutation;
//                                    Rem Add Spl Mrg Ant Del Acc Gho Oct Pck Sld Ext
// Gates and Extend weights since algorithmVersion 3 follow the user's dataset (legato: ~77 % sounding).
constexpr std::array<FeelProfile, kNumFeels> kProfiles {{
    { Feel::Random,     4,  9,  6, 900, 970, 54, 150, 300, 650, 300, 700, 10, 2, { 2, 2, 2, 2, 2, 1, 2, 2, 1, 1, 1, 3 } },
    { Feel::Sparse,     2,  6,  3, 920, 980, 52,  80, 250, 750, 300, 850, 13, 2, { 3, 1, 1, 2, 2, 2, 2, 1, 2, 2, 1, 3 } },
    { Feel::Balanced,   4,  9,  6, 900, 970, 54, 150, 250, 650, 300, 750, 10, 2, { 2, 2, 2, 2, 2, 1, 2, 2, 1, 2, 1, 4 } },
    { Feel::Busy,       8, 13, 10, 800, 930, 52, 250, 250, 650, 150, 600,  4, 3, { 2, 3, 3, 1, 1, 0, 3, 3, 1, 1, 2, 1 } },
    { Feel::Rolling,    6, 12,  9, 840, 950, 56, 250, 450, 850, 300, 750,  5, 2, { 1, 2, 2, 1, 1, 0, 2, 4, 1, 2, 1, 2 } },
    { Feel::Syncopated, 4, 10,  6, 920, 980, 54, 100, 500, 950, 150, 700,  9, 2, { 1, 1, 1, 2, 4, 2, 2, 1, 1, 2, 1, 4 } },
    { Feel::Stabby,     3,  9,  5, 400, 520, 52,  60, 250, 800, 150, 700, 10, 2, { 2, 2, 0, 0, 2, 2, 3, 1, 2, 1, 0, 0 } },
}};

bool isOnset (const Step& s) noexcept { return s.kind == StepKind::Onset; }

int noteEnd (const RhythmCell& c, int onsetStep) noexcept
{
    int e = onsetStep + 1;
    while (e < kStepsPerBar && c.steps[(size_t) e].kind == StepKind::Hold) ++e;
    return e; // exclusive
}

int noteStartContaining (const RhythmCell& c, int step) noexcept
{
    int s = step;
    while (s >= 0 && c.steps[(size_t) s].kind == StepKind::Hold) --s;
    return (s >= 0 && isOnset (c.steps[(size_t) s])) ? s : -1;
}

void clearSlidesInto (RhythmCell& c, int onsetStep) noexcept
{
    if (onsetStep <= 0) return;
    const int prev = noteStartContaining (c, onsetStep - 1);
    if (prev >= 0) c.steps[(size_t) prev].slide = false;
}

std::vector<int> collect (const RhythmCell& c, auto pred)
{
    std::vector<int> v;
    for (int i = 0; i < kStepsPerBar; ++i)
        if (pred (i, c.steps[(size_t) i])) v.push_back (i);
    return v;
}

int pickFrom (const std::vector<int>& v, Rng& rng) { return v.empty() ? -1 : v[(size_t) rng.below ((uint32_t) v.size())]; }

bool sparseStyle (const FeelProfile& fp) noexcept { return fp.feel == Feel::Sparse || fp.feel == Feel::Stabby; }
} // namespace

std::string_view toString (Mutation m) noexcept
{
    static constexpr std::array<std::string_view, kNumMutations> names {
        "remove_onset", "add_onset", "split_note", "merge_notes", "anticipate", "delay_entry",
        "accent_shift", "ghost_note", "octave_punct", "pickup", "slide_toggle", "extend"
    };
    return names[(size_t) m % kNumMutations];
}

const FeelProfile& feelProfile (Feel f) noexcept { return kProfiles[(size_t) f % kNumFeels]; }

namespace rhythm
{
RhythmCell parseGrid (std::string_view g)
{
    RhythmCell c;
    for (int i = 0; i < kStepsPerBar && i < (int) g.size(); ++i)
    {
        Step s;
        switch (g[(size_t) i])
        {
            case 'A': s = { StepKind::Onset, 2, Role::Anchor, false }; break;
            case 'X': s = { StepKind::Onset, 2, Role::Normal, false }; break;
            case 'x': s = { StepKind::Onset, 1, Role::Normal, false }; break;
            case 'g': s = { StepKind::Onset, 0, Role::Ghost, false }; break;
            case 'o': s = { StepKind::Onset, 2, Role::Octave, false }; break;
            case '-': s = { StepKind::Hold, 1, Role::Normal, false }; break;
            case '~':
                s = { StepKind::Hold, 1, Role::Normal, false };
                if (int st = noteStartContaining (c, i - 1); st >= 0) c.steps[(size_t) st].slide = true;
                break;
            default: s = {}; break;
        }
        if (s.kind == StepKind::Hold && (i == 0 || c.steps[(size_t) i - 1].kind == StepKind::Rest))
            s = {}; // malformed hold after rest -> rest
        c.steps[(size_t) i] = s;
    }
    return c;
}

void applyFill (RhythmCell& c, std::string_view fill)
{
    const int n = (int) std::min<size_t> (fill.size(), (size_t) kStepsPerBar);
    const int from = kStepsPerBar - n;
    std::string grid = toGrid (c);
    for (int i = 0; i < n; ++i) grid[(size_t) (from + i)] = fill[(size_t) i];
    // preserve slide markers already encoded in toGrid, re-parse for consistency
    c = parseGrid (grid);
    // fill onsets keep their character but approach role is given to the last onset
    for (int i = kStepsPerBar - 1; i >= from; --i)
        if (isOnset (c.steps[(size_t) i]))
        {
            if (c.steps[(size_t) i].role == Role::Normal) c.steps[(size_t) i].role = Role::Approach;
            break;
        }
}

std::string toGrid (const RhythmCell& c)
{
    std::string g (kStepsPerBar, '.');
    for (int i = 0; i < kStepsPerBar; ++i)
    {
        const auto& s = c.steps[(size_t) i];
        if (s.kind == StepKind::Onset)
        {
            switch (s.role)
            {
                case Role::Anchor: g[(size_t) i] = 'A'; break;
                case Role::Ghost:  g[(size_t) i] = 'g'; break;
                case Role::Octave: g[(size_t) i] = 'o'; break;
                default: g[(size_t) i] = s.accent >= 2 ? 'X' : (s.accent == 0 ? 'g' : 'x'); break;
            }
        }
        else if (s.kind == StepKind::Hold) g[(size_t) i] = '-';
    }
    // mark slides on the last step of sliding notes when it is a hold
    for (int i = 0; i < kStepsPerBar; ++i)
        if (isOnset (c.steps[(size_t) i]) && c.steps[(size_t) i].slide)
        {
            const int e = noteEnd (c, i);
            if (e - 1 > i) g[(size_t) (e - 1)] = '~';
        }
    return g;
}

int onsetCount (const RhythmCell& c) noexcept
{
    int n = 0;
    for (const auto& s : c.steps) n += isOnset (s) ? 1 : 0;
    return n;
}

std::vector<NoteSlot> slots (const RhythmCell& c)
{
    std::vector<NoteSlot> out;
    for (int i = 0; i < kStepsPerBar; ++i)
    {
        const auto& s = c.steps[(size_t) i];
        if (! isOnset (s)) continue;
        NoteSlot ns;
        ns.step = i;
        ns.length = noteEnd (c, i) - i;
        ns.accent = s.accent;
        ns.role = s.role;
        const int e = i + ns.length;
        ns.slide = s.slide && e < kStepsPerBar && isOnset (c.steps[(size_t) e]);
        out.push_back (ns);
    }
    return out;
}

int longestRest (const RhythmCell& c) noexcept
{
    int best = 0, run = 0;
    for (const auto& s : c.steps)
    {
        run = (s.kind == StepKind::Rest) ? run + 1 : 0;
        best = std::max (best, run);
    }
    return best;
}

double onsetSimilarity (const RhythmCell& a, const RhythmCell& b) noexcept
{
    int inter = 0, uni = 0;
    for (int i = 0; i < kStepsPerBar; ++i)
    {
        const bool x = isOnset (a.steps[(size_t) i]), y = isOnset (b.steps[(size_t) i]);
        inter += (x && y) ? 1 : 0;
        uni += (x || y) ? 1 : 0;
    }
    return uni == 0 ? 1.0 : (double) inter / (double) uni;
}

Mutation pickMutation (Rng& rng, const FeelProfile& fp, Family fam)
{
    auto w = fp.mutationWeights;
    if (fam == Family::Acid)
    {
        w[(size_t) M::SlideToggle] += 2;
        w[(size_t) M::AccentShift] += 1;
    }
    else
    {
        w[(size_t) M::SlideToggle] = std::min (w[(size_t) M::SlideToggle], 1);
        w[(size_t) M::OctavePunct] += 1;
    }
    return (Mutation) rng.pickWeighted (w.data(), kNumMutations);
}

bool mutate (RhythmCell& cell, Mutation m, Rng& rng, const FeelProfile& fp, Family fam)
{
    RhythmCell c = cell;
    const int count = onsetCount (c);
    auto& st = c.steps;

    switch (m)
    {
        case M::RemoveOnset:
        {
            if (count - 1 < fp.minOnsets) return false;
            auto cand = collect (c, [] (int i, const Step& s) { return i != 0 && isOnset (s) && s.role != Role::Anchor && s.role != Role::Octave; });
            const int p = pickFrom (cand, rng);
            if (p < 0) return false;
            clearSlidesInto (c, p);
            const int e = noteEnd (c, p);
            for (int i = p; i < e; ++i) st[(size_t) i] = {};
            break;
        }
        case M::AddOnset:
        {
            if (count + 1 > fp.maxOnsets) return false;
            auto cand = collect (c, [&] (int i, const Step& s) {
                if (s.kind != StepKind::Rest || i % 4 == 0) return false; // never add onto a kick beat
                if (sparseStyle (fp))
                {
                    const bool prevOn = i > 0 && st[(size_t) i - 1].kind != StepKind::Rest;
                    const bool nextOn = i + 1 < kStepsPerBar && isOnset (st[(size_t) i + 1]);
                    if (prevOn || nextOn) return false;
                }
                return true;
            });
            const int p = pickFrom (cand, rng);
            if (p < 0) return false;
            const bool ghost = rng.chancePermille (fp.ghostPermille);
            st[(size_t) p] = ghost ? Step { StepKind::Onset, 0, Role::Ghost, false } : Step { StepKind::Onset, 1, Role::Normal, false };
            break;
        }
        case M::SplitNote:
        {
            if (count + 1 > fp.maxOnsets) return false;
            auto cand = collect (c, [&] (int i, const Step& s) { return isOnset (s) && noteEnd (c, i) - i >= 2; });
            const int p = pickFrom (cand, rng);
            if (p < 0) return false;
            const int len = noteEnd (c, p) - p;
            const int at = p + (len >= 4 ? 2 : 1);
            const bool ghost = rng.chancePermille (fp.ghostPermille + 200);
            st[(size_t) at] = ghost ? Step { StepKind::Onset, 0, Role::Ghost, false } : Step { StepKind::Onset, 1, Role::Normal, false };
            if (st[(size_t) p].slide) { st[(size_t) at].slide = true; st[(size_t) p].slide = false; }
            break;
        }
        case M::MergeNotes:
        {
            if (fp.feel == Feel::Stabby || count - 1 < fp.minOnsets) return false;
            auto cand = collect (c, [&] (int i, const Step& s) {
                return i > 0 && isOnset (s) && s.role != Role::Anchor && s.role != Role::Octave && s.accent < 2
                       && st[(size_t) i - 1].kind != StepKind::Rest;
            });
            const int p = pickFrom (cand, rng);
            if (p < 0) return false;
            const int prev = noteStartContaining (c, p - 1);
            const bool carrySlide = st[(size_t) p].slide;
            st[(size_t) p] = { StepKind::Hold, 1, Role::Normal, false };
            if (prev >= 0) st[(size_t) prev].slide = carrySlide;
            break;
        }
        case M::Anticipate:
        {
            auto cand = collect (c, [&] (int i, const Step& s) {
                return i >= 4 && i % 4 == 0 && isOnset (s) && st[(size_t) i - 1].kind == StepKind::Rest;
            });
            const int p = pickFrom (cand, rng);
            if (p < 0) return false;
            Step moved = st[(size_t) p];
            if (moved.role == Role::Anchor) moved.role = Role::Normal;
            moved.accent = 2; // anticipations are pushed
            st[(size_t) p - 1] = moved;
            st[(size_t) p] = { StepKind::Hold, 1, Role::Normal, false };
            break;
        }
        case M::DelayEntry:
        {
            if (! isOnset (st[0])) return false;
            const int d = rng.chancePermille (500) ? 1 : 2;
            for (int i = 1; i <= d; ++i)
                if (isOnset (st[(size_t) i])) return false;
            const Step first = st[0];
            for (int i = 0; i < d; ++i) st[(size_t) i] = {};
            st[(size_t) d] = first; // delayed root entry keeps the anchor role
            break;
        }
        case M::AccentShift:
        {
            auto normals = collect (c, [] (int, const Step& s) { return isOnset (s) && s.accent == 1 && s.role == Role::Normal; });
            auto accents = collect (c, [] (int, const Step& s) { return isOnset (s) && s.accent == 2 && s.role == Role::Normal; });
            const int a = pickFrom (normals, rng);
            if (a < 0) return false;
            st[(size_t) a].accent = 2;
            if (const int b = pickFrom (accents, rng); b >= 0) st[(size_t) b].accent = 1;
            break;
        }
        case M::GhostNote:
        {
            if (count + 1 > fp.maxOnsets) return false;
            auto cand = collect (c, [&] (int i, const Step& s) {
                return s.kind == StepKind::Rest && (i % 2 == 1) && i > 0 && st[(size_t) i - 1].kind != StepKind::Rest;
            });
            const int p = pickFrom (cand, rng);
            if (p < 0) return false;
            st[(size_t) p] = { StepKind::Onset, 0, Role::Ghost, false };
            clearSlidesInto (c, p);
            break;
        }
        case M::OctavePunct:
        {
            const int maxOct = fam == Family::Acid ? 2 : 1;
            int have = 0;
            for (const auto& s : st) have += (isOnset (s) && s.role == Role::Octave) ? 1 : 0;
            if (have >= maxOct) return false;
            auto cand = collect (c, [] (int i, const Step& s) { return isOnset (s) && s.role == Role::Normal && i % 2 == 0 && i % 4 != 0; });
            const int p = pickFrom (cand, rng);
            if (p < 0) return false;
            st[(size_t) p].role = Role::Octave;
            st[(size_t) p].accent = 2;
            break;
        }
        case M::Pickup:
        {
            if (count + 1 > fp.maxOnsets) return false;
            if (st[15].kind != StepKind::Rest || isOnset (st[14])) return false;
            st[15] = { StepKind::Onset, 1, Role::Approach, false };
            break;
        }
        case M::SlideToggle:
        {
            auto cand = collect (c, [&] (int i, const Step& s) {
                if (! isOnset (s) || s.role == Role::Ghost) return false;
                const int e = noteEnd (c, i);
                return e < kStepsPerBar && isOnset (st[(size_t) e]) && st[(size_t) e].role != Role::Ghost;
            });
            const int p = pickFrom (cand, rng);
            if (p < 0) return false;
            if (fam == Family::Sub && ! st[(size_t) p].slide)
            {
                int slides = 0;
                for (const auto& s : st) slides += (isOnset (s) && s.slide) ? 1 : 0;
                if (slides >= 1) return false; // Sub: at most one glide per bar
            }
            st[(size_t) p].slide = ! st[(size_t) p].slide;
            break;
        }
        case M::Extend:
        {
            // tie a note into the rest that follows it (1-2 steps), keeping a gap before the next onset
            auto cand = collect (c, [&] (int i, const Step& s) {
                if (! isOnset (s) || s.role == Role::Ghost) return false;
                const int e = noteEnd (c, i);
                return e < kStepsPerBar && st[(size_t) e].kind == StepKind::Rest;
            });
            const int p = pickFrom (cand, rng);
            if (p < 0) return false;
            int e = noteEnd (c, p);
            const int maxAdd = rng.range (1, 2);
            for (int k = 0; k < maxAdd && e < kStepsPerBar && st[(size_t) e].kind == StepKind::Rest; ++k, ++e)
                st[(size_t) e] = { StepKind::Hold, 1, Role::Normal, false };
            break;
        }
        case M::Count: return false;
    }

    // global bounds re-check
    const int n = onsetCount (c);
    if (n < fp.minOnsets || n > fp.maxOnsets) return false;
    if (n == 0) return false;
    cell = c;
    return true;
}
} // namespace rhythm
} // namespace gfb

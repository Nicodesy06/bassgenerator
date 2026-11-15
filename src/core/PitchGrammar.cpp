#include "core/PitchGrammar.h"

#include <algorithm>
#include <array>
#include <cstdlib>

#include "core/ScaleTheory.h"

namespace gfb
{
bool MotifVocabulary::contains (int semis) const noexcept
{
    return std::any_of (entries.begin(), entries.end(), [semis] (const VocabEntry& e) { return e.semis == semis; });
}
}

namespace gfb::pitch
{
namespace
{
constexpr int kNA = 999;
constexpr int kRoles = (int) PitchRole::Count;
//                                        Root Oct Fif FBl 7Bl 3rd 4th 2nd 6th 6Bl O3
// v4 (docs/BASS_RESEARCH.md R3): colour tones of a root-centred club line --
// octave, fifth, b7 below and b3 first; 4th/2nd/6th are rare (ACID only).
constexpr std::array<int, kRoles> kSubWeights  { 0,  6,  5,  3,  4,  3,  1,  0,  0,  1,  0 };
constexpr std::array<int, kRoles> kAcidWeights { 0,  5,  4,  2,  4,  4,  2,  2,  1,  1,  1 };

int firstPresent (ScaleType s, std::initializer_list<int> ivs) noexcept
{
    for (int iv : ivs)
        if (theory::degreeOfInterval (s, iv) >= 0) return iv;
    return kNA;
}

bool isStable (int semis) noexcept { return semis == 0 || semis == 12 || semis == 7 || semis == -5 || semis == -12; }

int clampToRegister (int semis, Family f) noexcept
{
    while (semis < minSemis (f)) semis += 12;
    while (semis > maxSemis (f)) semis -= 12;
    return semis;
}

int pickApproach (Rng& rng, const PitchContext& ctx)
{
    const int second = roleSemis (PitchRole::Second, ctx.scale);
    const int sevB = roleSemis (PitchRole::SeventhBelow, ctx.scale);
    const int fifB = roleSemis (PitchRole::FifthBelow, ctx.scale);
    const int fifth = roleSemis (PitchRole::Fifth, ctx.scale);
    const std::array<int, 5> cand { second, sevB, fifB, fifth, 12 };
    std::array<int, 5> w = ctx.family == Family::Sub ? std::array<int, 5> { 0, 4, 2, 1, 2 } : std::array<int, 5> { 2, 4, 1, 1, 2 };
    for (size_t i = 0; i < cand.size(); ++i)
        if (cand[i] == kNA) w[i] = 0;
    return cand[(size_t) rng.pickWeighted (w.data(), (int) w.size())];
}

} // namespace

int minSemis (Family f) noexcept { return f == Family::Sub ? -7 : -5; }
int maxSemis (Family f) noexcept { return f == Family::Sub ? 12 : 17; }

int roleSemis (PitchRole r, ScaleType s) noexcept
{
    const auto& iv = theory::intervals (s);
    switch (r)
    {
        case PitchRole::Root: return 0;
        case PitchRole::Octave: return 12;
        case PitchRole::Fifth: return firstPresent (s, { 7 });
        case PitchRole::FifthBelow: return firstPresent (s, { 7 }) == kNA ? kNA : -5;
        case PitchRole::SeventhBelow: return iv.back() - 12;
        case PitchRole::Third: return firstPresent (s, { 3, 4 });
        case PitchRole::Fourth: return firstPresent (s, { 5 });
        case PitchRole::Second: return firstPresent (s, { 2, 1 });
        case PitchRole::Sixth: return firstPresent (s, { 8, 9 });
        case PitchRole::SixthBelow: { const int v = firstPresent (s, { 8, 9 }); return v == kNA ? kNA : v - 12; }
        case PitchRole::OctaveThird: { const int v = firstPresent (s, { 3, 4 }); return v == kNA ? kNA : v + 12; }
        case PitchRole::Count: break;
    }
    return kNA;
}

MotifVocabulary chooseVocabulary (Rng& rng, const PitchContext& ctx)
{
    MotifVocabulary v;
    v.entries.push_back ({ PitchRole::Root, 0 });
    auto has = [&] (PitchRole r) { return std::any_of (v.entries.begin(), v.entries.end(), [r] (auto& e) { return e.role == r; }); };
    auto add = [&] (PitchRole r) {
        const int s = roleSemis (r, ctx.scale);
        if (s == kNA || has (r) || s < minSemis (ctx.family) || s > maxSemis (ctx.family)) return false;
        v.entries.push_back ({ r, s });
        return true;
    };

    // v4: root + 1-2 colour tones (SUB) / 2-3 (ACID); pedal lines keep a single colour
    int size = ctx.family == Family::Sub ? 1 + rng.range (1, 2) : 1 + rng.range (2, 3);
    if (ctx.contour == ContourHint::Pedal) size = 2;

    switch (ctx.contour)
    {
        case ContourHint::RootFifth:    if (! add (rng.chancePermille (550) ? PitchRole::Fifth : PitchRole::FifthBelow)) add (PitchRole::Octave); break;
        case ContourHint::OctaveBounce: add (PitchRole::Octave); break;
        case ContourHint::Stepwise:     add (PitchRole::SeventhBelow); if (ctx.family == Family::Acid) add (PitchRole::Third); break;
        case ContourHint::CallResponse: if (! add (PitchRole::Fourth)) add (PitchRole::Fifth); break;
        case ContourHint::Pedal:        add (PitchRole::Octave); break;
        case ContourHint::Riff:         break;
    }

    auto weights = ctx.family == Family::Sub ? kSubWeights : kAcidWeights;
    for (int guard = 0; (int) v.entries.size() < size && guard < 32; ++guard)
    {
        auto w = weights;
        for (int r = 0; r < kRoles; ++r)
            if (has ((PitchRole) r) || roleSemis ((PitchRole) r, ctx.scale) == kNA) w[(size_t) r] = 0;
        const int total = [&] { int t = 0; for (int x : w) t += x; return t; }();
        if (total == 0) break;
        add ((PitchRole) rng.pickWeighted (w.data(), kRoles));
    }
    return v;
}

std::vector<int> assignPitches (const std::vector<NoteSlot>& slots, const MotifVocabulary& vocab, Rng& rng,
                                const PitchContext& ctx, BarFunction fn, int prevSemis)
{
    // v4 (docs/BASS_RESEARCH.md R2/R3): the position inside the beat decides
    // the pitch role. `&` (off-8th) carries the tonic or its octave, `e` rolls
    // on the tonic with occasional colour, `a` is the pickup/colour position.
    // The tonic dominates; colours never walk by scale step.
    std::vector<int> out;
    out.reserve (slots.size());
    int prev = prevSemis, run = 0;
    const int count = (int) slots.size();
    const int maxDistinct = ctx.family == Family::Acid ? 4 : 3;
    std::vector<int> used;
    for (int i = 0; i < count; ++i)
    {
        const auto& sl = slots[(size_t) i];
        const int pos = sl.step % 4; // 0 kick, 1 e, 2 &, 3 a
        int p = 0;
        if (sl.role == Role::Ghost) p = i == 0 ? 0 : prev;
        else if (sl.role == Role::Octave) p = 12;
        else if (sl.role == Role::Approach) p = pickApproach (rng, ctx);
        else if (sl.role == Role::Anchor || (i == 0 && fn != BarFunction::Answer))
            p = rng.chancePermille (ctx.family == Family::Sub ? 920 : 820) ? 0 : 12;
        else
        {
            std::vector<int> w (vocab.entries.size());
            for (size_t k = 0; k < vocab.entries.size(); ++k)
            {
                const auto& e = vocab.entries[k];
                const bool tonic = e.semis == 0;
                const bool octave = e.semis == 12;
                int x;
                switch (pos)
                {
                    case 2:  x = tonic ? 10 : (octave ? 4 : 1); break;          // off-8th: the groove note
                    case 1:  x = tonic ? 7 : (octave ? 3 : 2); break;           // e: roll
                    case 3:  x = tonic ? 4 : 3; break;                          // a: pickup / colour
                    default: x = tonic ? 9 : 1; break;                          // kick step (non four-floor contexts)
                }
                if (sl.accent >= 2 && ! (tonic || octave)) x = std::max (1, x / 2);
                if (fn == BarFunction::Answer && ! tonic) x = x * 3 / 2 + 1;
                // a colour is a flick: never repeat it more than twice
                if (! tonic && e.semis == prev && run >= 2) x = 0;
                // no leaps wider than an octave between consecutive notes
                if (std::abs (e.semis - prev) > 12) x = 0;
                // at most 3 (ACID 4) different pitches per bar
                if ((int) used.size() >= maxDistinct && std::find (used.begin(), used.end(), e.semis) == used.end()) x = 0;
                switch (ctx.contour)
                {
                    case ContourHint::Pedal:        if (tonic) x *= 2; break;
                    case ContourHint::OctaveBounce: if (octave) x = x * 2 + 1; break;
                    case ContourHint::RootFifth:    if (e.semis == 7 || e.semis == -5) x = x * 2 + 1; break;
                    case ContourHint::CallResponse:
                        if (i * 2 < count) { if (tonic) x *= 2; }
                        else if (! tonic) x = x * 2 + 1;
                        break;
                    default: break;
                }
                w[k] = x;
            }
            int total = 0;
            for (int x : w) total += x;
            p = total > 0 ? vocab.entries[(size_t) rng.pickWeighted (w.data(), (int) w.size())].semis : 0;
        }
        p = clampToRegister (p, ctx.family);
        run = (p == prev && i > 0) ? run + 1 : 1;
        prev = p;
        if (std::find (used.begin(), used.end(), p) == used.end()) used.push_back (p);
        out.push_back (p);
    }
    return out;
}

void varyPitches (const std::vector<NoteSlot>& slots, std::vector<int>& pitches, const MotifVocabulary& vocab,
                  Rng& rng, const PitchContext& ctx, int maxChanges)
{
    std::vector<int> cand;
    for (size_t i = 0; i < slots.size(); ++i)
        if (slots[i].role == Role::Normal || slots[i].role == Role::Approach)
            if (i != 0 && slots[i].step % 4 != 2) cand.push_back ((int) i); // v4: the off-8th groove note stays
    if (cand.empty())
        for (size_t i = 1; i < slots.size(); ++i)
            if (slots[i].role == Role::Normal) cand.push_back ((int) i);
    const int changes = std::min<int> (maxChanges, (int) cand.size());
    for (int c = 0; c < changes; ++c)
    {
        const int pick = (int) rng.below ((uint32_t) cand.size());
        const int i = cand[(size_t) pick];
        cand.erase (cand.begin() + pick);
        const int cur = pitches[(size_t) i];
        const int before = pitches[(size_t) i - 1];
        std::vector<int> w (vocab.entries.size());
        for (size_t k = 0; k < vocab.entries.size(); ++k)
        {
            const int s = vocab.entries[k].semis;
            if (s == cur) { w[k] = 0; continue; }
            const int a = std::abs (s - before);
            w[k] = s == 0 ? 4 : (s == 12 || s == 7 || s == -5 || s == -2 ? 3 : 2); // v4: root-centred colours
            if (a > 12) w[k] = 0;
        }
        int total = 0;
        for (int x : w) total += x;
        if (total == 0) continue;
        pitches[(size_t) i] = clampToRegister (vocab.entries[(size_t) rng.pickWeighted (w.data(), (int) w.size())].semis, ctx.family);
        if (cand.empty()) break;
    }
    for (size_t i = 1; i < slots.size(); ++i)
        if (slots[i].role == Role::Ghost) pitches[i] = pitches[i - 1];
}

int transposeDiatonic (int semis, int steps, ScaleType s) noexcept
{
    const int pc = ((semis % 12) + 12) % 12;
    const int deg = theory::degreeOfInterval (s, pc);
    if (deg < 0) return semis;
    const int octave = (semis - pc) / 12;
    return theory::semitones (s, theory::step (s, { deg, octave }, steps));
}

void resolveEnding (const std::vector<NoteSlot>& slots, std::vector<int>& pitches, const PitchContext& ctx, Rng& rng)
{
    if (slots.empty()) return;
    const size_t last = slots.size() - 1;
    const int p = pitches[last];
    const bool nearRoot = std::abs (p) <= 2 || std::abs (p - 12) <= 2;
    if (isStable (p) || nearRoot) return;
    int np;
    const int r = rng.range (0, 9);
    if (r < (ctx.family == Family::Sub ? 5 : 3)) np = 0;
    else if (r < 7) np = roleSemis (PitchRole::Fifth, ctx.scale) == kNA ? 12 : (ctx.family == Family::Sub ? -5 : 7);
    else np = pickApproach (rng, ctx);
    pitches[last] = clampToRegister (np, ctx.family);
}

std::vector<int> fillPitches (const std::vector<NoteSlot>& slots, int fromStep, FillShape shape,
                              const std::vector<int>& base, const PitchContext& ctx)
{
    std::vector<int> out = base;
    std::vector<size_t> idx;
    for (size_t i = 0; i < slots.size(); ++i)
        if (slots[i].step >= fromStep) idx.push_back (i);
    const int k = (int) idx.size();
    if (k == 0) return out;
    for (int j = 0; j < k; ++j)
    {
        int p = 0;
        switch (shape)
        {
            // v4: fills move between chord tones, never as scale runs
            case FillShape::RunUp:
            {
                const int fifthBelow = roleSemis (PitchRole::FifthBelow, ctx.scale) == kNA ? -12 : -5;
                const int seventhBelow = roleSemis (PitchRole::SeventhBelow, ctx.scale);
                const int cyc[3] = { 0, fifthBelow, seventhBelow };
                p = j == k - 1 ? seventhBelow : cyc[j % 3]; // ends on the b7 pickup into the root
                break;
            }
            case FillShape::RunDown:
            {
                const int fifth = roleSemis (PitchRole::Fifth, ctx.scale) == kNA ? 0 : 7;
                const int down[3] = { 12, roleSemis (PitchRole::SeventhBelow, ctx.scale) + 12, fifth };
                p = down[std::min (j, 2)];
                break;
            }
            case FillShape::OctaveJump: p = (j % 2 == 0) ? 0 : 12; break;
            case FillShape::Approach:   p = (j == k - 1) ? transposeDiatonic (0, -1, ctx.scale) : (roleSemis (PitchRole::Fifth, ctx.scale) == kNA ? 12 : 7); break;
            case FillShape::Pedal:      p = (j == k - 1) ? 12 : 0; break;
            case FillShape::Break:      p = 0; break;
        }
        if (slots[idx[(size_t) j]].role == Role::Octave) p = 12;
        out[idx[(size_t) j]] = p;
    }
    // keep the whole run in register by octave-shifting it as a block
    int lo = 1000, hi = -1000;
    for (size_t i : idx) { lo = std::min (lo, out[i]); hi = std::max (hi, out[i]); }
    int shift = 0;
    while (lo + shift < minSemis (ctx.family)) shift += 12;
    while (hi + shift > maxSemis (ctx.family) && lo + shift - 12 >= minSemis (ctx.family)) shift -= 12;
    for (size_t i : idx) out[i] = clampToRegister (out[i] + shift, ctx.family);
    return out;
}
} // namespace gfb::pitch

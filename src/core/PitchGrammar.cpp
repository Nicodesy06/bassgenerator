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
constexpr std::array<int, kRoles> kSubWeights  { 0,  3,  4,  4,  4,  5,  5,  3,  0,  1,  0 };
constexpr std::array<int, kRoles> kAcidWeights { 0,  4,  5,  3,  4,  5,  5,  3,  2,  1,  1 };

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
    std::array<int, 5> w = ctx.family == Family::Sub ? std::array<int, 5> { 1, 3, 3, 2, 1 } : std::array<int, 5> { 3, 3, 1, 1, 2 };
    for (size_t i = 0; i < cand.size(); ++i)
        if (cand[i] == kNA) w[i] = 0;
    return cand[(size_t) rng.pickWeighted (w.data(), (int) w.size())];
}

int rolePrior (PitchRole r) noexcept
{
    switch (r)
    {
        case PitchRole::Root: return 5;
        case PitchRole::Fifth: case PitchRole::Octave: case PitchRole::FifthBelow: return 4;
        default: return 3;
    }
}

int transitionWeight (const VocabEntry& e, int prev, int prevLeap, int run, const NoteSlot& slot,
                      const PitchContext& ctx, BarFunction fn, int slotIndex, int slotCount)
{
    const int iv = e.semis - prev;
    const int a = std::abs (iv);
    int w = rolePrior (e.role);
    // interval preferences re-balanced against the reference basslines:
    // repeats ~40 %, steps ~20 %, fourth/fifth ~17 %, octave jumps ~7 %
    if (ctx.family == Family::Sub)
    {
        if (a == 0) w *= 4;
        else if (a <= 2) w *= 3;
        else if (a == 5 || a == 7) w *= 3;
        else if (a <= 4) w *= 2;
        else if (a == 12) w *= 1;
        else w *= 1;
    }
    else
    {
        if (a == 0) w *= 4;
        else if (a <= 2) w *= 4;
        else if (a <= 5) w *= 3;
        else if (a == 7) w *= 2;
        else if (a == 12) w *= 1;
        else w *= 1;
    }
    // after a leap, prefer to turn back (gap-fill) and forbid a second leap the same way
    if (std::abs (prevLeap) > 5)
    {
        const bool sameDir = (iv > 0) == (prevLeap > 0) && iv != 0;
        if (sameDir && a > 2) w = 0;
        else if (! sameDir && iv != 0) w *= 2;
    }
    // stop endless repetition: Acid max 3 identical in a row, Sub max 4 (pedal contour 6)
    const int maxRun = ctx.family == Family::Acid ? 3 : (ctx.contour == ContourHint::Pedal ? 5 : 3);
    if (a == 0 && run >= maxRun) w = 0;
    // accented notes lean on stable tones
    if (slot.accent >= 2 && isStable (e.semis)) w *= 2;
    switch (ctx.contour)
    {
        case ContourHint::Pedal:        if (e.semis == 0) w *= 3; break;
        case ContourHint::OctaveBounce: if (e.semis == 0 || e.semis == 12) w *= 2; break;
        case ContourHint::Stepwise:     if (a > 0 && a <= 2) w *= 2; break;
        case ContourHint::RootFifth:    if (e.semis == 7 || e.semis == -5) w *= 2; break;
        case ContourHint::Riff:         break;
        case ContourHint::CallResponse:
            if (slotIndex * 2 < slotCount) { if (e.semis == 0) w *= 2; }
            else if (e.semis != 0) w *= 2;
            break;
    }
    if (fn == BarFunction::Answer && e.semis != 0) w = w * 3 / 2;
    return w;
}
} // namespace

int minSemis (Family f) noexcept { return f == Family::Sub ? -7 : -5; }
int maxSemis (Family f) noexcept { return f == Family::Sub ? 12 : 15; }

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

    int size = ctx.family == Family::Sub ? 1 + rng.range (2, 5) : 1 + rng.range (3, 5);
    if (ctx.contour == ContourHint::Pedal) size = std::min (size, ctx.family == Family::Sub ? 2 : 3);
    if (ctx.contour == ContourHint::Riff) size = std::clamp (size, 3, 4);

    switch (ctx.contour)
    {
        case ContourHint::RootFifth:    if (! add (rng.chancePermille (550) ? PitchRole::Fifth : PitchRole::FifthBelow)) add (PitchRole::Octave); break;
        case ContourHint::OctaveBounce: add (PitchRole::Octave); break;
        case ContourHint::Stepwise:     if (! add (PitchRole::Second)) add (PitchRole::Third); add (PitchRole::SeventhBelow); break;
        case ContourHint::CallResponse: if (! add (PitchRole::Fourth)) add (PitchRole::Fifth); break;
        case ContourHint::Pedal: case ContourHint::Riff: break;
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
    std::vector<int> out;
    out.reserve (slots.size());
    int prev = prevSemis, prevLeap = 0, run = 0;
    const int count = (int) slots.size();
    for (int i = 0; i < count; ++i)
    {
        const auto& sl = slots[(size_t) i];
        int p = 0;
        if (sl.role == Role::Ghost)
        {
            p = i == 0 ? 0 : prev;
        }
        else if (sl.role == Role::Octave)
        {
            p = 12;
        }
        else if (sl.role == Role::Approach)
        {
            p = pickApproach (rng, ctx);
        }
        else if (sl.role == Role::Anchor || (i == 0 && sl.step <= 2 && fn != BarFunction::Answer))
        {
            const int rootPermille = ctx.family == Family::Sub ? 900 : 760;
            if (rng.chancePermille (rootPermille)) p = 0;
            else
            {
                const std::array<int, 3> c { 12, roleSemis (PitchRole::Fifth, ctx.scale), roleSemis (PitchRole::FifthBelow, ctx.scale) };
                std::array<int, 3> w { 3, c[1] == kNA ? 0 : 2, c[2] == kNA ? 0 : 1 };
                p = c[(size_t) rng.pickWeighted (w.data(), 3)];
            }
        }
        else
        {
            std::vector<int> w (vocab.entries.size());
            for (size_t k = 0; k < vocab.entries.size(); ++k)
                w[k] = transitionWeight (vocab.entries[k], prev, prevLeap, run, sl, ctx, fn, i, count);
            const int idx = rng.pickWeighted (w.data(), (int) w.size());
            p = vocab.entries[(size_t) idx].semis;
        }
        p = clampToRegister (p, ctx.family);
        run = (p == prev && i > 0) ? run + 1 : 1;
        prevLeap = i > 0 ? p - prev : 0;
        prev = p;
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
            if (i != 0) cand.push_back ((int) i);
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
            w[k] = ctx.family == Family::Acid ? (a <= 2 ? 5 : (a <= 5 ? 3 : (a == 12 ? 2 : 1)))
                                              : ((a == 5 || a == 7 || a == 12 || a == 0) ? 4 : (a <= 3 ? 2 : 1));
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
            case FillShape::RunUp:      p = transposeDiatonic (0, -(k - j), ctx.scale); break;
            case FillShape::RunDown:    p = transposeDiatonic (12, -j, ctx.scale); break;
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

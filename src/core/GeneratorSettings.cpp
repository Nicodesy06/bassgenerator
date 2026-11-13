#include "core/GeneratorSettings.h"

#include <algorithm>
#include <array>

#include "core/SeedRng.h"

namespace gfb
{
namespace
{
constexpr std::array<std::string_view, kNumFamilies> kFamilyNames { "SUB", "ACID" };
constexpr std::array<std::string_view, kNumFeels> kFeelNames { "RANDOM", "SPARSE", "BALANCED", "BUSY", "ROLLING", "SYNCOPATED", "STABBY" };
constexpr std::array<std::string_view, kNumScales> kScaleIds { "natural_minor", "major", "dorian", "phrygian", "mixolydian", "harmonic_minor", "minor_pentatonic" };
constexpr std::array<std::string_view, kNumScales> kScaleNames { "Natural Minor", "Major", "Dorian", "Phrygian", "Mixolydian", "Harmonic Minor", "Minor Pentatonic" };
constexpr std::array<std::string_view, kNumKickContexts> kKickNames { "FOUR_ON_FLOOR", "BROKEN", "OPEN" };

template <typename E, size_t N>
std::optional<E> lookup (const std::array<std::string_view, N>& names, std::string_view s) noexcept
{
    for (size_t i = 0; i < N; ++i)
        if (names[i] == s) return (E) i;
    return std::nullopt;
}
} // namespace

GeneratorSettings sanitise (GeneratorSettings s) noexcept
{
    s.root = ((s.root % 12) + 12) % 12;
    if ((int) s.scale >= kNumScales) s.scale = ScaleType::NaturalMinor;
    if ((int) s.family >= kNumFamilies) s.family = Family::Sub;
    if ((int) s.feel >= kNumFeels) s.feel = Feel::Balanced;
    if (! (s.bars == 1 || s.bars == 2 || s.bars == 4 || s.bars == 8)) s.bars = 4;
    s.octave = std::clamp (s.octave, kMinOctave, kMaxOctave);
    if ((int) s.kickContext >= kNumKickContexts) s.kickContext = KickContext::FourOnFloor;
    if (s.swing != kAutoSwing) s.swing = std::clamp (s.swing, kMinSwing, kMaxSwing);
    return s;
}

std::string_view toString (Family f) noexcept { return kFamilyNames[(size_t) f % kNumFamilies]; }
std::string_view toString (Feel f) noexcept { return kFeelNames[(size_t) f % kNumFeels]; }
std::string_view toString (ScaleType s) noexcept { return kScaleIds[(size_t) s % kNumScales]; }
std::string_view toString (KickContext k) noexcept { return kKickNames[(size_t) k % kNumKickContexts]; }
std::string_view scaleDisplayName (ScaleType s) noexcept { return kScaleNames[(size_t) s % kNumScales]; }
std::optional<Family> familyFromString (std::string_view s) noexcept { return lookup<Family> (kFamilyNames, s); }
std::optional<Feel> feelFromString (std::string_view s) noexcept { return lookup<Feel> (kFeelNames, s); }
std::optional<ScaleType> scaleFromString (std::string_view s) noexcept { return lookup<ScaleType> (kScaleIds, s); }
std::optional<KickContext> kickContextFromString (std::string_view s) noexcept { return lookup<KickContext> (kKickNames, s); }

std::optional<uint64_t> parseSeed (std::string_view t) noexcept
{
    while (! t.empty() && (t.front() == ' ' || t.front() == '\t')) t.remove_prefix (1);
    while (! t.empty() && (t.back() == ' ' || t.back() == '\t' || t.back() == '\n' || t.back() == '\r')) t.remove_suffix (1);
    if (t.empty()) return std::nullopt;
    uint64_t base = 10;
    if (t.size() > 2 && t[0] == '0' && (t[1] == 'x' || t[1] == 'X')) { base = 16; t.remove_prefix (2); }
    if (t.empty() || t.size() > 20) return std::nullopt;
    uint64_t v = 0;
    for (char c : t)
    {
        uint64_t d;
        if (c >= '0' && c <= '9') d = (uint64_t) (c - '0');
        else if (base == 16 && c >= 'a' && c <= 'f') d = (uint64_t) (c - 'a' + 10);
        else if (base == 16 && c >= 'A' && c <= 'F') d = (uint64_t) (c - 'A' + 10);
        else return std::nullopt;
        if (v > (UINT64_MAX - d) / base) return std::nullopt; // overflow
        v = v * base + d;
    }
    return v;
}

std::string seedToString (uint64_t seed) { return std::to_string (seed); }

GeneratorSettings rollSettings (const GeneratorSettings& in, const LockSet& l, RollMode mode, uint64_t entropy)
{
    GeneratorSettings s = sanitise (in);
    Rng r (deriveSeed (entropy, "roll"));
    auto newSeed = [&] { return (uint64_t) r.range (1, 999999); };
    const bool style = mode == RollMode::Seeds || mode == RollMode::StyleOnly || mode == RollMode::Everything;
    const bool var = mode == RollMode::Seeds || mode == RollMode::VarOnly || mode == RollMode::Everything;
    if (style && ! l.styleSeed) { const auto old = s.styleSeed; do s.styleSeed = newSeed(); while (s.styleSeed == old); }
    if (var && ! l.varSeed) { const auto old = s.varSeed; do s.varSeed = newSeed(); while (s.varSeed == old); }
    if (mode == RollMode::Everything)
    {
        if (! l.root) s.root = r.range (0, 11);
        if (! l.scale)
        {
            // club-bass weighting: minor modes favoured
            const std::array<int, kNumScales> w { 4, 1, 3, 2, 1, 1, 2 };
            s.scale = (ScaleType) r.pickWeighted (w.data(), kNumScales);
        }
        if (! l.family) s.family = r.chancePermille (500) ? Family::Sub : Family::Acid;
        if (! l.feel) s.feel = (Feel) r.range (1, kNumFeels - 1);
        if (! l.bars)
        {
            const std::array<int, 4> bars { 1, 2, 4, 8 };
            const std::array<int, 4> w { 1, 2, 4, 2 };
            s.bars = bars[(size_t) r.pickWeighted (w.data(), 4)];
        }
        if (! l.octave)
        {
            const std::array<int, 3> w { 1, 4, 1 };
            s.octave = r.pickWeighted (w.data(), 3) - 1;
        }
    }
    return s;
}
} // namespace gfb

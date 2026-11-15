#pragma once
#include <array>
#include <cstdint>
#include <optional>
#include <string>
#include <string_view>

namespace gfb
{
enum class Family : uint8_t { Sub = 0, Acid = 1 };
enum class Feel : uint8_t { Random = 0, Sparse, Balanced, Busy, Rolling, Syncopated, Stabby };
enum class ScaleType : uint8_t { NaturalMinor = 0, Major, Dorian, Phrygian, Mixolydian, HarmonicMinor, MinorPentatonic };
/** Implied drum context used as a *prior* by grammar selection and scoring. */
enum class KickContext : uint8_t { FourOnFloor = 0, Broken, Open };

constexpr int kNumFamilies     = 2;
constexpr int kNumFeels        = 7; // including Random
constexpr int kNumScales       = 7;
constexpr int kNumKickContexts = 3;
constexpr int kMinOctave = -3, kMaxOctave = 3;
constexpr int kAutoSwing = -1;
constexpr int kMinSwing = 50, kMaxSwing = 66;

struct GeneratorSettings
{
    int root = 9;                                  // pitch class 0..11 (9 = A)
    ScaleType scale = ScaleType::NaturalMinor;
    Family family = Family::Sub;
    Feel feel = Feel::Balanced;
    int bars = 4;                                  // 1, 2, 4, 8
    int octave = 0;                                // -3..+3 relative to family reference register
    uint64_t styleSeed = 1;
    uint64_t varSeed = 1;
    KickContext kickContext = KickContext::FourOnFloor;
    int swing = kAutoSwing;                        // -1 = feel default, else 50..66 (%)
    bool chromatic = false;                        // labelled chromatic embellishment mode (approach notes)

    bool operator== (const GeneratorSettings&) const = default;
};

/** Which settings are protected from the Roll / Randomise actions. */
struct LockSet
{
    bool root = false, scale = false, family = false, feel = false;
    bool bars = true, octave = true;
    bool styleSeed = false, varSeed = false;

    bool operator== (const LockSet&) const = default;
};

/** Clamp/repair every field into its legal range. */
GeneratorSettings sanitise (GeneratorSettings s) noexcept;

std::string_view toString (Family) noexcept;
std::string_view toString (Feel) noexcept;
std::string_view toString (ScaleType) noexcept;
std::string_view toString (KickContext) noexcept;
std::optional<Family> familyFromString (std::string_view) noexcept;
std::optional<Feel> feelFromString (std::string_view) noexcept;
std::optional<ScaleType> scaleFromString (std::string_view) noexcept;
std::optional<KickContext> kickContextFromString (std::string_view) noexcept;

/** Display name of a scale for the UI ("Natural Minor", ...). */
std::string_view scaleDisplayName (ScaleType) noexcept;

/** Parse an unsigned 64-bit decimal (or 0x-prefixed hex) seed. Rejects
    overflow, signs, empty strings and garbage. Leading/trailing spaces ok. */
std::optional<uint64_t> parseSeed (std::string_view text) noexcept;
std::string seedToString (uint64_t seed);

enum class RollMode : uint8_t
{
    Seeds,      // Generate: new STYLE + VAR (unless locked), other settings kept
    StyleOnly,  // new STYLE only (unless locked)
    VarOnly,    // new VAR only (unless locked)
    Everything  // Randomise: every unlocked setting + seeds, musically weighted
};

/** Pure, deterministic for a given `entropy`. Locked fields are never changed.
    New seeds are drawn from 1..999999 so they stay easy to read/type; any
    uint64 value can still be entered manually. */
GeneratorSettings rollSettings (const GeneratorSettings& s, const LockSet& locks, RollMode mode, uint64_t entropy);
} // namespace gfb

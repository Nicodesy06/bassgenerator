#pragma once
// Original, hand-authored rhythm archetype corpus for GrooveForge Bass.
// Every grid is 16 sixteenth-steps (one 4/4 bar) written in a readable
// notation (see kGridLegend). All material was written for this project; no
// third-party songs, sample packs or commercial pattern datasets were used.

#include <cstdint>
#include <span>
#include <string_view>

#include "core/GeneratorSettings.h"

namespace gfb
{
/** Grid legend (one char per 16th step):
    A = anchor onset (accented, prefers root / strong role)
    X = accented onset
    x = normal onset
    g = ghost onset (soft, short, usually repeats previous pitch)
    o = octave punctuation onset (accented, octave-up role)
    - = hold (previous note continues through this step)
    ~ = hold + slide into the next onset (must be followed by an onset)
    . = rest */
inline constexpr std::string_view kGridLegend = "A X x g o - ~ .";

enum class ContourHint : uint8_t
{
    Pedal,        // mostly root, rhythmic pedal
    RootFifth,    // root/fifth (and fifth below) alternation
    OctaveBounce, // root/octave jumps
    Stepwise,     // scale-step neighbour motion
    CallResponse, // first half stable, second half moves away and returns
    Riff          // compact 3-4 pitch hook cell
};

enum FamilyMask : uint8_t { kFamSub = 1, kFamAcid = 2, kFamBoth = 3 };
enum KickFitMask : uint8_t { kFitFour = 1, kFitBroken = 2, kFitOpen = 4, kFitAll = 7 };

struct RhythmArchetype
{
    int id;
    std::string_view name;
    std::string_view grid;     // 16 chars
    Feel feel;                 // primary feel
    Feel alsoFeel;             // secondary feel pool (Feel::Random = none)
    uint8_t families;          // FamilyMask
    uint8_t kickFit;           // KickFitMask
    ContourHint contour;
    int maxSwing;              // max swing % this groove tolerates
    std::string_view usage;    // intended use
};

enum class FillShape : uint8_t { RunUp, RunDown, OctaveJump, Break, Approach, Pedal };

struct FillCell
{
    int id;
    std::string_view name;
    std::string_view grid;     // 4 or 8 chars, replaces the end of a bar
    FillShape shape;
    uint8_t families;
};

std::span<const RhythmArchetype> rhythmCorpus() noexcept;
std::span<const FillCell> fillCorpus() noexcept;
const RhythmArchetype* findArchetype (int id) noexcept;
} // namespace gfb

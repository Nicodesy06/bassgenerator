#pragma once
#include <cstdint>
#include <string>
#include <vector>

#include "core/GeneratorSettings.h"

namespace gfb
{
/** A scale-relative pitch: degree index (may exceed the scale size or be
    negative, wrapping into neighbouring octaves) plus explicit octave offset. */
struct ScalePitch
{
    int degree = 0;
    int octave = 0;
    bool operator== (const ScalePitch&) const = default;
};

namespace theory
{
/** Interval set (semitones from root, ascending, starting with 0). */
const std::vector<int>& intervals (ScaleType s) noexcept;
int degreeCount (ScaleType s) noexcept;
/** Semitone offset from the root for a scale pitch (handles wrap). */
int semitones (ScaleType s, ScalePitch p) noexcept;
/** true if pitch class pc (0..11) belongs to root/scale. */
bool inScale (int root, ScaleType s, int pitchClass) noexcept;
/** Degree index of the perfect fifth, or -1 if the scale has none. */
int fifthDegree (ScaleType s) noexcept;
/** Degree index whose interval equals semis (mod 12) or -1. */
int degreeOfInterval (ScaleType s, int semis) noexcept;
/** Normalise so 0 <= degree < degreeCount (moving whole octaves). */
ScalePitch normalise (ScaleType s, ScalePitch p) noexcept;
/** Move by `steps` scale steps. */
ScalePitch step (ScaleType s, ScalePitch p, int steps) noexcept;

/** Reference MIDI note of the root for a family before octave transposition:
    SUB roots sit in C2..B2 (MIDI 36..47), ACID roots in F2..E3 (41..52). */
int referenceRootMidi (int rootPc, Family f) noexcept;

/** Consistent root display names (sharp/flat choice is fixed per root). */
std::string rootName (int rootPc);
/** Note name for a MIDI note spelled for the given key (e.g. "Bb1", "F#2"). */
std::string noteName (int midiNote, int keyRootPc, ScaleType s);
/** Does this key prefer flat spellings? */
bool prefersFlats (int keyRootPc, ScaleType s) noexcept;
} // namespace theory
} // namespace gfb

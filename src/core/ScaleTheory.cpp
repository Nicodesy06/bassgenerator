#include "core/ScaleTheory.h"

#include <array>

namespace gfb::theory
{
namespace
{
const std::array<std::vector<int>, kNumScales> kIntervals {
    std::vector<int> { 0, 2, 3, 5, 7, 8, 10 },  // natural minor (aeolian)
    std::vector<int> { 0, 2, 4, 5, 7, 9, 11 },  // major (ionian)
    std::vector<int> { 0, 2, 3, 5, 7, 9, 10 },  // dorian
    std::vector<int> { 0, 1, 3, 5, 7, 8, 10 },  // phrygian
    std::vector<int> { 0, 2, 4, 5, 7, 9, 10 },  // mixolydian
    std::vector<int> { 0, 2, 3, 5, 7, 8, 11 },  // harmonic minor
    std::vector<int> { 0, 3, 5, 7, 10 },        // minor pentatonic
};

int floorDiv (int a, int b) noexcept { int q = a / b; if ((a % b != 0) && ((a < 0) != (b < 0))) --q; return q; }
int floorMod (int a, int b) noexcept { return a - floorDiv (a, b) * b; }

// Major-key signature root pcs that use flats: F Bb Eb Ab Db (Gb written as F#)
bool majorKeyUsesFlats (int pc) noexcept { return pc == 5 || pc == 10 || pc == 3 || pc == 8 || pc == 1; }

// Relative-major offset per scale (mode): semitones from mode root down to its parent major root.
int parentMajorOffset (ScaleType s) noexcept
{
    switch (s)
    {
        case ScaleType::NaturalMinor:    return 3;   // A minor -> C major
        case ScaleType::MinorPentatonic: return 3;
        case ScaleType::HarmonicMinor:   return 3;
        case ScaleType::Major:           return 0;
        case ScaleType::Dorian:          return 10;  // D dorian -> C major (D + 10 = C)
        case ScaleType::Phrygian:        return 8;   // E phrygian -> C
        case ScaleType::Mixolydian:      return 5;   // G mixolydian -> C
    }
    return 0;
}
} // namespace

const std::vector<int>& intervals (ScaleType s) noexcept { return kIntervals[(size_t) s % kNumScales]; }
int degreeCount (ScaleType s) noexcept { return (int) intervals (s).size(); }

ScalePitch normalise (ScaleType s, ScalePitch p) noexcept
{
    const int n = degreeCount (s);
    return { floorMod (p.degree, n), p.octave + floorDiv (p.degree, n) };
}

int semitones (ScaleType s, ScalePitch p) noexcept
{
    const auto q = normalise (s, p);
    return intervals (s)[(size_t) q.degree] + 12 * q.octave;
}

ScalePitch step (ScaleType s, ScalePitch p, int steps) noexcept { return normalise (s, { p.degree + steps, p.octave }); }

bool inScale (int root, ScaleType s, int pc) noexcept
{
    const int rel = floorMod (pc - root, 12);
    for (int iv : intervals (s))
        if (iv == rel) return true;
    return false;
}

int degreeOfInterval (ScaleType s, int semis) noexcept
{
    const int rel = floorMod (semis, 12);
    const auto& iv = intervals (s);
    for (size_t i = 0; i < iv.size(); ++i)
        if (iv[i] == rel) return (int) i;
    return -1;
}

int fifthDegree (ScaleType s) noexcept { return degreeOfInterval (s, 7); }

int referenceRootMidi (int rootPc, Family f) noexcept
{
    const int pc = floorMod (rootPc, 12);
    if (f == Family::Sub)
        return pc >= 7 ? 24 + pc : 36 + pc; // G1(31)..F#2(42)
    return 36 + pc;                          // C2(36)..B2(47)
}

std::string rootName (int pc)
{
    static const char* names[12] = { "C", "C#", "D", "Eb", "E", "F", "F#", "G", "Ab", "A", "Bb", "B" };
    return names[floorMod (pc, 12)];
}

bool prefersFlats (int root, ScaleType s) noexcept
{
    const int parent = floorMod (root + parentMajorOffset (s), 12);
    return majorKeyUsesFlats (parent);
}

std::string noteName (int midi, int keyRoot, ScaleType s)
{
    static const char* sharps[12] = { "C", "C#", "D", "D#", "E", "F", "F#", "G", "G#", "A", "A#", "B" };
    static const char* flats[12]  = { "C", "Db", "D", "Eb", "E", "F", "Gb", "G", "Ab", "A", "Bb", "B" };
    const int pc = floorMod (midi, 12);
    const int oct = floorDiv (midi, 12) - 1; // MIDI 60 = C4
    return std::string (prefersFlats (keyRoot, s) ? flats[pc] : sharps[pc]) + std::to_string (oct);
}
} // namespace gfb::theory

#pragma once
// Pitch layer: phrase vocabularies built from scale roles and a conditioned
// transition model that depends on the rhythmic role of every onset.
// All pitches are semitone offsets from the reference root and are, by
// construction, members of the selected scale.

#include <vector>

#include "core/GeneratorSettings.h"
#include "core/RhythmCorpus.h"
#include "core/RhythmGrammar.h"
#include "core/SeedRng.h"

namespace gfb
{
enum class PitchRole : uint8_t { Root, Octave, Fifth, FifthBelow, SeventhBelow, Third, Fourth, Second, Sixth, SixthBelow, OctaveThird, Count };

struct VocabEntry
{
    PitchRole role;
    int semis; // relative to the reference root
};

struct MotifVocabulary
{
    std::vector<VocabEntry> entries; // entries[0] is always the root
    bool contains (int semis) const noexcept;
};

struct PitchContext
{
    int root = 9;
    ScaleType scale = ScaleType::NaturalMinor;
    Family family = Family::Sub;
    ContourHint contour = ContourHint::RootFifth;
};

enum class BarFunction : uint8_t { Statement, Variation, Answer, Fill, Final };

namespace pitch
{
/** Semitone value of a role in the scale, or a sentinel 999 if unavailable. */
int roleSemis (PitchRole r, ScaleType s) noexcept;

MotifVocabulary chooseVocabulary (Rng& rng, const PitchContext& ctx);

/** Assign one pitch (semis) per slot. `prevSemis` is the last pitch of the
    preceding bar (or 0) so lines connect across bar lines. */
std::vector<int> assignPitches (const std::vector<NoteSlot>& slots, const MotifVocabulary& vocab,
                                Rng& rng, const PitchContext& ctx, BarFunction fn, int prevSemis);

/** Re-pick up to `maxChanges` non-anchor pitches (bounded variation). */
void varyPitches (const std::vector<NoteSlot>& slots, std::vector<int>& pitches, const MotifVocabulary& vocab,
                  Rng& rng, const PitchContext& ctx, int maxChanges);

/** Diatonic transposition by `steps` scale steps (keeps every note in scale). */
int transposeDiatonic (int semis, int steps, ScaleType s) noexcept;

/** Make the final note of a phrase resolve to root/fifth or approach the
    root of the (looping) downbeat by step. */
void resolveEnding (const std::vector<NoteSlot>& slots, std::vector<int>& pitches, const PitchContext& ctx, Rng& rng);

/** Pitches for fill slots following a fill shape. */
std::vector<int> fillPitches (const std::vector<NoteSlot>& slots, int fromStep, FillShape shape,
                              const std::vector<int>& basePitches, const PitchContext& ctx);

/** Allowed register (relative to reference root) per family. */
int minSemis (Family f) noexcept;
int maxSemis (Family f) noexcept;
} // namespace pitch
} // namespace gfb

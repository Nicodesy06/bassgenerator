#pragma once
// Phrase layer: bar labels (A, A', B, F ...) for 1/2/4/8-bar phrases, bar
// variation helpers, and the articulation renderer that turns realised bars
// into canonical NoteEvents (swing, gate, velocity, accent, slide).

#include <string>
#include <vector>

#include "core/PatternTypes.h"
#include "core/PitchGrammar.h"
#include "core/RhythmGrammar.h"

namespace gfb
{
struct BarRealisation
{
    std::string label;          // "A", "A'", "B", "F" ...
    RhythmCell cell;
    std::vector<NoteSlot> slots;
    std::vector<int> pitches;   // semis relative to reference root, one per slot
};

enum class AnswerType : uint8_t { Transposed, Contrast, Inverted };

struct ArticulationParams
{
    Feel feel = Feel::Balanced;
    Family family = Family::Sub;
    int root = 9;
    ScaleType scale = ScaleType::NaturalMinor;
    int octave = 0;
    int swingPercent = 50;
};

namespace phrase
{
int templateCount (int bars) noexcept;
/** Bar labels of template `index` for a phrase of `bars` bars. */
std::vector<std::string> phraseTemplate (int bars, int index);

/** Keep pitches of slots whose step survived a rhythm edit, assign new ones
    with the transition model for added slots. */
std::vector<int> carryPitches (const std::vector<NoteSlot>& oldSlots, const std::vector<int>& oldPitches,
                               const std::vector<NoteSlot>& newSlots, const MotifVocabulary& vocab, Rng& rng,
                               const PitchContext& ctx);

/** Steps from the root in scale-degree space (for diatonic inversion). */
int diatonicStepsFromRoot (int semis, ScaleType s) noexcept;

/** Render realised bars to a canonical pattern (notes only; metadata left to caller).
    The whole line is folded by octaves into MIDI 0..127 so intervals and key are kept. */
Pattern renderBars (const std::vector<BarRealisation>& bars, const ArticulationParams& ap);
} // namespace phrase
} // namespace gfb

#pragma once
// Rhythm layer: parsing of corpus grids into editable one-bar cells, Feel
// profiles (density/gate/swing/syncopation targets) and bounded, musically
// constrained mutation operators.

#include <array>
#include <string>
#include <string_view>
#include <vector>

#include "core/PatternTypes.h"
#include "core/RhythmCorpus.h"
#include "core/SeedRng.h"

namespace gfb
{
enum class StepKind : uint8_t { Rest, Onset, Hold };
enum class Role : uint8_t { Normal, Anchor, Ghost, Octave, Approach };

struct Step
{
    StepKind kind = StepKind::Rest;
    uint8_t accent = 1;   // 0 ghost, 1 normal, 2 accented
    Role role = Role::Normal;
    bool slide = false;   // on onsets: this note slides into the directly following onset
    bool operator== (const Step&) const = default;
};

/** One 4/4 bar of rhythm on the 16th grid. */
struct RhythmCell
{
    std::array<Step, kStepsPerBar> steps {};
    bool operator== (const RhythmCell&) const = default;
};

/** A note of a cell, derived from onset + holds. */
struct NoteSlot
{
    int step = 0;        // 0..15
    int length = 1;      // in steps (>= 1)
    uint8_t accent = 1;
    Role role = Role::Normal;
    bool slide = false;  // only true when the next onset starts exactly at step+length
};

enum class Mutation : uint8_t
{
    RemoveOnset, AddOnset, SplitNote, MergeNotes, Anticipate, DelayEntry,
    AccentShift, GhostNote, OctavePunct, Pickup, SlideToggle, Extend, Count
};
constexpr int kNumMutations = (int) Mutation::Count;
std::string_view toString (Mutation) noexcept;

struct FeelProfile
{
    Feel feel;
    int minOnsets, maxOnsets, targetOnsets; // per bar
    int gateShort, gateLong;                // permille of slot length used as duration
    int defaultSwing;                       // percent (50 = straight)
    int ghostPermille;                      // chance a newly added onset is a ghost
    int syncLo, syncHi;                     // syncopation index target, permille
    int offbeatLo, offbeatHi;               // off-8th onset ratio target, permille
    int maxRestSteps;                       // longest internal rest tolerated (in steps)
    int maxStyleMutations;                  // mutations applied to the core motif
    std::array<int, kNumMutations> mutationWeights;
};

const FeelProfile& feelProfile (Feel f) noexcept;

namespace rhythm
{
RhythmCell parseGrid (std::string_view grid16);
/** Replace the last grid.size() steps (4 or 8) of `cell` with a fill grid. */
void applyFill (RhythmCell& cell, std::string_view fillGrid);
std::string toGrid (const RhythmCell& cell);
int onsetCount (const RhythmCell& cell) noexcept;
std::vector<NoteSlot> slots (const RhythmCell& cell);
/** Longest run of rest steps inside the bar (not wrapping). */
int longestRest (const RhythmCell& cell) noexcept;
/** Jaccard similarity of the onset sets of two cells (1 = identical onsets). */
double onsetSimilarity (const RhythmCell& a, const RhythmCell& b) noexcept;

/** Apply one mutation with hard musical bounds. Returns false (cell
    untouched) when the operation is not applicable or would violate the
    feel's density / style rules. */
bool mutate (RhythmCell& cell, Mutation m, Rng& rng, const FeelProfile& fp, Family fam);

/** Pick a mutation according to the feel's weights (family-adjusted). */
Mutation pickMutation (Rng& rng, const FeelProfile& fp, Family fam);
} // namespace rhythm
} // namespace gfb

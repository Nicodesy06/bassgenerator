#pragma once
// Canonical musical data model shared by generator, UI, synth scheduler and
// MIDI export. All time values are integer ticks (PPQ 960) to avoid floating
// point equality issues.

#include <cstdint>
#include <string>
#include <vector>

#include "core/GeneratorSettings.h"

namespace gfb
{
constexpr int kPPQ               = 960;
constexpr int kTicksPerSixteenth = kPPQ / 4;           // 240
constexpr int kStepsPerBar       = 16;                 // 4/4, 16th grid
constexpr int kTicksPerBar       = kPPQ * 4;           // 3840
constexpr uint32_t kAlgorithmVersion = 2;
constexpr int kMaxNotesPerPattern    = 512;
/** A note flagged slideToNext may overlap the following (different-pitch)
    note by at most this many ticks: this is how legato/glide is encoded in
    exported MIDI. All other notes are strictly non-overlapping (mono line). */
constexpr int kMaxSlideOverlapTicks  = 60;
constexpr int kSlideOverlapTicks     = 30;
constexpr int kMinNoteDurationTicks  = 30;

struct NoteEvent
{
    int64_t startTick     = 0;   // 0-based musical ticks from pattern start
    int64_t durationTicks = 240; // strictly positive
    uint8_t midiNote      = 36;  // 0..127
    uint8_t velocity      = 100; // 1..127
    bool accent           = false;
    bool slideToNext      = false;

    int64_t endTick() const noexcept { return startTick + durationTicks; }
    bool operator== (const NoteEvent&) const = default;
};

struct Pattern
{
    uint32_t algorithmVersion = kAlgorithmVersion;
    uint32_t ticksPerQuarter  = kPPQ;
    int bars = 1;
    std::vector<NoteEvent> notes;

    // ---- metadata (not part of the canonical note hash) -------------------
    GeneratorSettings settings;          // settings that produced this pattern
    Feel resolvedFeel = Feel::Balanced;  // concrete feel when settings.feel == Random
    int archetypeId = -1;                // primary rhythm archetype
    int answerArchetypeId = -1;          // archetype used for B sections (or -1)
    std::string phrasePlan;              // e.g. "A A' B F"
    int swingPercent = 50;               // effective swing used for rendering
    double tempoBpm = 124.0;             // only used for SMF tempo meta / preview
    uint64_t hash = 0;                   // computePatternHash(*this)

    int64_t lengthTicks() const noexcept { return (int64_t) bars * kTicksPerBar; }
    bool empty() const noexcept { return notes.empty(); }
};

/** Stable 64-bit FNV-1a hash over algorithmVersion, PPQ, bars and the note list. */
uint64_t computePatternHash (const Pattern& p);

/** Returns an empty string when all invariants hold, otherwise a description
    of the first violation. Invariants:
    - bars in {1,2,4,8}, ppq == 960, note count <= kMaxNotesPerPattern
    - notes sorted by start, unique starts (monophonic line)
    - start >= 0, duration > 0, end <= pattern length
    - midi note 0..127, velocity 1..127
    - no overlap, except slideToNext notes may overlap the next note by
      <= kMaxSlideOverlapTicks when the next note has a different pitch
    - the last note never has slideToNext (no hanging glide on loop wrap) */
std::string validatePattern (const Pattern& p);

/** Sort + clamp + resolve overlaps so validatePattern() succeeds. Used as a
    final safety net; generator output is expected to already be valid. */
void normalisePattern (Pattern& p);

bool isValidBars (int bars) noexcept;
} // namespace gfb

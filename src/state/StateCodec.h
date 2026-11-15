#pragma once
// Per-instance project state (stored in the VST3 chunk next to the APVTS
// parameters). Schema-versioned ValueTree; decoding is tolerant: unknown or
// corrupt fields fall back to defaults instead of failing the whole load, and
// stored note snapshots are restored exactly (never regenerated on load).

#include <deque>
#include <optional>

#include <juce_data_structures/juce_data_structures.h>

#include "core/PatternTypes.h"

namespace gfb::state
{
constexpr int kSchemaVersion = 1;
inline const juce::Identifier kRootId { "GrooveForgeState" };

struct InstanceState
{
    GeneratorSettings settings;
    LockSet locks;
    Pattern pattern;                 // exact current notes
    std::deque<Pattern> history;     // up to 64 exact snapshots
    int historyCursor = -1;
    int presetIndex = 1;
    bool hostSync = true;
    bool midiOut = false;
    double previewBpm = 124.0;
    int editorWidth = 1220, editorHeight = 780;
    bool drawerOpen = false;
    bool useTaste = true;            // apply the local LIKE/DISLIKE profile when generating
};

juce::ValueTree encode (const InstanceState& s);
/** Returns nullopt if the tree is not a GrooveForge state at all or is from a
    newer, unknown major schema; otherwise a repaired state. */
std::optional<InstanceState> decode (const juce::ValueTree& tree);
} // namespace gfb::state

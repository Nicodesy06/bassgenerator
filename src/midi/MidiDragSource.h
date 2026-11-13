#pragma once
// MIDI file creation for "Export MIDI..." and for native external drag &
// drop out of the plugin window (JUCE performExternalDragDropOfFiles).
// Files for dragging are written *before* the drag starts into a per-user
// cache folder and are never deleted during a drag; cleanup only removes our
// own .mid files older than 24 h at startup.

#include <juce_core/juce_core.h>

#include "core/PatternTypes.h"

namespace gfb::mididrag
{
juce::File dragCacheDirectory();
/** Write the pattern to `target` (overwrites). Returns an error result with a user-readable message on failure. */
juce::Result exportTo (const Pattern& p, double bpm, const juce::File& target);
/** Write a uniquely named file in the cache directory for dragging. */
juce::Result writeDragFile (const Pattern& p, double bpm, juce::File& outFile);
/** Delete GrooveForge .mid files older than maxAgeHours in the cache directory. Returns count deleted. */
int cleanupDragCache (double maxAgeHours = 24.0);
} // namespace gfb::mididrag

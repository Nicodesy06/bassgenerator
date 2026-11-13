#pragma once
// Compact, versioned text encodings used by project state and favorites.
// Notes: "start:duration:note:velocity:flags;" (flags bit0 accent, bit1 slide)
// Settings: "key=value;..." with stable keys.

#include <map>
#include <optional>
#include <string>
#include <string_view>
#include <vector>

#include "core/PatternTypes.h"

namespace gfb::serial
{
std::string notesToString (const std::vector<NoteEvent>& notes);
/** Strict parse; returns nullopt on any malformed token or out-of-range value. */
std::optional<std::vector<NoteEvent>> notesFromString (std::string_view text);

std::map<std::string, std::string> settingsToMap (const GeneratorSettings& s);
/** Missing/invalid keys fall back to `defaults` field-by-field. */
GeneratorSettings settingsFromMap (const std::map<std::string, std::string>& kv, const GeneratorSettings& defaults = {});

std::map<std::string, std::string> locksToMap (const LockSet& l);
LockSet locksFromMap (const std::map<std::string, std::string>& kv, const LockSet& defaults = {});

std::string mapToString (const std::map<std::string, std::string>& kv);
std::map<std::string, std::string> mapFromString (std::string_view s);

/** Full pattern (header + notes) as a single string, used for history/favorites. */
std::string patternToString (const Pattern& p);
std::optional<Pattern> patternFromString (std::string_view s);
} // namespace gfb::serial

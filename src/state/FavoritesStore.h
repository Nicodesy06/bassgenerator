#pragma once
// Global favorites shared by all plugin instances and sessions.
//
// File: <user app data>/GrooveForgeBass/favorites.json  (Windows: %APPDATA%\GrooveForgeBass)
// Format: {"schema":1,"app":"GrooveForge Bass","favorites":[{"id","name","created","pattern"}]}
// Concurrency: every mutation is read-modify-write under an InterProcessLock
// (other processes) plus a process-wide mutex (other instances in the same
// host process), so concurrent instances merge instead of overwriting.
// Atomicity: write to a temp sibling, flush, then replace the target; the
// previous good file is kept as favorites.bak.json.
// Corruption: an unreadable file is moved aside (favorites.corrupt-<ms>.json)
// and the backup is used if valid; nothing is silently overwritten.
// Duplicates: adding a pattern whose notes+settings already exist returns the
// existing entry instead of creating a copy.
// Never called from the audio thread.

#include <functional>
#include <mutex>
#include <optional>
#include <vector>

#include <juce_core/juce_core.h>

#include "core/PatternTypes.h"

namespace gfb
{
struct Favorite
{
    juce::String id;
    juce::String name;
    juce::int64 createdMs = 0;
    Pattern pattern;
};

class FavoritesStore
{
public:
    static constexpr int kSchemaVersion = 1;
    static constexpr int kMaxFavorites = 2000;

    explicit FavoritesStore (juce::File file = defaultFile());
    static juce::File defaultFile();

    /** Re-read the file if it changed on disk. Returns true if the list changed. */
    bool reloadIfChanged();
    void reload();
    std::vector<Favorite> list() const;
    const juce::File& getFile() const noexcept { return file; }
    juce::String lastError() const;

    /** Adds (or finds) a favorite; returns its id or an empty optional on I/O error. */
    std::optional<juce::String> add (const Pattern& p, const juce::String& name);
    bool remove (const juce::String& id);
    bool rename (const juce::String& id, const juce::String& newName);

private:
    bool mutate (const std::function<bool (std::vector<Favorite>&)>& fn);
    std::vector<Favorite> readFileLocked (bool& ok);
    bool writeFileLocked (const std::vector<Favorite>& favs);

    juce::File file;
    std::vector<Favorite> cache;
    juce::Time cachedModTime;
    juce::int64 cachedSize = -1;
    juce::String error;
    mutable std::mutex cacheMutex;
};
} // namespace gfb

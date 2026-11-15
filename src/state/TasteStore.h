#pragma once
// Persistent local taste profile (LIKE / DISLIKE feedback), shared by every
// GrooveForge instance on this computer and updated after each rating.
//
// File: <user app data>/GrooveForgeBass/taste.json  (Windows: %APPDATA%\GrooveForgeBass)
// {"schema":1, "profile":"<TasteProfile string>", "liked":["<pattern>",...], "rated":["<hash>",...]}
// Same safety model as FavoritesStore: read-modify-write under an
// InterProcessLock + process mutex, temp file + replace, backup, corrupt file
// moved aside. Never touched by the audio thread.

#include <functional>
#include <mutex>
#include <vector>

#include <juce_core/juce_core.h>

#include "core/TasteProfile.h"

namespace gfb
{
class TasteStore
{
public:
    static constexpr int kSchemaVersion = 1;
    static constexpr int kMaxLiked = 1000;
    static constexpr int kMaxRated = 4000;

    explicit TasteStore (juce::File file = defaultFile());
    static juce::File defaultFile();

    TasteProfile profile() const;
    std::vector<Pattern> likedPatterns() const;
    int ratings() const;
    bool alreadyRated (const Pattern& p) const;

    /** Records the rating (ignored if this exact pattern was already rated). Returns false on I/O error. */
    bool rate (const Pattern& p, bool liked);
    bool reset();
    bool reloadIfChanged();
    void reload();
    const juce::File& getFile() const noexcept { return file; }
    juce::String lastError() const;

private:
    struct Data
    {
        TasteProfile profile;
        std::vector<Pattern> liked;
        std::vector<uint64_t> rated;
    };
    bool mutate (const std::function<bool (Data&)>& fn);
    Data readLocked();
    bool writeLocked (const Data& d);

    juce::File file;
    Data cache;
    juce::Time cachedModTime;
    juce::int64 cachedSize = -1;
    juce::String error;
    mutable std::mutex cacheMutex;
};
} // namespace gfb

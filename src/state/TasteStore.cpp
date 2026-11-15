#include "state/TasteStore.h"

#include <algorithm>

#include "core/Serialization.h"

namespace gfb
{
namespace
{
std::mutex& tasteMutex()
{
    static std::mutex m;
    return m;
}

struct ScopedTasteLock
{
    explicit ScopedTasteLock (const juce::File& f)
        : ipl ("GrooveForgeBassTaste_" + juce::String::toHexString (f.getFullPathName().hashCode64())), local (tasteMutex())
    {
        ok = ipl.enter (5000);
    }
    ~ScopedTasteLock() { if (ok) ipl.exit(); }
    juce::InterProcessLock ipl;
    std::lock_guard<std::mutex> local;
    bool ok = false;
};
} // namespace

juce::File TasteStore::defaultFile()
{
    return juce::File::getSpecialLocation (juce::File::userApplicationDataDirectory).getChildFile ("GrooveForgeBass").getChildFile ("taste.json");
}

TasteStore::TasteStore (juce::File f) : file (std::move (f)) { reload(); }

juce::String TasteStore::lastError() const { std::lock_guard<std::mutex> g (cacheMutex); return error; }

TasteStore::Data TasteStore::readLocked()
{
    Data d;
    if (! file.existsAsFile()) return d;
    juce::var root;
    const bool parsed = juce::JSON::parse (file.loadFileAsString(), root).wasOk() && root.isObject()
                     && (int) root.getProperty ("schema", 0) == kSchemaVersion;
    std::optional<TasteProfile> prof;
    if (parsed) prof = taste::fromString (root.getProperty ("profile", "").toString().toStdString());
    if (! prof)
    {
        const auto aside = file.getSiblingFile ("taste.corrupt-" + juce::String (juce::Time::currentTimeMillis()) + ".json");
        file.moveFileTo (aside);
        error = "taste.json was unreadable and was moved to " + aside.getFileName() + "; starting a new taste profile";
        return d;
    }
    d.profile = *prof;
    if (auto* liked = root.getProperty ("liked", juce::var()).getArray())
        for (const auto& v : *liked)
            if (auto p = serial::patternFromString (v.toString().toStdString())) d.liked.push_back (*p);
    if (auto* rated = root.getProperty ("rated", juce::var()).getArray())
        for (const auto& v : *rated) d.rated.push_back ((uint64_t) v.toString().getHexValue64());
    return d;
}

bool TasteStore::writeLocked (const Data& d)
{
    if (! file.getParentDirectory().createDirectory()) { error = "cannot create settings folder"; return false; }
    auto* root = new juce::DynamicObject();
    root->setProperty ("schema", kSchemaVersion);
    root->setProperty ("app", "GrooveForge Bass");
    root->setProperty ("profile", juce::String (taste::toString (d.profile)));
    juce::Array<juce::var> liked, rated;
    for (const auto& p : d.liked) liked.add (juce::String (serial::patternToString (p)));
    for (auto h : d.rated) rated.add (juce::String::toHexString ((juce::int64) h));
    root->setProperty ("liked", liked);
    root->setProperty ("rated", rated);
    const auto tmp = file.getSiblingFile ("taste.json.tmp-" + juce::String::toHexString (juce::Random::getSystemRandom().nextInt64()));
    if (! tmp.replaceWithText (juce::JSON::toString (juce::var (root)))) { error = "cannot write taste file"; return false; }
    if (file.existsAsFile()) file.copyFileTo (file.getSiblingFile ("taste.bak.json"));
    if (! tmp.replaceFileIn (file) && ! tmp.moveFileTo (file)) { tmp.deleteFile(); error = "cannot replace taste file"; return false; }
    return true;
}

void TasteStore::reload()
{
    ScopedTasteLock lock (file);
    auto d = readLocked();
    std::lock_guard<std::mutex> g (cacheMutex);
    cache = std::move (d);
    cachedModTime = file.getLastModificationTime();
    cachedSize = file.getSize();
}

bool TasteStore::reloadIfChanged()
{
    {
        std::lock_guard<std::mutex> g (cacheMutex);
        if (file.getLastModificationTime() == cachedModTime && file.getSize() == cachedSize) return false;
    }
    reload();
    return true;
}

bool TasteStore::mutate (const std::function<bool (Data&)>& fn)
{
    ScopedTasteLock lock (file);
    if (! lock.ok) { std::lock_guard<std::mutex> g (cacheMutex); error = "taste file is locked by another process"; return false; }
    auto d = readLocked(); // merge with what other instances wrote
    if (! fn (d)) { std::lock_guard<std::mutex> g (cacheMutex); cache = d; return true; }
    const bool ok = writeLocked (d);
    std::lock_guard<std::mutex> g (cacheMutex);
    cache = d;
    cachedModTime = file.getLastModificationTime();
    cachedSize = file.getSize();
    return ok;
}

TasteProfile TasteStore::profile() const { std::lock_guard<std::mutex> g (cacheMutex); return cache.profile; }
std::vector<Pattern> TasteStore::likedPatterns() const { std::lock_guard<std::mutex> g (cacheMutex); return cache.liked; }
int TasteStore::ratings() const { std::lock_guard<std::mutex> g (cacheMutex); return cache.profile.likes + cache.profile.dislikes; }

bool TasteStore::alreadyRated (const Pattern& p) const
{
    std::lock_guard<std::mutex> g (cacheMutex);
    return std::find (cache.rated.begin(), cache.rated.end(), p.hash) != cache.rated.end();
}

bool TasteStore::rate (const Pattern& p, bool liked)
{
    return mutate ([&] (Data& d) {
        if (std::find (d.rated.begin(), d.rated.end(), p.hash) != d.rated.end()) return false;
        taste::addFeedback (d.profile, p, liked);
        d.rated.push_back (p.hash);
        if ((int) d.rated.size() > kMaxRated) d.rated.erase (d.rated.begin());
        if (liked)
        {
            d.liked.push_back (p);
            if ((int) d.liked.size() > kMaxLiked) d.liked.erase (d.liked.begin());
        }
        return true;
    });
}

bool TasteStore::reset()
{
    return mutate ([] (Data& d) { d = Data {}; return true; });
}
} // namespace gfb

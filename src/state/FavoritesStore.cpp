#include "state/FavoritesStore.h"

#include <algorithm>
#include <mutex>

#include "core/Serialization.h"

namespace gfb
{
namespace
{
std::mutex& processWideMutex()
{
    static std::mutex m; // one per process: instances in the same host share it
    return m;
}

struct ScopedFileLock
{
    explicit ScopedFileLock (const juce::File& f)
        : ipl ("GrooveForgeBass_" + juce::String::toHexString (f.getFullPathName().hashCode64())), local (processWideMutex())
    {
        ok = ipl.enter (5000);
    }
    ~ScopedFileLock() { if (ok) ipl.exit(); }
    juce::InterProcessLock ipl;
    std::lock_guard<std::mutex> local;
    bool ok = false;
};

std::optional<std::vector<Favorite>> parse (const juce::String& text)
{
    juce::var root;
    if (juce::JSON::parse (text, root).failed() || ! root.isObject()) return std::nullopt;
    const int schema = (int) root.getProperty ("schema", 0);
    if (schema < 1 || schema > FavoritesStore::kSchemaVersion) return std::nullopt;
    const auto* arr = root.getProperty ("favorites", juce::var()).getArray();
    if (arr == nullptr) return std::nullopt;
    std::vector<Favorite> out;
    for (const auto& v : *arr)
    {
        Favorite f;
        f.id = v.getProperty ("id", "").toString();
        f.name = v.getProperty ("name", "").toString().substring (0, 80);
        f.createdMs = (juce::int64) v.getProperty ("created", 0);
        auto p = serial::patternFromString (v.getProperty ("pattern", "").toString().toStdString());
        if (f.id.isEmpty() || ! p) continue; // skip a damaged entry, keep the rest
        f.pattern = *p;
        out.push_back (std::move (f));
        if ((int) out.size() >= FavoritesStore::kMaxFavorites) break;
    }
    return out;
}

juce::String serialise (const std::vector<Favorite>& favs)
{
    juce::Array<juce::var> arr;
    for (const auto& f : favs)
    {
        auto* o = new juce::DynamicObject();
        o->setProperty ("id", f.id);
        o->setProperty ("name", f.name);
        o->setProperty ("created", f.createdMs);
        o->setProperty ("pattern", juce::String (serial::patternToString (f.pattern)));
        arr.add (juce::var (o));
    }
    auto* root = new juce::DynamicObject();
    root->setProperty ("schema", FavoritesStore::kSchemaVersion);
    root->setProperty ("app", "GrooveForge Bass");
    root->setProperty ("favorites", arr);
    return juce::JSON::toString (juce::var (root));
}
} // namespace

juce::File FavoritesStore::defaultFile()
{
    return juce::File::getSpecialLocation (juce::File::userApplicationDataDirectory).getChildFile ("GrooveForgeBass").getChildFile ("favorites.json");
}

FavoritesStore::FavoritesStore (juce::File f) : file (std::move (f)) { reload(); }

juce::String FavoritesStore::lastError() const { std::lock_guard<std::mutex> g (cacheMutex); return error; }

std::vector<Favorite> FavoritesStore::readFileLocked (bool& ok)
{
    ok = true;
    if (! file.existsAsFile()) return {};
    if (auto favs = parse (file.loadFileAsString())) return *favs;
    // corrupt: move aside, try the backup
    ok = false;
    const auto aside = file.getSiblingFile ("favorites.corrupt-" + juce::String (juce::Time::currentTimeMillis()) + ".json");
    file.moveFileTo (aside);
    const auto bak = file.getSiblingFile ("favorites.bak.json");
    if (bak.existsAsFile())
        if (auto favs = parse (bak.loadFileAsString())) { error = "favorites.json was corrupt; restored from backup"; return *favs; }
    error = "favorites.json was corrupt and was moved to " + aside.getFileName();
    return {};
}

bool FavoritesStore::writeFileLocked (const std::vector<Favorite>& favs)
{
    if (! file.getParentDirectory().createDirectory()) { error = "cannot create favorites folder"; return false; }
    const auto tmp = file.getSiblingFile ("favorites.json.tmp-" + juce::String::toHexString (juce::Random::getSystemRandom().nextInt64()));
    {
        juce::FileOutputStream out (tmp);
        if (! out.openedOk()) { error = "cannot write favorites temp file"; return false; }
        out.setPosition (0);
        out.truncate();
        out.writeText (serialise (favs), false, false, "\n");
        out.flush();
        if (out.getStatus().failed()) { error = out.getStatus().getErrorMessage(); tmp.deleteFile(); return false; }
    }
    // keep a backup of the last good file
    if (file.existsAsFile() && parse (file.loadFileAsString())) file.copyFileTo (file.getSiblingFile ("favorites.bak.json"));
    if (! tmp.replaceFileIn (file) && ! tmp.moveFileTo (file)) { error = "cannot replace favorites file"; tmp.deleteFile(); return false; }
    return true;
}

void FavoritesStore::reload()
{
    ScopedFileLock lock (file);
    bool ok;
    auto favs = readFileLocked (ok);
    std::lock_guard<std::mutex> g (cacheMutex);
    cache = std::move (favs);
    cachedModTime = file.getLastModificationTime();
    cachedSize = file.getSize();
}

bool FavoritesStore::reloadIfChanged()
{
    {
        std::lock_guard<std::mutex> g (cacheMutex);
        if (file.getLastModificationTime() == cachedModTime && file.getSize() == cachedSize) return false;
    }
    reload();
    return true;
}

std::vector<Favorite> FavoritesStore::list() const
{
    std::lock_guard<std::mutex> g (cacheMutex);
    return cache;
}

bool FavoritesStore::mutate (const std::function<bool (std::vector<Favorite>&)>& fn)
{
    ScopedFileLock lock (file);
    if (! lock.ok) { std::lock_guard<std::mutex> g (cacheMutex); error = "favorites file is locked by another process"; return false; }
    bool ok;
    auto favs = readFileLocked (ok); // always merge with the latest on-disk content
    if (! fn (favs)) { std::lock_guard<std::mutex> g (cacheMutex); cache = favs; return false; }
    const bool written = writeFileLocked (favs);
    std::lock_guard<std::mutex> g (cacheMutex);
    cache = favs;
    cachedModTime = file.getLastModificationTime();
    cachedSize = file.getSize();
    return written;
}

std::optional<juce::String> FavoritesStore::add (const Pattern& p, const juce::String& name)
{
    juce::String resultId;
    bool duplicate = false;
    const bool ok = mutate ([&] (std::vector<Favorite>& favs) {
        for (const auto& f : favs)
            if (f.pattern.hash == p.hash && f.pattern.notes == p.notes && f.pattern.settings == p.settings) { resultId = f.id; duplicate = true; return false; }
        if ((int) favs.size() >= kMaxFavorites) return false;
        Favorite f;
        f.id = juce::Uuid().toString();
        f.name = name.isNotEmpty() ? name.substring (0, 80) : juce::String ("Favorite ") + juce::String ((int) favs.size() + 1);
        f.createdMs = juce::Time::currentTimeMillis();
        f.pattern = p;
        resultId = f.id;
        favs.push_back (f);
        return true;
    });
    if (duplicate || ok) return resultId;
    return std::nullopt;
}

bool FavoritesStore::remove (const juce::String& id)
{
    return mutate ([&] (std::vector<Favorite>& favs) {
        const auto before = favs.size();
        favs.erase (std::remove_if (favs.begin(), favs.end(), [&] (const Favorite& f) { return f.id == id; }), favs.end());
        return favs.size() != before;
    });
}

bool FavoritesStore::rename (const juce::String& id, const juce::String& newName)
{
    return mutate ([&] (std::vector<Favorite>& favs) {
        for (auto& f : favs)
            if (f.id == id) { f.name = newName.trim().substring (0, 80); return true; }
        return false;
    });
}
} // namespace gfb

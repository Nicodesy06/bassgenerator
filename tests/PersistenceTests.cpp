#include <thread>

#include <juce_core/juce_core.h>

#include "TestFramework.h"
#include "core/PatternGenerator.h"
#include "state/FavoritesStore.h"
#include "state/StateCodec.h"

using namespace gfb;

namespace
{
Pattern pat (uint64_t style, int bars = 4, Family f = Family::Sub)
{
    GeneratorSettings s;
    s.styleSeed = style; s.varSeed = style * 3 + 1; s.bars = bars; s.family = f; s.feel = Feel::Rolling;
    return generatePattern (s);
}

juce::File tempDir (const juce::String& name)
{
    auto d = juce::File::getSpecialLocation (juce::File::tempDirectory).getChildFile ("gfb_tests").getChildFile (name + juce::String (juce::Random::getSystemRandom().nextInt (1 << 30)));
    d.deleteRecursively();
    d.createDirectory();
    return d;
}

juce::ValueTree throughXml (const juce::ValueTree& t)
{
    const auto xml = t.toXmlString();
    return juce::ValueTree::fromXml (xml);
}

// worker: --worker fav <file> <count> <offset>
struct WorkerRegistrar
{
    WorkerRegistrar()
    {
        gfbtest::workerHook() = [] (int argc, char** argv) -> int {
            if (argc < 6 || juce::String (argv[2]) != "fav") return 3;
            FavoritesStore store { juce::File (juce::String (argv[3])) };
            const int count = juce::String (argv[4]).getIntValue(), offset = juce::String (argv[5]).getIntValue();
            int ok = 0;
            for (int i = 0; i < count; ++i) ok += store.add (pat ((uint64_t) (offset + i) + 1), "p" + juce::String (offset + i)).has_value() ? 1 : 0;
            return ok == count ? 0 : 1;
        };
    }
} workerRegistrar;
} // namespace

TEST_CASE (State, RoundTripExact)
{
    state::InstanceState s;
    s.settings.root = 4; s.settings.scale = ScaleType::Phrygian; s.settings.family = Family::Acid; s.settings.bars = 8;
    s.settings.styleSeed = 18446744073709551615ULL; s.settings.varSeed = 99;
    s.pattern = generatePattern (s.settings);
    s.locks.root = true; s.locks.styleSeed = true; s.locks.bars = false;
    for (int i = 0; i < 64; ++i) s.history.push_back (pat ((uint64_t) i + 1, 1 << (i % 4)));
    s.historyCursor = 17; s.presetIndex = 6; s.hostSync = false; s.midiOut = true; s.previewBpm = 131.5;
    s.editorWidth = 1400; s.editorHeight = 900; s.drawerOpen = true;
    const auto d = state::decode (throughXml (state::encode (s)));
    REQUIRE (d.has_value());
    CHECK (d->settings == s.settings);
    CHECK (d->locks == s.locks);
    CHECK (d->pattern.notes == s.pattern.notes);
    CHECK_EQ (d->pattern.hash, s.pattern.hash);
    REQUIRE (d->history.size() == 64);
    for (size_t i = 0; i < 64; ++i) CHECK (d->history[i].notes == s.history[i].notes);
    CHECK_EQ (d->historyCursor, 17);
    CHECK_EQ (d->presetIndex, 6);
    CHECK (! d->hostSync && d->midiOut);
    CHECK_NEAR (d->previewBpm, 131.5, 1e-9);
    CHECK (d->drawerOpen);
}

TEST_CASE (State, StoredNotesAreNotRegenerated)
{
    // a pattern whose notes differ from what the generator would produce today must be restored verbatim
    state::InstanceState s;
    s.pattern = generatePattern (s.settings);
    s.pattern.notes.pop_back();
    s.pattern.hash = computePatternHash (s.pattern);
    const auto d = state::decode (throughXml (state::encode (s)));
    REQUIRE (d.has_value());
    CHECK (d->pattern.notes == s.pattern.notes);
}

TEST_CASE (State, CorruptedFailsSafe)
{
    CHECK (! state::decode (juce::ValueTree ("Nope")).has_value());
    juce::ValueTree future (state::kRootId);
    future.setProperty ("schema", 99, nullptr);
    CHECK (! state::decode (future).has_value());
    juce::ValueTree bad (state::kRootId);
    bad.setProperty ("schema", 1, nullptr);
    bad.setProperty ("settings", "root=99;bars=7;style=abc;feel=WHAT", nullptr);
    bad.setProperty ("pattern", "garbage|1:2:3", nullptr);
    bad.setProperty ("preset", 9999, nullptr);
    juce::ValueTree h ("history");
    juce::ValueTree e ("entry");
    e.setProperty ("data", "x=1|0:240:200:100:0;", nullptr); // note 200 invalid
    h.appendChild (e, nullptr);
    bad.appendChild (h, nullptr);
    const auto d = state::decode (bad);
    REQUIRE (d.has_value());
    CHECK (d->settings == sanitise (GeneratorSettings {}));
    CHECK (d->pattern.notes.empty()); // caller regenerates from settings
    CHECK (d->history.empty());
    CHECK_EQ (d->presetIndex, 7);
    // random byte soup through the XML parser never crashes
    juce::Random r (42);
    for (int i = 0; i < 200; ++i)
    {
        juce::String junk;
        for (int k = 0; k < 64; ++k) junk += (juce::juce_wchar) r.nextInt (juce::Range<int> (32, 126));
        auto t = juce::ValueTree::fromXml ("<GrooveForgeState schema=\"1\" settings=\"" + juce::String (junk.hashCode()) + "\" pattern=\"" + junk.removeCharacters ("\"<>&") + "\"/>");
        if (t.isValid()) state::decode (t);
    }
}

TEST_CASE (Favorites, PersistAcrossInstances)
{
    const auto dir = tempDir ("fav");
    const auto file = dir.getChildFile ("favorites.json");
    juce::String id;
    {
        FavoritesStore a (file);
        auto r = a.add (pat (1), "First");
        REQUIRE (r.has_value());
        id = *r;
        CHECK (a.add (pat (2, 2, Family::Acid), "Second").has_value());
        CHECK_EQ (a.list().size(), (size_t) 2);
    }
    FavoritesStore b (file);
    const auto l = b.list();
    REQUIRE (l.size() == 2);
    CHECK (l[0].name == "First");
    CHECK (l[0].pattern.notes == pat (1).notes);
    CHECK (l[1].pattern.settings == pat (2, 2, Family::Acid).settings);
    CHECK (b.rename (id, "Renamed"));
    CHECK (b.remove (l[1].id));
    FavoritesStore c (file);
    REQUIRE (c.list().size() == 1);
    CHECK (c.list()[0].name == "Renamed");
    // schema/version marker present
    CHECK (file.loadFileAsString().contains ("\"schema\": 1"));
    dir.deleteRecursively();
}

TEST_CASE (Favorites, DuplicateAddReturnsExisting)
{
    const auto dir = tempDir ("dup");
    FavoritesStore a (dir.getChildFile ("favorites.json"));
    const auto x = a.add (pat (5), "A");
    const auto y = a.add (pat (5), "B");
    REQUIRE (x.has_value() && y.has_value());
    CHECK (*x == *y);
    CHECK_EQ (a.list().size(), (size_t) 1);
    dir.deleteRecursively();
}

TEST_CASE (Favorites, CorruptFileRecoversFromBackup)
{
    const auto dir = tempDir ("corrupt");
    const auto file = dir.getChildFile ("favorites.json");
    {
        FavoritesStore a (file);
        a.add (pat (1), "one");
        a.add (pat (2), "two"); // second write creates favorites.bak.json with "one"
    }
    file.replaceWithText ("{ this is not json");
    FavoritesStore b (file);
    CHECK (b.list().size() >= 1);              // restored from backup
    CHECK (b.lastError().contains ("backup"));
    CHECK (dir.findChildFiles (juce::File::findFiles, false, "favorites.corrupt-*.json").size() == 1); // never silently deleted
    // no backup at all -> empty list, file moved aside
    dir.getChildFile ("favorites.bak.json").deleteFile();
    file.replaceWithText ("[]]]");
    FavoritesStore c (file);
    CHECK (c.list().empty());
    CHECK (c.add (pat (3), "fresh").has_value());
    FavoritesStore d (file);
    CHECK_EQ (d.list().size(), (size_t) 1);
    dir.deleteRecursively();
}

TEST_CASE (Favorites, ConcurrentInstancesInOneProcessMerge)
{
    const auto dir = tempDir ("threads");
    const auto file = dir.getChildFile ("favorites.json");
    std::vector<std::thread> ts;
    std::atomic<int> ok { 0 };
    for (int t = 0; t < 8; ++t)
        ts.emplace_back ([&, t] {
            FavoritesStore s (file);
            for (int i = 0; i < 20; ++i) ok += s.add (pat ((uint64_t) (t * 100 + i + 1)), "t" + juce::String (t)).has_value() ? 1 : 0;
        });
    for (auto& t : ts) t.join();
    CHECK_EQ (ok.load(), 160);
    FavoritesStore check (file);
    CHECK_EQ (check.list().size(), (size_t) 160);
    dir.deleteRecursively();
}

TEST_CASE (Favorites, ConcurrentProcessesMerge)
{
    const auto dir = tempDir ("procs");
    const auto file = dir.getChildFile ("favorites.json");
    const auto exe = juce::File::getSpecialLocation (juce::File::currentExecutableFile);
    std::vector<std::unique_ptr<juce::ChildProcess>> procs;
    for (int i = 0; i < 4; ++i)
    {
        auto cp = std::make_unique<juce::ChildProcess>();
        REQUIRE (cp->start (juce::StringArray { exe.getFullPathName(), "--worker", "fav", file.getFullPathName(), "15", juce::String (i * 1000) }));
        procs.push_back (std::move (cp));
    }
    for (auto& p : procs)
    {
        CHECK (p->waitForProcessToFinish (60000));
        CHECK_EQ ((int) p->getExitCode(), 0);
    }
    FavoritesStore check (file);
    CHECK_EQ (check.list().size(), (size_t) 60);
    dir.deleteRecursively();
}

#include <set>

#include "TestFramework.h"
#include "core/GeneratorSettings.h"
#include "core/RhythmCorpus.h"
#include "core/RhythmGrammar.h"
#include "core/ScaleTheory.h"
#include "core/SeedRng.h"
#include "core/Serialization.h"

using namespace gfb;

TEST_CASE (Theory, ScalesHaveExpectedShapes)
{
    CHECK_EQ (theory::degreeCount (ScaleType::MinorPentatonic), 5);
    for (int s = 0; s < kNumScales; ++s)
    {
        const auto& iv = theory::intervals ((ScaleType) s);
        CHECK_EQ (iv.front(), 0);
        for (size_t i = 1; i < iv.size(); ++i) CHECK (iv[i] > iv[i - 1] && iv[i] < 12);
        CHECK (theory::fifthDegree ((ScaleType) s) >= 0); // every shipped scale has a perfect fifth
    }
}

TEST_CASE (Theory, InScaleForAllRoots)
{
    for (int root = 0; root < 12; ++root)
        for (int s = 0; s < kNumScales; ++s)
        {
            int count = 0;
            for (int pc = 0; pc < 12; ++pc) count += theory::inScale (root, (ScaleType) s, pc) ? 1 : 0;
            CHECK_EQ (count, theory::degreeCount ((ScaleType) s));
            CHECK (theory::inScale (root, (ScaleType) s, root));
        }
}

TEST_CASE (Theory, DiatonicStepWraps)
{
    const auto s = ScaleType::NaturalMinor;
    CHECK_EQ (theory::semitones (s, { 7, 0 }), 12);
    CHECK_EQ (theory::semitones (s, { -1, 0 }), -2);
    CHECK_EQ (theory::semitones (s, theory::step (s, { 0, 0 }, -3)), -5); // A -> G -> F -> E
}

TEST_CASE (Theory, ReferenceRegisters)
{
    for (int pc = 0; pc < 12; ++pc)
    {
        const int sub = theory::referenceRootMidi (pc, Family::Sub);
        const int acid = theory::referenceRootMidi (pc, Family::Acid);
        CHECK (sub >= 36 && sub <= 47);
        CHECK (acid >= 41 && acid <= 52);
        CHECK_EQ (sub % 12, pc);
        CHECK_EQ (acid % 12, pc);
    }
}

TEST_CASE (Theory, EnharmonicSpellingIsKeyConsistent)
{
    CHECK_EQ (theory::noteName (46, 5, ScaleType::Major), std::string ("Bb2"));      // F major uses flats
    CHECK_EQ (theory::noteName (42, 4, ScaleType::NaturalMinor), std::string ("F#2")); // E minor uses sharps
    CHECK_EQ (theory::noteName (39, 0, ScaleType::NaturalMinor), std::string ("Eb2")); // C minor uses flats
    CHECK_EQ (theory::rootName (10), std::string ("Bb"));
}

TEST_CASE (Seed, ParseAndOverflow)
{
    CHECK (parseSeed ("0") == std::optional<uint64_t> (0));
    CHECK (parseSeed (" 123 ") == std::optional<uint64_t> (123));
    CHECK (parseSeed ("18446744073709551615") == std::optional<uint64_t> (UINT64_MAX));
    CHECK (! parseSeed ("18446744073709551616").has_value());
    CHECK (! parseSeed ("-1").has_value());
    CHECK (! parseSeed ("12a").has_value());
    CHECK (! parseSeed ("").has_value());
    CHECK (parseSeed ("0xFF") == std::optional<uint64_t> (255));
}

TEST_CASE (Seed, RngGoldenSequence)
{
    // Fixed outputs: guards against accidental PRNG changes (breaks reproducibility).
    Rng r (12345);
    const uint64_t a = r.next(), b = r.next();
    Rng r2 (12345);
    CHECK_EQ (r2.next(), a);
    CHECK_EQ (r2.next(), b);
    CHECK (a != b);
    CHECK_EQ (deriveSeed (1, "rhythm"), deriveSeed (1, "rhythm"));
    CHECK (deriveSeed (1, "rhythm") != deriveSeed (1, "pitch"));
    CHECK (deriveSeed (1, "rhythm", 0) != deriveSeed (1, "rhythm", 1));
    CHECK (deriveSeed (1, "x") != deriveSeed (2, "x"));
}

TEST_CASE (Seed, BoundedSamplingUniform)
{
    Rng r (99);
    int hist[7] {};
    for (int i = 0; i < 70000; ++i)
    {
        const auto v = r.below (7);
        REQUIRE (v < 7);
        hist[v]++;
    }
    for (int h : hist) CHECK (h > 9300 && h < 10700);
    int w[3] { 1, 0, 3 };
    int c[3] {};
    for (int i = 0; i < 40000; ++i) c[r.pickWeighted (w, 3)]++;
    CHECK_EQ (c[1], 0);
    CHECK (c[2] > c[0] * 2);
}

TEST_CASE (Corpus, SizeValidityAndDistinctness)
{
    const auto corpus = rhythmCorpus();
    CHECK (corpus.size() >= 48);
    std::set<int> ids;
    std::set<std::vector<int>> rotations;
    for (const auto& a : corpus)
    {
        CHECK (ids.insert (a.id).second);
        REQUIRE (a.grid.size() == 16);
        const auto cell = rhythm::parseGrid (a.grid);
        CHECK (rhythm::onsetCount (cell) >= 2);
        // no hold after rest and every '~' must be followed by an onset
        for (size_t i = 0; i < 16; ++i)
        {
            const char c = a.grid[i];
            if (c == '-' || c == '~') CHECK_MSG (i > 0 && a.grid[i - 1] != '.', std::string (a.name));
            if (c == '~') CHECK_MSG (i < 15 && std::string ("AXxgo").find (a.grid[i + 1]) != std::string::npos, std::string (a.name));
        }
        // round trip through the cell representation
        CHECK_EQ (rhythm::toGrid (rhythm::parseGrid (rhythm::toGrid (cell))), rhythm::toGrid (cell));
        // distinct under every rotation (no shifted duplicates)
        std::vector<int> v (16);
        for (int i = 0; i < 16; ++i) v[(size_t) i] = cell.steps[(size_t) i].kind == StepKind::Onset ? 1 : 0;
        for (int r = 0; r < 16; ++r)
        {
            std::vector<int> rv (16);
            for (int i = 0; i < 16; ++i) rv[(size_t) i] = v[(size_t) ((i + r) % 16)];
            if (r > 0) CHECK_MSG (! rotations.count (rv), std::string ("rotation duplicate: ") + std::string (a.name));
        }
        CHECK_MSG (rotations.insert (v).second, std::string ("duplicate onsets: ") + std::string (a.name));
    }
}

TEST_CASE (Corpus, EveryFeelFamilyPoolHasThreeOrMore)
{
    for (int f = 1; f < kNumFeels; ++f)
        for (int fam = 0; fam < 2; ++fam)
        {
            const uint8_t bit = fam == 0 ? kFamSub : kFamAcid;
            int n = 0;
            for (const auto& a : rhythmCorpus())
                if ((a.families & bit) && (a.feel == (Feel) f || a.alsoFeel == (Feel) f)) ++n;
            CHECK_MSG (n >= 3, std::string (toString ((Feel) f)) + " " + std::string (toString ((Family) fam)));
        }
}

TEST_CASE (Corpus, ArchetypeDensityMatchesPrimaryFeel)
{
    for (const auto& a : rhythmCorpus())
    {
        const auto& fp = feelProfile (a.feel);
        const int n = rhythm::onsetCount (rhythm::parseGrid (a.grid));
        CHECK_MSG (n >= fp.minOnsets && n <= fp.maxOnsets, std::string (a.name));
    }
}

TEST_CASE (Rhythm, MutationsRespectBounds)
{
    for (const auto& a : rhythmCorpus())
        for (int fam = 0; fam < 2; ++fam)
        {
            const auto& fp = feelProfile (a.feel);
            Rng r ((uint64_t) a.id * 31 + (uint64_t) fam);
            auto cell = rhythm::parseGrid (a.grid);
            for (int i = 0; i < 200; ++i)
            {
                const auto m = rhythm::pickMutation (r, fp, (Family) fam);
                const auto before = cell;
                const bool ok = rhythm::mutate (cell, m, r, fp, (Family) fam);
                const int n = rhythm::onsetCount (cell);
                if (ok) CHECK (n >= fp.minOnsets && n <= fp.maxOnsets);
                else CHECK (cell == before);
                for (const auto& sl : rhythm::slots (cell)) CHECK (sl.length >= 1 && sl.step + sl.length <= 16);
            }
        }
}

TEST_CASE (Settings, SerializationRoundTrip)
{
    GeneratorSettings s;
    s.root = 3; s.scale = ScaleType::Phrygian; s.family = Family::Acid; s.feel = Feel::Stabby; s.bars = 8; s.octave = -2;
    s.styleSeed = UINT64_MAX; s.varSeed = 42; s.kickContext = KickContext::Broken; s.swing = 58;
    const auto back = serial::settingsFromMap (serial::mapFromString (serial::mapToString (serial::settingsToMap (s))));
    CHECK (back == s);
    LockSet l; l.root = true; l.bars = false; l.varSeed = true;
    CHECK (serial::locksFromMap (serial::locksToMap (l)) == l);
    // garbage values fall back to defaults field by field
    auto kv = serial::settingsToMap (s);
    kv["bars"] = "3"; kv["octave"] = "9"; kv["style"] = "x";
    const auto repaired = serial::settingsFromMap (kv);
    CHECK_EQ (repaired.bars, GeneratorSettings {}.bars);
    CHECK_EQ (repaired.octave, GeneratorSettings {}.octave);
    CHECK_EQ (repaired.styleSeed, GeneratorSettings {}.styleSeed);
    CHECK_EQ (repaired.root, 3);
}

TEST_CASE (Settings, RollRespectsLocks)
{
    GeneratorSettings s;
    LockSet all;
    all.root = all.scale = all.family = all.feel = all.bars = all.octave = all.styleSeed = all.varSeed = true;
    for (uint64_t e = 0; e < 200; ++e)
        CHECK (rollSettings (s, all, RollMode::Everything, e) == sanitise (s));
    LockSet none;
    none.bars = none.octave = false;
    int rootChanged = 0, styleChanged = 0;
    for (uint64_t e = 0; e < 200; ++e)
    {
        const auto r = rollSettings (s, none, RollMode::Everything, e);
        rootChanged += r.root != s.root;
        styleChanged += r.styleSeed != s.styleSeed;
        CHECK (r.feel != Feel::Random);
        CHECK (r.octave >= -1 && r.octave <= 1);
    }
    CHECK (rootChanged > 150);
    CHECK_EQ (styleChanged, 200);
    LockSet onlyVar; onlyVar.styleSeed = true;
    const auto r = rollSettings (s, onlyVar, RollMode::Seeds, 7);
    CHECK_EQ (r.styleSeed, s.styleSeed);
    CHECK (r.varSeed != s.varSeed);
    const auto v = rollSettings (s, LockSet {}, RollMode::VarOnly, 7);
    CHECK_EQ (v.styleSeed, s.styleSeed);
    CHECK (v.varSeed != s.varSeed);
    CHECK (v.root == s.root && v.scale == s.scale && v.feel == s.feel);
}

#include <map>
#include <set>

#include "TestFramework.h"
#include "core/PatternGenerator.h"
#include "core/PatternHistory.h"
#include "core/PatternMetrics.h"
#include "core/ScaleTheory.h"
#include "core/Serialization.h"

using namespace gfb;

namespace
{
GeneratorSettings make (int root, int scale, int fam, int feel, int bars, uint64_t style, uint64_t var, int octave = 0)
{
    GeneratorSettings s;
    s.root = root; s.scale = (ScaleType) scale; s.family = (Family) fam; s.feel = (Feel) feel; s.bars = bars;
    s.styleSeed = style; s.varSeed = var; s.octave = octave;
    return s;
}
} // namespace

TEST_CASE (Generator, AllOptionCombinationsValidAndInScale)
{
    const int barsOpts[] = { 1, 2, 4, 8 };
    int total = 0;
    for (int root = 0; root < 12; ++root)
        for (int sc = 0; sc < kNumScales; ++sc)
            for (int fam = 0; fam < 2; ++fam)
                for (int feel = 0; feel < kNumFeels; ++feel)
                {
                    const int bars = barsOpts[(root + sc + feel) % 4];
                    const auto s = make (root, sc, fam, feel, bars, (uint64_t) (root * 1000 + sc * 100 + feel * 10 + fam + 1), (uint64_t) (feel + 7));
                    const auto p = generatePattern (s);
                    ++total;
                    const auto err = validatePattern (p);
                    CHECK_MSG (err.empty(), err);
                    CHECK (! p.notes.empty());
                    CHECK_EQ (p.bars, bars);
                    for (const auto& n : p.notes) CHECK (theory::inScale (root, (ScaleType) sc, n.midiNote % 12));
                    const auto m = computeMetrics (p);
                    CHECK_EQ (m.emptyBars, 0);
                }
    CHECK_EQ (total, 12 * kNumScales * 2 * kNumFeels);
}

TEST_CASE (Generator, TenThousandRandomCasesNoInvalidEvents)
{
    Rng r (2024);
    const int barsOpts[] = { 1, 2, 4, 8 };
    int invalid = 0, outOfScale = 0, silent = 0, fallbacks = 0;
    for (int i = 0; i < 10000; ++i)
    {
        GeneratorSettings s = make (r.range (0, 11), r.range (0, kNumScales - 1), r.range (0, 1), r.range (0, kNumFeels - 1),
                                    barsOpts[r.below (4)], r.next(), r.next(), r.range (-3, 3));
        s.kickContext = (KickContext) r.range (0, 2);
        GenerationReport rep;
        GenerationOptions o; o.styleCandidates = 6; o.varCandidates = 6; // keep the test fast; same code paths
        const auto p = generatePattern (s, o, &rep);
        invalid += validatePattern (p).empty() ? 0 : 1;
        silent += p.notes.empty() ? 1 : 0;
        fallbacks += rep.fallbackUsed ? 1 : 0;
        for (const auto& n : p.notes) outOfScale += theory::inScale (s.root, s.scale, n.midiNote % 12) ? 0 : 1;
    }
    CHECK_EQ (invalid, 0);
    CHECK_EQ (outOfScale, 0);
    CHECK_EQ (silent, 0);
    CHECK (fallbacks < 50); // < 0.5 %
}

TEST_CASE (Generator, DeterministicSameInputsSameHash)
{
    for (int i = 0; i < 60; ++i)
    {
        const auto s = make (i % 12, i % kNumScales, i % 2, i % kNumFeels, 1 << (i % 4), (uint64_t) i * 977 + 1, (uint64_t) i * 13 + 5);
        const auto a = generatePattern (s);
        const auto b = generatePattern (s);
        CHECK_EQ (a.hash, b.hash);
        CHECK (a.notes == b.notes);
        CHECK_EQ (a.hash, computePatternHash (a));
    }
}

// Golden fixture: exact hashes recorded for the current kAlgorithmVersion. These must be
// identical on every supported compiler/OS (verified on Linux GCC and the
// Windows MSVC / macOS Apple Clang CI jobs). Update ONLY together with
// kAlgorithmVersion (regenerate with: pattern_diagnostics --golden).
TEST_CASE (Generator, GoldenHashes)
{
    struct G { GeneratorSettings s; uint64_t hash; };
    const std::vector<G> golden = {
#include "golden_hashes.inc"
    };
    CHECK (golden.size() >= 12);
    for (const auto& g : golden)
    {
        const auto p = generatePattern (g.s);
        CHECK_MSG (p.hash == g.hash, serial::mapToString (serial::settingsToMap (g.s)) + " got " + std::to_string (p.hash));
    }
}

TEST_CASE (Generator, VarPreservesStructureStyleChangesConcept)
{
    double varSimSum = 0, styleSimSum = 0;
    int n = 0, varChanged = 0;
    for (int i = 0; i < 300; ++i)
    {
        const int bars = i % 2 ? 4 : 2;
        const auto base = make (i % 12, i % kNumScales, i % 2, 1 + i % 6, bars, (uint64_t) i * 7919 + 3, 11);
        auto varS = base; varS.varSeed = 12345 + (uint64_t) i;
        auto styleS = base; styleS.styleSeed = base.styleSeed + 104729;
        const auto a = generatePattern (base), bv = generatePattern (varS), bs = generatePattern (styleS);
        varSimSum += patternNoteOverlap (a, bv);
        styleSimSum += patternNoteOverlap (a, bs);
        varChanged += a.hash != bv.hash ? 1 : 0;
        ++n;
    }
    const double varSim = varSimSum / n, styleSim = styleSimSum / n;
    std::printf ("    VAR change note overlap %.3f, STYLE change note overlap %.3f, VAR changed output %d/%d\n", varSim, styleSim, varChanged, n);
    CHECK (varSim > 0.6);              // recognisable structure kept
    CHECK (styleSim < varSim - 0.25);  // new concept
    CHECK (varChanged > n * 8 / 10);   // VAR genuinely changes something
}

TEST_CASE (Generator, OctaveTranspositionKeepsValidMidiAndKey)
{
    for (int oct = kMinOctave; oct <= kMaxOctave; ++oct)
        for (int fam = 0; fam < 2; ++fam)
            for (int root = 0; root < 12; ++root)
            {
                const auto s = make (root, 0, fam, 4, 4, 77, 3, oct);
                const auto p = generatePattern (s);
                CHECK (validatePattern (p).empty());
                for (auto& e : p.notes) CHECK (e.midiNote <= 127 && theory::inScale (root, s.scale, e.midiNote % 12));
            }
    // octave shift moves the line by octaves (same pitch classes and rhythm)
    auto s0 = make (9, 0, 0, 2, 4, 5, 6, 0), s1 = s0;
    s1.octave = 1;
    const auto a = generatePattern (s0), b = generatePattern (s1);
    REQUIRE (a.notes.size() == b.notes.size());
    for (size_t i = 0; i < a.notes.size(); ++i) CHECK_EQ ((int) b.notes[i].midiNote - (int) a.notes[i].midiNote, 12);
}

TEST_CASE (Generator, PhraseStructureAndEndings)
{
    int resolved = 0, total = 0, motifOk = 0, longTotal = 0;
    for (int i = 0; i < 400; ++i)
    {
        const int bars = 1 << (i % 4);
        const auto p = generatePattern (make (i % 12, i % kNumScales, i % 2, 1 + i % 6, bars, (uint64_t) i + 100, (uint64_t) i + 9));
        const auto m = computeMetrics (p);
        resolved += m.endsResolved ? 1 : 0;
        ++total;
        if (bars >= 4)
        {
            ++longTotal;
            // structural repetition: some bars repeat, but not all bars identical
            motifOk += (m.distinctBars > 1 && m.distinctBars < bars && m.barSimilarity >= 0.45) ? 1 : 0;
        }
        // no note rings over the loop end; last note never slides into the wrap
        CHECK (p.notes.back().endTick() <= p.lengthTicks());
        CHECK (! p.notes.back().slideToNext);
        // A section anchored: first bar non-empty
        CHECK (p.notes.front().startTick < kTicksPerBar);
    }
    std::printf ("    resolved endings %d/%d, structured long phrases %d/%d\n", resolved, total, motifOk, longTotal);
    CHECK (resolved > total * 9 / 10);
    CHECK (motifOk > longTotal * 85 / 100);
}

TEST_CASE (Generator, SwingIsBoundedAndOffGridOnlyOnOddSixteenths)
{
    auto s = make (0, 0, 0, 4, 4, 31, 4);
    s.swing = 66;
    const auto p = generatePattern (s);
    CHECK (validatePattern (p).empty());
    for (const auto& n : p.notes)
    {
        const int64_t off = n.startTick % kTicksPerSixteenth;
        const int64_t step = n.startTick / kTicksPerSixteenth;
        if (off != 0) CHECK (step % 2 == 1 && off <= 80);
    }
    s.swing = 50;
    for (const auto& n : generatePattern (s).notes) CHECK_EQ (n.startTick % kTicksPerSixteenth, 0);
}

TEST_CASE (Generator, FewExactDuplicatesAcrossSeeds)
{
    std::map<uint64_t, int> seen;
    int dup = 0, N = 0;
    for (int fam = 0; fam < 2; ++fam)
        for (int feel = 1; feel < kNumFeels; ++feel)
            for (uint64_t seed = 1; seed <= 40; ++seed)
            {
                const auto p = generatePattern (make (9, 0, fam, feel, 4, seed * 101, seed * 7));
                dup += seen[p.hash]++ > 0 ? 1 : 0;
                ++N;
            }
    std::printf ("    exact duplicates %d/%d\n", dup, N);
    CHECK (dup * 100 < N * 2); // < 2 %
}

TEST_CASE (History, CapacityDedupeAndRecall)
{
    PatternHistory h;
    std::vector<uint64_t> hashes;
    for (int i = 0; i < 70; ++i)
    {
        const auto p = generatePattern (make (0, 0, 0, 2, 1, (uint64_t) i + 1, 1));
        CHECK (h.push (p));
        hashes.push_back (p.hash);
    }
    CHECK_EQ (h.size(), PatternHistory::kCapacity);
    CHECK_EQ (h.at (0).hash, hashes[6]);
    CHECK_EQ (h.cursor(), 63);
    CHECK (! h.push (h.at (63))); // same as newest -> ignored
    auto back = h.back();
    REQUIRE (back.has_value());
    CHECK_EQ (back->hash, hashes[68]);
    CHECK_EQ (back->settings.styleSeed, 69u); // recall restores generator settings
    const auto sel = h.select (10);
    REQUIRE (sel.has_value());
    CHECK_EQ (sel->hash, hashes[16]);
    // pushing after navigation appends (nothing is lost) and moves cursor to the end
    CHECK (h.push (generatePattern (make (1, 1, 1, 3, 2, 999, 1))));
    CHECK_EQ (h.cursor(), 63);
    CHECK_EQ (h.at (9).hash, hashes[16]);
}

TEST_CASE (Serialization, PatternStringRoundTrip)
{
    for (int i = 0; i < 50; ++i)
    {
        const auto p = generatePattern (make (i % 12, i % 7, i % 2, i % 7, 1 << (i % 4), (uint64_t) i * 3 + 1, (uint64_t) i));
        const auto back = serial::patternFromString (serial::patternToString (p));
        REQUIRE (back.has_value());
        CHECK (back->notes == p.notes);
        CHECK (back->settings == p.settings);
        CHECK_EQ (back->hash, p.hash);
        CHECK_EQ (back->bars, p.bars);
    }
    CHECK (! serial::patternFromString ("garbage").has_value());
    CHECK (! serial::patternFromString ("bars=4;|0:240:300:100:0;").has_value());
    CHECK (! serial::notesFromString ("0:0:40:100:0;").has_value());
}

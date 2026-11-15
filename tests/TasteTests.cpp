// Local taste learning (LIKE / DISLIKE) and the chromatic embellishment mode.
#include <map>

#include "TestFramework.h"
#include "core/PatternGenerator.h"
#include "core/PatternMetrics.h"
#include "core/ScaleTheory.h"
#include "core/TasteProfile.h"

using namespace gfb;

namespace
{
GeneratorSettings make (Feel feel, Family fam, uint64_t style, uint64_t var)
{
    GeneratorSettings s;
    s.feel = feel; s.family = fam; s.bars = 4; s.styleSeed = style; s.varSeed = var;
    return s;
}
} // namespace

TEST_CASE (Taste, EmptyProfileEqualsFactory)
{
    TasteProfile empty;
    GenerationOptions o;
    o.taste = &empty;
    for (uint64_t i = 1; i <= 40; ++i)
    {
        const auto s = make ((Feel) (1 + i % 6), (Family) (i % 2), i * 101, i);
        CHECK_EQ (generatePattern (s, o).hash, generatePattern (s).hash);
    }
}

TEST_CASE (Taste, FeedbackMovesScoreAndRoundTrips)
{
    TasteProfile t;
    const auto liked = generatePattern (make (Feel::Rolling, Family::Sub, 11, 1));
    const auto disliked = generatePattern (make (Feel::Sparse, Family::Sub, 22, 2));
    for (int i = 0; i < 4; ++i) { taste::addFeedback (t, liked, true); taste::addFeedback (t, disliked, false); }
    CHECK_EQ (t.likes, 4);
    CHECK_EQ (t.dislikes, 4);
    CHECK_EQ (t.revision, 8u);
    CHECK (taste::score (liked, t) > 0.55);
    CHECK (taste::score (disliked, t) < 0.45);
    CHECK (taste::confidence (t) >= 1.0);
    CHECK (taste::archetypePermille (t, liked.archetypeId) > 1000);
    CHECK (taste::archetypePermille (t, disliked.archetypeId) < 1000);
    CHECK (! t.cells.empty());
    const auto back = taste::fromString (taste::toString (t));
    REQUIRE (back.has_value());
    CHECK (*back == t);
    CHECK (! taste::fromString ("garbage").has_value());
    CHECK (! taste::fromString ("v=1;rev=x;").has_value());
}

TEST_CASE (Taste, DeterministicForSameProfile)
{
    TasteProfile t;
    for (uint64_t i = 1; i <= 6; ++i) taste::addFeedback (t, generatePattern (make (Feel::Balanced, Family::Acid, i * 7, i)), i % 3 != 0);
    GenerationOptions o;
    o.taste = &t;
    for (uint64_t i = 1; i <= 30; ++i)
    {
        const auto s = make (Feel::Balanced, Family::Acid, 1000 + i, i);
        const auto a = generatePattern (s, o), b = generatePattern (s, o);
        CHECK_EQ (a.hash, b.hash);
        CHECK_EQ (a.tasteRevision, t.revision);
        CHECK (validatePattern (a).empty());
    }
}

TEST_CASE (Taste, LearningChangesWhatGetsGenerated)
{
    // like 6 patterns of one groove family, then measure how much the generator moved towards them
    TasteProfile t;
    std::vector<Pattern> liked;
    for (uint64_t i = 1; i <= 6; ++i)
    {
        auto p = generatePattern (make (Feel::Syncopated, Family::Sub, 500 + i, i));
        taste::addFeedback (t, p, true);
        liked.push_back (p);
    }
    for (uint64_t i = 1; i <= 6; ++i) taste::addFeedback (t, generatePattern (make (Feel::Syncopated, Family::Sub, 900 + i, i)), false);
    GenerationOptions o;
    o.taste = &t;
    double before = 0, after = 0;
    int learnedCellHits = 0;
    for (uint64_t i = 1; i <= 150; ++i)
    {
        const auto s = make (Feel::Syncopated, Family::Sub, 20000 + i * 13, i);
        const auto f = generatePattern (s), g = generatePattern (s, o);
        before += taste::score (f, t);
        after += taste::score (g, t);
        learnedCellHits += g.archetypeId >= 9000 ? 1 : 0;
    }
    std::printf ("    mean taste score factory %.3f -> with taste %.3f, learned cells used %d/150\n", before / 150, after / 150, learnedCellHits);
    CHECK (after / 150 > before / 150 + 0.01);
    CHECK (learnedCellHits > 15);
}

TEST_CASE (Taste, DislikedArchetypeBecomesRarer)
{
    // find a frequent archetype, dislike it repeatedly, count again
    std::map<int, int> freq;
    for (uint64_t i = 1; i <= 200; ++i) freq[generatePattern (make (Feel::Balanced, Family::Sub, i * 31, 1)).archetypeId]++;
    int top = -1, topN = 0;
    for (auto& [id, n] : freq) if (n > topN) { top = id; topN = n; }
    TasteProfile t;
    for (uint64_t i = 1; i <= 400 && t.dislikes < 6; ++i)
    {
        const auto p = generatePattern (make (Feel::Balanced, Family::Sub, i * 31, 1));
        if (p.archetypeId == top) taste::addFeedback (t, p, false);
    }
    GenerationOptions o;
    o.taste = &t;
    int after = 0;
    for (uint64_t i = 1; i <= 200; ++i) after += generatePattern (make (Feel::Balanced, Family::Sub, i * 31, 1), o).archetypeId == top ? 1 : 0;
    std::printf ("    archetype %d: %d/200 before, %d/200 after 6 dislikes\n", top, topN, after);
    CHECK (after < topN / 2);
}

TEST_CASE (Taste, ChromaticModeOnlyWhenEnabledAndResolves)
{
    int chromNotes = 0, patternsWith = 0;
    for (uint64_t i = 1; i <= 300; ++i)
    {
        auto s = make ((Feel) (1 + i % 6), (Family) (i % 2), i * 17, i);
        s.root = (int) (i % 12);
        s.scale = (ScaleType) (i % kNumScales);
        for (auto& n : generatePattern (s).notes) CHECK (theory::inScale (s.root, s.scale, n.midiNote % 12));
        s.chromatic = true;
        const auto p = generatePattern (s);
        CHECK (validatePattern (p).empty());
        std::map<int, int> perBar;
        int here = 0;
        for (size_t k = 0; k < p.notes.size(); ++k)
        {
            const auto& n = p.notes[k];
            if (theory::inScale (s.root, s.scale, n.midiNote % 12)) continue;
            ++here;
            REQUIRE (k + 1 < p.notes.size());
            CHECK_EQ (std::abs ((int) p.notes[k + 1].midiNote - (int) n.midiNote), 1); // resolves by semitone
            CHECK (++perBar[tickToStep (n.startTick) / kStepsPerBar] == 1);            // max one per bar
        }
        chromNotes += here;
        patternsWith += here > 0 ? 1 : 0;
    }
    std::printf ("    chromatic: %d approach notes in %d/300 patterns\n", chromNotes, patternsWith);
    CHECK (patternsWith > 60);
}

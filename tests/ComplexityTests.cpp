// Complexity control: 50 = factory behaviour, higher = more notes, lower = fewer.
#include <cstdio>

#include "TestFramework.h"
#include "core/PatternGenerator.h"
#include "core/PatternMetrics.h"
#include "core/RhythmGrammar.h"
#include "core/Serialization.h"

using namespace gfb;

namespace
{
GeneratorSettings make (Feel feel, Family fam, uint64_t style, uint64_t var, int cx)
{
    GeneratorSettings s;
    s.feel = feel; s.family = fam; s.bars = 4; s.styleSeed = style; s.varSeed = var; s.complexity = cx;
    return s;
}
} // namespace

TEST_CASE (Complexity, NeutralIsUnchangedAndProfileIdentity)
{
    for (int f = 1; f < kNumFeels; ++f)
    {
        const auto& base = feelProfile ((Feel) f);
        const auto same = withComplexity (base, kNeutralComplexity);
        CHECK_EQ (same.targetOnsets, base.targetOnsets);
        CHECK_EQ (same.minOnsets, base.minOnsets);
        CHECK_EQ (same.maxOnsets, base.maxOnsets);
        CHECK (same.mutationWeights == base.mutationWeights);
        for (int cx : { 0, 25, 75, 100 })
        {
            const auto p = withComplexity (base, cx);
            CHECK (p.minOnsets >= 2 && p.minOnsets <= p.targetOnsets && p.targetOnsets <= p.maxOnsets && p.maxOnsets <= 16);
        }
    }
    GeneratorSettings d;
    CHECK_EQ (d.complexity, kNeutralComplexity);
}

TEST_CASE (Complexity, MoreComplexityMeansMoreNotes)
{
    const int levels[] = { 0, 25, 50, 75, 100 };
    for (int fam = 0; fam < 2; ++fam)
        for (int f = 1; f < kNumFeels; ++f)
        {
            double mean[5] {};
            for (int li = 0; li < 5; ++li)
            {
                double sum = 0;
                for (uint64_t i = 1; i <= 60; ++i)
                {
                    const auto p = generatePattern (make ((Feel) f, (Family) fam, 300 + i * 13, i, levels[li]));
                    CHECK (validatePattern (p).empty());
                    sum += computeMetrics (p).onsetsPerBar;
                }
                mean[li] = sum / 60.0;
            }
            std::printf ("    %s %-10s onsets/bar  0:%.2f 25:%.2f 50:%.2f 75:%.2f 100:%.2f\n", fam ? "ACID" : "SUB ",
                         std::string (toString ((Feel) f)).c_str(), mean[0], mean[1], mean[2], mean[3], mean[4]);
            for (int li = 1; li < 5; ++li) CHECK (mean[li] >= mean[li - 1] - 0.05);
            CHECK (mean[4] > mean[2] + 0.6);
            CHECK (mean[0] < mean[2] - 0.4);
        }
}

TEST_CASE (Complexity, SerialisedAndDeterministic)
{
    auto s = make (Feel::Rolling, Family::Acid, 77, 9, 85);
    const auto back = serial::settingsFromMap (serial::settingsToMap (s));
    CHECK_EQ (back.complexity, 85);
    CHECK_EQ (generatePattern (s).hash, generatePattern (s).hash);
    s.complexity = 150;
    CHECK_EQ (sanitise (s).complexity, 100);
}

// Algorithm v4 genre rules (docs/BASS_RESEARCH.md): four-on-the-floor kick lane,
// root-centred pitch, accents on the off-8th, few pitches per bar.
#include <set>

#include "TestFramework.h"
#include "core/PatternGenerator.h"
#include "core/PatternMetrics.h"

using namespace gfb;

namespace
{
GeneratorSettings make (Feel feel, Family fam, uint64_t style, uint64_t var, int bars)
{
    GeneratorSettings s;
    s.feel = feel; s.family = fam; s.bars = bars; s.styleSeed = style; s.varSeed = var;
    s.kickContext = KickContext::FourOnFloor;
    s.root = (int) (style % 12);
    return s;
}
} // namespace

TEST_CASE (Groove, FourOnFloorKickLaneIsFree)
{
    int notes = 0;
    for (int fam = 0; fam < 2; ++fam)
        for (int f = 0; f < kNumFeels; ++f)
            for (uint64_t i = 1; i <= 40; ++i)
            {
                const auto p = generatePattern (make ((Feel) f, (Family) fam, 900 + i * 31, i, 1 << (i % 4)));
                for (const auto& n : p.notes)
                {
                    ++notes;
                    CHECK (n.startTick % kPPQ != 0); // never on a kick
                    if (n.slideToNext) continue;
                    const int64_t nextKick = (n.startTick / kPPQ + 1) * kPPQ;
                    CHECK (n.endTick() <= nextKick); // released before the next kick
                }
            }
    CHECK (notes > 1000);
}

TEST_CASE (Groove, RootCentredAndFewPitchesPerBar)
{
    for (int fam = 0; fam < 2; ++fam)
    {
        double rootSum = 0;
        int n = 0, tooMany = 0, bars = 0;
        for (int f = 1; f < kNumFeels; ++f)
            for (uint64_t i = 1; i <= 40; ++i)
            {
                const auto p = generatePattern (make ((Feel) f, (Family) fam, 4000 + i * 17, i, 4));
                rootSum += computeMetrics (p).rootRatio;
                ++n;
                for (int b = 0; b < p.bars; ++b)
                {
                    std::set<int> pcs;
                    for (const auto& note : p.notes)
                        if (note.startTick / kTicksPerBar == b) pcs.insert (note.midiNote);
                    ++bars;
                    // fills may add one chord tone: allow ACID 5, SUB 4 distinct notes
                    if ((int) pcs.size() > (fam == 1 ? 5 : 4)) ++tooMany;
                }
            }
        const double root = rootSum / n;
        std::printf ("    %s mean root share %.2f, bars over the pitch cap %d/%d\n", fam ? "ACID" : "SUB", root, tooMany, bars);
        CHECK (root > (fam == 1 ? 0.35 : 0.5));
        CHECK (tooMany * 50 < bars); // < 2 %
    }
}

TEST_CASE (Groove, AccentsSitOnTheOffEighth)
{
    int acc = 0, offAcc = 0;
    for (uint64_t i = 1; i <= 120; ++i)
    {
        const auto p = generatePattern (make ((Feel) (1 + i % 6), (Family) (i % 2), 70 + i * 13, i, 4));
        for (const auto& n : p.notes)
            if (n.accent)
            {
                ++acc;
                // swing never moves an off-8th (even 16th), so the grid step is exact
                if (n.startTick % kPPQ == 2 * kTicksPerSixteenth) ++offAcc;
            }
    }
    std::printf ("    accents on the off-8th: %d/%d\n", offAcc, acc);
    CHECK (acc > 100);
    CHECK (offAcc * 100 >= acc * 95);
}

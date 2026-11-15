// Statistical distinctness of Feel and Family behaviour (A16).
#include <array>

#include "TestFramework.h"
#include "core/PatternGenerator.h"
#include "core/PatternMetrics.h"

using namespace gfb;

namespace
{
struct Mean
{
    double onsets = 0, sync = 0, offbeat = 0, meanDur = 0, shortR = 0, longR = 0, chain = 0, rest = 0, ghost = 0,
           sounding = 0, step = 0, slide = 0, leap = 0, distinct = 0, accent = 0, octave = 0, range = 0, swung = 0;
};

Mean measure (Family fam, Feel feel, int n = 240)
{
    Mean m;
    for (int i = 0; i < n; ++i)
    {
        GeneratorSettings s;
        s.family = fam; s.feel = feel; s.bars = 4;
        s.root = i % 12; s.scale = (ScaleType) (i % kNumScales);
        s.styleSeed = (uint64_t) i * 2654435761ULL + 17; s.varSeed = (uint64_t) i + 3;
        const auto x = computeMetrics (generatePattern (s));
        m.onsets += x.onsetsPerBar; m.sync += x.syncopation; m.offbeat += x.offbeatRatio; m.meanDur += x.meanDurSteps;
        m.shortR += x.shortRatio; m.longR += x.longRatio; m.chain += x.maxChain; m.rest += x.longestRestSteps; m.ghost += x.ghostRatio;
        m.sounding += x.soundingRatio; m.step += x.stepRatio; m.slide += x.slideRatio; m.leap += x.leapRatio;
        m.distinct += x.distinctPitches; m.accent += x.accentRatio; m.octave += x.octaveRatio; m.range += x.pitchRange; m.swung += x.swungRatio;
    }
    for (double* d : { &m.onsets, &m.sync, &m.offbeat, &m.meanDur, &m.shortR, &m.longR, &m.chain, &m.rest, &m.ghost, &m.sounding,
                       &m.step, &m.slide, &m.leap, &m.distinct, &m.accent, &m.octave, &m.range, &m.swung })
        *d /= n;
    return m;
}

void print (const char* label, const Mean& m)
{
    std::printf ("    %-16s ons/bar %.2f sync %.2f offb %.2f dur %.2f short %.2f long %.2f chain %.1f rest %.1f ghost %.2f sound %.2f step %.2f slide %.2f oct %.2f range %.1f\n",
                 label, m.onsets, m.sync, m.offbeat, m.meanDur, m.shortR, m.longR, m.chain, m.rest, m.ghost, m.sounding, m.step, m.slide, m.octave, m.range);
}
} // namespace

TEST_CASE (Metrics, FeelsAreStatisticallyDistinct)
{
    for (int fam = 0; fam < 2; ++fam)
    {
        std::array<Mean, kNumFeels> m;
        for (int f = 1; f < kNumFeels; ++f)
        {
            m[(size_t) f] = measure ((Family) fam, (Feel) f);
            print ((std::string (toString ((Family) fam)) + " " + std::string (toString ((Feel) f))).c_str(), m[(size_t) f]);
        }
        const auto& sp = m[(size_t) Feel::Sparse];
        const auto& ba = m[(size_t) Feel::Balanced];
        const auto& bu = m[(size_t) Feel::Busy];
        const auto& ro = m[(size_t) Feel::Rolling];
        const auto& sy = m[(size_t) Feel::Syncopated];
        const auto& st = m[(size_t) Feel::Stabby];
        // density ladder
        CHECK (sp.onsets < ba.onsets - 1.0);
        CHECK (ba.onsets < bu.onsets - 2.0);
        CHECK (ro.onsets > ba.onsets + 1.0);
        CHECK (sp.onsets >= 2.0 && sp.onsets <= 6.0);
        CHECK (bu.onsets >= 8.0 && bu.onsets <= 13.0);
        // sparse: long silences
        CHECK (sp.rest > ba.rest && sp.rest > bu.rest);
        // busy: the longest 16th chains but bounded (no machine gun)
        CHECK (bu.chain > ba.chain && bu.chain <= 8.0);
        // stabby: clipped notes, lowest sounding ratio
        CHECK (st.shortR > ba.shortR + 0.2 && st.shortR > sy.shortR + 0.2);
        CHECK (st.sounding < ba.sounding && st.sounding < sy.sounding);
        // syncopated: most syncopation, notes held through beats (long ratio)
        CHECK (sy.sync > ba.sync && sy.sync > st.sync);
        CHECK (sy.longR > st.longR);
        // rolling: offbeat bounce and ghost/short chains
        CHECK (ro.offbeat > bu.offbeat);
        CHECK (ro.meanDur < ba.meanDur - 0.2); // short-note chains (v3 gates are legato, see QUALITY_REPORT)
        CHECK (ro.chain > sp.chain);
    }
}

TEST_CASE (Metrics, SubAndAcidAreStatisticallyDistinct)
{
    Mean sub, acid;
    for (int f = 1; f < kNumFeels; ++f)
    {
        const auto a = measure (Family::Sub, (Feel) f, 120), b = measure (Family::Acid, (Feel) f, 120);
        sub.step += a.step; acid.step += b.step;
        sub.slide += a.slide; acid.slide += b.slide;
        sub.distinct += a.distinct; acid.distinct += b.distinct;
        sub.accent += a.accent; acid.accent += b.accent;
        sub.sounding += a.sounding; acid.sounding += b.sounding;
        sub.leap += a.leap; acid.leap += b.leap;
    }
    std::printf ("    SUB  step %.2f slide %.3f distinct %.2f accent %.2f sounding %.2f leap %.3f\n", sub.step / 6, sub.slide / 6, sub.distinct / 6, sub.accent / 6, sub.sounding / 6, sub.leap / 6);
    std::printf ("    ACID step %.2f slide %.3f distinct %.2f accent %.2f sounding %.2f leap %.3f\n", acid.step / 6, acid.slide / 6, acid.distinct / 6, acid.accent / 6, acid.sounding / 6, acid.leap / 6);
    CHECK (acid.step > sub.step * 1.5);       // acid: scale-step motion
    CHECK (acid.slide > sub.slide * 3.0);     // acid: slides
    CHECK (acid.distinct > sub.distinct);     // acid: richer pitch cells
    CHECK (sub.sounding > acid.sounding);     // sub: longer, fuller notes
}

#include "core/PatternMetrics.h"

#include <algorithm>
#include <cstdlib>
#include <set>
#include <sstream>

#include "core/ScaleTheory.h"

namespace gfb
{
int tickToStep (int64_t tick) noexcept { return (int) ((tick + kTicksPerSixteenth / 2) / kTicksPerSixteenth); }

PatternMetrics computeMetrics (const Pattern& p)
{
    PatternMetrics m;
    m.bars = p.bars;
    const auto& n = p.notes;
    m.noteCount = (int) n.size();
    if (p.bars <= 0) return m;
    const int totalSteps = p.bars * kStepsPerBar;
    std::vector<int> barOnsets ((size_t) p.bars, 0);
    std::vector<char> onsetAt ((size_t) totalSteps, 0);
    std::vector<int> pitchAt ((size_t) totalSteps, -1);

    double syncSum = 0, durSum = 0;
    int offbeat = 0, kick = 0, down = 0, shortN = 0, longN = 0, acc = 0, ghost = 0, slide = 0, swung = 0;
    int64_t sounding = 0;
    const int root = p.settings.root;
    const auto scale = p.settings.scale;
    std::set<int> pitches;
    int lo = 127, hi = 0, rootN = 0, fifthN = 0;
    for (const auto& e : n)
    {
        const int step = std::clamp (tickToStep (e.startTick), 0, totalSteps - 1);
        const int s = step % kStepsPerBar;
        barOnsets[(size_t) (step / kStepsPerBar)]++;
        onsetAt[(size_t) step] = 1;
        pitchAt[(size_t) step] = e.midiNote;
        syncSum += (s % 4 == 0) ? 0.0 : (s % 2 == 0 ? 0.5 : 1.0);
        offbeat += (s % 4 == 2) ? 1 : 0;
        kick += (s == 4 || s == 8 || s == 12) ? 1 : 0;
        down += (s == 0) ? 1 : 0;
        const double ds = (double) e.durationTicks / kTicksPerSixteenth;
        durSum += ds;
        shortN += ds < 0.8 ? 1 : 0;
        longN += ds >= 2.0 ? 1 : 0;
        acc += e.accent ? 1 : 0;
        ghost += e.velocity < 70 ? 1 : 0;
        slide += e.slideToNext ? 1 : 0;
        swung += (e.startTick % kTicksPerSixteenth) != 0 ? 1 : 0;
        sounding += e.durationTicks;
        pitches.insert (e.midiNote);
        lo = std::min<int> (lo, e.midiNote);
        hi = std::max<int> (hi, e.midiNote);
        const int rel = ((e.midiNote - root) % 12 + 12) % 12;
        rootN += rel == 0 ? 1 : 0;
        fifthN += rel == 7 ? 1 : 0;
        m.outOfScale += theory::inScale (root, scale, e.midiNote % 12) ? 0 : 1;
    }
    const double N = std::max (1, m.noteCount);
    m.onsetsPerBar = (double) m.noteCount / p.bars;
    m.minBarOnsets = *std::min_element (barOnsets.begin(), barOnsets.end());
    m.maxBarOnsets = *std::max_element (barOnsets.begin(), barOnsets.end());
    m.emptyBars = (int) std::count (barOnsets.begin(), barOnsets.end(), 0);
    m.syncopation = syncSum / N;
    m.offbeatRatio = offbeat / N;
    m.kickCollision = kick / N;
    m.downbeatRatio = (double) down / p.bars;
    m.meanDurSteps = durSum / N;
    m.shortRatio = shortN / N;
    m.longRatio = longN / N;
    m.soundingRatio = (double) std::min<int64_t> (sounding, p.lengthTicks()) / (double) p.lengthTicks();
    m.accentRatio = acc / N;
    m.ghostRatio = ghost / N;
    m.slideRatio = slide / N;
    m.swungRatio = swung / N;
    m.distinctPitches = (int) pitches.size();
    m.pitchRange = m.noteCount ? hi - lo : 0;
    m.rootRatio = rootN / N;
    m.fifthRatio = fifthN / N;

    // chains of consecutive 16th onsets (no wrap) and longest silence (wrap-aware)
    int chain = 0;
    for (int i = 0; i < totalSteps; ++i)
    {
        chain = onsetAt[(size_t) i] ? chain + 1 : 0;
        m.maxChain = std::max (m.maxChain, chain);
    }
    if (! n.empty())
    {
        int64_t longest = 0;
        for (size_t i = 0; i < n.size(); ++i)
        {
            const int64_t nextStart = i + 1 < n.size() ? n[i + 1].startTick : n[0].startTick + p.lengthTicks();
            longest = std::max<int64_t> (longest, nextStart - n[i].endTick());
        }
        m.longestRestSteps = (double) longest / kTicksPerSixteenth;
    }
    else m.longestRestSteps = totalSteps;

    // intervals
    int steps = 0, leaps = 0, octs = 0, reps = 0, run = 1;
    double absSum = 0;
    for (size_t i = 1; i < n.size(); ++i)
    {
        const int iv = std::abs ((int) n[i].midiNote - (int) n[i - 1].midiNote);
        absSum += iv;
        steps += (iv >= 1 && iv <= 2) ? 1 : 0;
        leaps += (iv > 7 && iv != 12) ? 1 : 0;
        octs += iv == 12 ? 1 : 0;
        reps += iv == 0 ? 1 : 0;
        run = iv == 0 ? run + 1 : 1;
        m.maxSamePitchRun = std::max (m.maxSamePitchRun, run);
    }
    if (n.size() == 1) m.maxSamePitchRun = 1;
    const double I = (double) std::max<size_t> (1, n.size() - (n.empty() ? 0 : 1));
    m.meanAbsInterval = absSum / I;
    m.stepRatio = steps / I;
    m.leapRatio = leaps / I;
    m.octaveRatio = octs / I;
    m.repeatRatio = reps / I;

    if (! n.empty())
    {
        m.firstIsRoot = ((n.front().midiNote - root) % 12 + 12) % 12 == 0;
        const int rel = ((n.back().midiNote - root) % 12 + 12) % 12;
        m.endsResolved = rel == 0 || rel == 7 || rel == 1 || rel == 2 || rel == 10 || rel == 11;
    }

    // bar similarity vs bar 1 and distinct bars
    if (p.bars > 1)
    {
        double sum = 0, mn = 1;
        std::set<std::vector<int>> distinct;
        for (int b = 0; b < p.bars; ++b)
        {
            std::vector<int> sig;
            int inter = 0, uni = 0;
            for (int s = 0; s < kStepsPerBar; ++s)
            {
                const bool x = onsetAt[(size_t) s], y = onsetAt[(size_t) (b * kStepsPerBar + s)];
                inter += (x && y) ? 1 : 0;
                uni += (x || y) ? 1 : 0;
                sig.push_back (pitchAt[(size_t) (b * kStepsPerBar + s)]);
            }
            distinct.insert (sig);
            if (b > 0)
            {
                const double j = uni ? (double) inter / uni : 1.0;
                sum += j;
                mn = std::min (mn, j);
            }
        }
        m.barSimilarity = sum / (p.bars - 1);
        m.minBarSimilarity = mn;
        m.distinctBars = (int) distinct.size();
    }
    return m;
}

double patternOnsetSimilarity (const Pattern& a, const Pattern& b)
{
    std::set<int> sa, sb;
    for (auto& e : a.notes) sa.insert (tickToStep (e.startTick));
    for (auto& e : b.notes) sb.insert (tickToStep (e.startTick));
    int inter = 0;
    for (int x : sa) inter += sb.count (x) ? 1 : 0;
    const int uni = (int) (sa.size() + sb.size()) - inter;
    return uni == 0 ? 1.0 : (double) inter / uni;
}

double patternNoteOverlap (const Pattern& a, const Pattern& b)
{
    if (a.notes.empty()) return b.notes.empty() ? 1.0 : 0.0;
    std::set<std::pair<int, int>> sb;
    for (auto& e : b.notes) sb.insert ({ tickToStep (e.startTick), e.midiNote % 12 });
    int hit = 0;
    for (auto& e : a.notes) hit += sb.count ({ tickToStep (e.startTick), e.midiNote % 12 }) ? 1 : 0;
    return (double) hit / (double) a.notes.size();
}

std::string metricsHeaderCsv()
{
    return "bars,notes,onsets_per_bar,min_bar_onsets,max_bar_onsets,empty_bars,syncopation,offbeat_ratio,kick_collision,"
           "downbeat_ratio,mean_dur_steps,short_ratio,long_ratio,sounding_ratio,longest_rest_steps,max_chain,accent_ratio,"
           "ghost_ratio,slide_ratio,swung_ratio,distinct_pitches,pitch_range,mean_abs_interval,step_ratio,leap_ratio,"
           "octave_ratio,repeat_ratio,max_same_pitch_run,out_of_scale,root_ratio,fifth_ratio,first_is_root,ends_resolved,"
           "bar_similarity,min_bar_similarity,distinct_bars";
}

std::string metricsToCsv (const PatternMetrics& m)
{
    std::ostringstream o;
    o.precision (4);
    o << m.bars << ',' << m.noteCount << ',' << m.onsetsPerBar << ',' << m.minBarOnsets << ',' << m.maxBarOnsets << ','
      << m.emptyBars << ',' << m.syncopation << ',' << m.offbeatRatio << ',' << m.kickCollision << ',' << m.downbeatRatio << ','
      << m.meanDurSteps << ',' << m.shortRatio << ',' << m.longRatio << ',' << m.soundingRatio << ',' << m.longestRestSteps << ','
      << m.maxChain << ',' << m.accentRatio << ',' << m.ghostRatio << ',' << m.slideRatio << ',' << m.swungRatio << ','
      << m.distinctPitches << ',' << m.pitchRange << ',' << m.meanAbsInterval << ',' << m.stepRatio << ',' << m.leapRatio << ','
      << m.octaveRatio << ',' << m.repeatRatio << ',' << m.maxSamePitchRun << ',' << m.outOfScale << ',' << m.rootRatio << ','
      << m.fifthRatio << ',' << (m.firstIsRoot ? 1 : 0) << ',' << (m.endsResolved ? 1 : 0) << ',' << m.barSimilarity << ','
      << m.minBarSimilarity << ',' << m.distinctBars;
    return o.str();
}
} // namespace gfb

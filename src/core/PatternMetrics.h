#pragma once
// Measurable descriptors of a pattern. Shared by the candidate evaluator, the
// pattern_diagnostics CLI and the statistical tests so that the same numbers
// drive tuning, scoring and QA reports.

#include <array>
#include <string>
#include <vector>

#include "core/PatternTypes.h"

namespace gfb
{
struct PatternMetrics
{
    int bars = 0;
    int noteCount = 0;
    double onsetsPerBar = 0;
    int minBarOnsets = 0, maxBarOnsets = 0;
    int emptyBars = 0;

    double syncopation = 0;      // 0..1, mean metric weakness of onsets (beat 0, off-8th .5, 16th 1)
    double offbeatRatio = 0;     // onsets on off-8ths (the "and")
    double kickCollision = 0;    // onsets on beats 2,3,4 (steps 4, 8, 12) / onsets
    double downbeatRatio = 0;    // onsets on step 0 of a bar / bars
    double meanDurSteps = 0;     // mean sounding duration in 16ths
    double shortRatio = 0;       // duration < 0.8 step
    double longRatio = 0;        // duration >= 2 steps
    double soundingRatio = 0;    // fraction of the timeline with a note sounding
    double longestRestSteps = 0; // longest silence (in 16ths, wrap-aware)
    int maxChain = 0;            // longest run of onsets on consecutive 16ths
    double accentRatio = 0;
    double ghostRatio = 0;       // velocity < 70
    double slideRatio = 0;
    double swungRatio = 0;       // onsets off the straight grid (swing / microtiming)

    int distinctPitches = 0;
    int pitchRange = 0;
    double meanAbsInterval = 0;
    double stepRatio = 0;        // |interval| in 1..2
    double leapRatio = 0;        // |interval| > 7 and != 12
    double octaveRatio = 0;      // |interval| == 12
    double repeatRatio = 0;      // interval 0
    int maxSamePitchRun = 0;
    int outOfScale = 0;
    double rootRatio = 0;        // pitch class == root
    double fifthRatio = 0;
    bool firstIsRoot = false;
    bool endsResolved = false;   // last note root/fifth/octave or a step from root

    double barSimilarity = 1;    // mean onset Jaccard of each bar vs bar 1
    double minBarSimilarity = 1;
    int distinctBars = 1;        // distinct (onset+pitch) bars
};

PatternMetrics computeMetrics (const Pattern& p);

/** Onset-set similarity between two patterns of the same length (Jaccard on
    16th-step onsets). Used for STYLE vs VAR structural identity checks. */
double patternOnsetSimilarity (const Pattern& a, const Pattern& b);
/** Fraction of onsets of `a` that have the same step and pitch class in `b`. */
double patternNoteOverlap (const Pattern& a, const Pattern& b);

/** Nearest 16th step (0..bars*16-1) of a tick, tolerant to swing offsets. */
int tickToStep (int64_t tick) noexcept;

std::string metricsHeaderCsv();
std::string metricsToCsv (const PatternMetrics& m);
} // namespace gfb

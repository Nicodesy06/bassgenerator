#pragma once
// Heuristic candidate scoring. This is a proxy for musical usefulness used to
// choose between deterministic candidates; it does not replace listening.

#include <string>
#include <vector>

#include "core/PatternMetrics.h"
#include "core/RhythmGrammar.h"
#include "core/TasteProfile.h"

namespace gfb
{
struct ScoreComponent
{
    std::string name;
    double value;  // 0..1
    double weight;
};

struct ScoreResult
{
    double total = 0;            // weighted mean 0..1 (0 when rejected)
    bool rejected = false;
    std::vector<std::string> reasons; // hard rejections
    std::vector<ScoreComponent> components;
    PatternMetrics metrics;
    std::string describe() const;
};

struct EvalContext
{
    Feel feel = Feel::Balanced;    // resolved (never Random)
    Family family = Family::Sub;
    KickContext kick = KickContext::FourOnFloor;
    bool scoreMotif = true;        // false for single-bar core candidates
    const TasteProfile* taste = nullptr; // optional user taste (LIKE/DISLIKE feedback)
    const FeelProfile* profile = nullptr; // complexity-adjusted profile; null = feelProfile (feel)
};

ScoreResult evaluatePattern (const Pattern& p, const EvalContext& ctx);

/** Max consecutive 16th onsets tolerated per feel (anti machine-gun bound). */
int maxChainForFeel (Feel f) noexcept;
} // namespace gfb

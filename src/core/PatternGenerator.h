#pragma once
// Top-level deterministic generator:
//   STYLE seed -> feel resolution (Random), archetype, answer archetype,
//                 motif vocabulary, phrase template, answer type, core motif
//                 (best of N style candidates)
//   VAR seed   -> bounded realisation of the phrase (A', A'', B', fills,
//                 ending), best of M variation candidates, expression jitter
// Pure function of (settings, algorithmVersion): no globals, no clocks.

#include <string>
#include <vector>

#include "core/PatternEvaluator.h"
#include "core/PatternTypes.h"

namespace gfb
{
struct GenerationOptions
{
    int styleCandidates = 12;
    int varCandidates = 16;
    bool collectLog = false;
};

struct CandidateLog
{
    int index = 0;
    double score = 0;
    bool rejected = false;
    std::string detail;
};

struct GenerationReport
{
    Feel resolvedFeel = Feel::Balanced;
    int archetypeId = -1;
    int answerArchetypeId = -1;
    std::string phrasePlan;
    std::string answerType;
    std::string coreGrid;
    std::vector<std::string> vocabulary;
    int chosenStyle = -1, chosenVar = -1;
    double chosenScore = 0;
    int rejectedCandidates = 0;
    bool fallbackUsed = false;
    std::vector<CandidateLog> styleLog, varLog;
};

Pattern generatePattern (const GeneratorSettings& settings, const GenerationOptions& opts = {}, GenerationReport* report = nullptr);

/** Effective swing for settings (auto -> feel/archetype default). */
int effectiveSwing (const GeneratorSettings& s, Feel resolvedFeel, int archetypeMaxSwing) noexcept;
} // namespace gfb

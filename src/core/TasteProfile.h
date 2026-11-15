#pragma once
// Local, user-specific taste model learned from LIKE / DISLIKE feedback.
//
// Pure C++ and deterministic: integer statistics only, so the same profile +
// same settings + same seeds always give the same pattern. Three mechanisms:
//  1. feature statistics (onset steps, intervals, durations, density) of liked
//     vs. disliked patterns -> evaluator component "taste" (0..1, 0.5 neutral)
//  2. archetype preference: per-archetype like/dislike counts reweight the
//     STYLE archetype choice
//  3. learned cells: the 1-bar rhythm cell of every liked pattern joins a
//     personal corpus that the generator draws from (dataset expansion)
// The profile lives in a local file (FavoritesStore-like, see TasteStore) and
// is updated after every feedback; it never leaves the machine.

#include <array>
#include <map>
#include <optional>
#include <string>
#include <vector>

#include "core/PatternTypes.h"
#include "core/RhythmCorpus.h"

namespace gfb
{
struct LearnedCell
{
    std::string grid;  // 16-char corpus notation
    Feel feel = Feel::Balanced;
    Family family = Family::Sub;
    int weight = 1;    // grows when the same cell is liked again
    bool operator== (const LearnedCell&) const = default;
};

struct TasteProfile
{
    static constexpr int kMaxCells = 96;
    uint32_t revision = 0;     // increments on every feedback
    int likes = 0, dislikes = 0;
    std::array<int, 16> likeSteps {}, dislikeSteps {};
    std::array<int, 25> likeIntervals {}, dislikeIntervals {};
    std::array<int, 6> likeDur {}, dislikeDur {};
    std::array<int, 4> likeDensity {}, dislikeDensity {};
    std::map<int, std::pair<int, int>> archetypes; // id -> (likes, dislikes)
    std::vector<LearnedCell> cells;

    bool empty() const noexcept { return likes + dislikes == 0; }
    bool operator== (const TasteProfile&) const = default;
};

namespace taste
{
void addFeedback (TasteProfile& t, const Pattern& p, bool liked);
/** 0..1 preference of the pattern under the profile (0.5 = neutral / no data). */
double score (const Pattern& p, const TasteProfile& t);
/** 0..1 confidence: grows with the number of ratings (full at 8). */
double confidence (const TasteProfile& t) noexcept;
/** Archetype weight multiplier in permille (1000 = neutral), from like/dislike counts. */
int archetypePermille (const TasteProfile& t, int archetypeId) noexcept;
/** Rhythm cell of the first bar in corpus notation (nullopt if empty/out of bounds). */
std::optional<std::string> cellFromPattern (const Pattern& p);

std::string toString (const TasteProfile& t);
std::optional<TasteProfile> fromString (const std::string& s);
} // namespace taste
} // namespace gfb

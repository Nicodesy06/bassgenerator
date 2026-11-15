#pragma once
// Aggregate rhythmic/melodic priors learned from the USER's own MIDI dataset (14 files:
// 9 hand-made basslines + 5 GrooveForge outputs the user kept). Aggregates only.
// Regenerate: reference_analyzer <dataset-dir> --emit-priors <tmp>, then paste.
#include <array>

namespace gfb::priors
{
inline constexpr std::array<int, 16> kUserSteps { 804, 179, 393, 536, 214, 571, 750, 321, 571, 357, 500, 286, 357, 304, 571, 196 };
inline constexpr std::array<int, 25> kUserIntervals { 5, 0, 0, 3, 0, 21, 13, 75, 0, 35, 131, 24, 378, 27, 139, 46, 0, 64, 0, 5, 3, 0, 5, 0, 24 };
} // namespace gfb::priors

#pragma once
// Deterministic, toolchain-independent pseudo random numbers.
// - SplitMix64 for seeding / domain separation (Steele, Lea, Flood 2014)
// - xoshiro256** as stream generator (Blackman & Vigna 2018)
// - Bounded integers via Lemire's multiply-shift with exact rejection.
// No <random> distributions are used: their output differs between
// standard libraries, which would break cross-platform reproducibility.
// The musical generator only uses the integer API (below/range/chancePermille/
// pickWeighted) so results never depend on floating point rounding.

#include <cstdint>
#include <string_view>

namespace gfb
{
uint64_t splitmix64Next (uint64_t& state) noexcept;
uint64_t mix64 (uint64_t z) noexcept;
uint64_t fnv1a64 (std::string_view s) noexcept;

/** Domain-separated sub-seed: deriveSeed(seed, "rhythm", i) is independent of
    deriveSeed(seed, "pitch", i) and of other indices. */
uint64_t deriveSeed (uint64_t seed, std::string_view domain, uint64_t index = 0) noexcept;

class Rng
{
public:
    explicit Rng (uint64_t seed) noexcept;

    uint64_t next() noexcept;
    /** Uniform integer in [0, n). n == 0 returns 0. */
    uint32_t below (uint32_t n) noexcept;
    /** Uniform integer in [lo, hi] (inclusive). */
    int range (int lo, int hi) noexcept;
    /** true with probability permille/1000. */
    bool chancePermille (int permille) noexcept;
    /** Index chosen proportionally to non-negative integer weights.
        Returns count-1 fallback is never used: if all weights are 0, returns 0. */
    int pickWeighted (const int* weights, int count) noexcept;

private:
    uint64_t s[4];
};
} // namespace gfb

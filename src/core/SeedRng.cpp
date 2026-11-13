#include "core/SeedRng.h"

namespace gfb
{
uint64_t splitmix64Next (uint64_t& state) noexcept
{
    uint64_t z = (state += 0x9E3779B97F4A7C15ULL);
    z = (z ^ (z >> 30)) * 0xBF58476D1CE4E5B9ULL;
    z = (z ^ (z >> 27)) * 0x94D049BB133111EBULL;
    return z ^ (z >> 31);
}

uint64_t mix64 (uint64_t z) noexcept
{
    uint64_t st = z;
    return splitmix64Next (st);
}

uint64_t fnv1a64 (std::string_view s) noexcept
{
    uint64_t h = 1469598103934665603ULL;
    for (unsigned char c : s) { h ^= c; h *= 1099511628211ULL; }
    return h;
}

uint64_t deriveSeed (uint64_t seed, std::string_view domain, uint64_t index) noexcept
{
    uint64_t st = seed ^ mix64 (fnv1a64 (domain));
    uint64_t a = splitmix64Next (st);
    st ^= mix64 (index + 0x632BE59BD9B4E019ULL);
    return a ^ splitmix64Next (st);
}

static inline uint64_t rotl (uint64_t x, int k) noexcept { return (x << k) | (x >> (64 - k)); }

Rng::Rng (uint64_t seed) noexcept
{
    uint64_t st = seed;
    for (auto& v : s) v = splitmix64Next (st);
    if ((s[0] | s[1] | s[2] | s[3]) == 0) s[0] = 1;
}

uint64_t Rng::next() noexcept
{
    const uint64_t result = rotl (s[1] * 5, 7) * 9;
    const uint64_t t = s[1] << 17;
    s[2] ^= s[0]; s[3] ^= s[1]; s[1] ^= s[2]; s[0] ^= s[3];
    s[2] ^= t;
    s[3] = rotl (s[3], 45);
    return result;
}

uint32_t Rng::below (uint32_t n) noexcept
{
    if (n == 0) return 0;
    uint32_t x = (uint32_t) (next() >> 32);
    uint64_t m = (uint64_t) x * (uint64_t) n;
    uint32_t l = (uint32_t) m;
    if (l < n)
    {
        const uint32_t t = (uint32_t) (-n) % n;
        while (l < t)
        {
            x = (uint32_t) (next() >> 32);
            m = (uint64_t) x * (uint64_t) n;
            l = (uint32_t) m;
        }
    }
    return (uint32_t) (m >> 32);
}

int Rng::range (int lo, int hi) noexcept
{
    if (hi <= lo) return lo;
    return lo + (int) below ((uint32_t) (hi - lo + 1));
}

bool Rng::chancePermille (int permille) noexcept
{
    if (permille <= 0) return false;
    if (permille >= 1000) return true;
    return (int) below (1000) < permille;
}

int Rng::pickWeighted (const int* w, int count) noexcept
{
    int64_t total = 0;
    for (int i = 0; i < count; ++i) total += w[i] > 0 ? w[i] : 0;
    if (total <= 0 || count <= 0) return 0;
    int64_t r = (int64_t) below ((uint32_t) total);
    for (int i = 0; i < count; ++i)
    {
        const int wi = w[i] > 0 ? w[i] : 0;
        if (r < wi) return i;
        r -= wi;
    }
    return count - 1;
}
} // namespace gfb

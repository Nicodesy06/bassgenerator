#include "dsp/Filter.h"

#include <algorithm>
#include <cmath>

namespace gfb::dsp
{
namespace
{
constexpr double kPi = 3.14159265358979323846;
float prewarp (float fc, double sr) noexcept
{
    const double f = std::clamp ((double) fc, 10.0, 0.45 * sr);
    return (float) std::tan (kPi * f / sr);
}
inline float softClip (float x) noexcept
{
    // smooth, bounded (|y| < 1.5), monotonic
    const float c = std::clamp (x, -3.0f, 3.0f);
    return c * (27.0f + c * c) / (27.0f + 9.0f * c * c) * 1.0f;
}
} // namespace

void SvfLowpass::set (float cutoffHz, float resonance) noexcept
{
    g = prewarp (cutoffHz, sr);
    const float q = 0.5f + std::clamp (resonance, 0.0f, 1.0f) * 7.5f;
    k = 1.0f / q;
    a1 = 1.0f / (1.0f + g * (g + k));
    a2 = g * a1;
    a3 = g * a2;
}

float SvfLowpass::process (float v0) noexcept
{
    const float v3 = v0 - ic2;
    const float v1 = a1 * ic1 + a2 * v3;
    const float v2 = ic2 + a2 * ic1 + a3 * v3;
    ic1 = 2.0f * v1 - ic1;
    ic2 = 2.0f * v2 - ic2;
    return v2;
}

void LadderLowpass::set (float cutoffHz, float resonance) noexcept
{
    g = prewarp (cutoffHz, sr);
    G = g / (1.0f + g);
    k = 3.8f * std::clamp (resonance, 0.0f, 1.0f);
}

float LadderLowpass::process (float x) noexcept
{
    // Each one-pole stage: y = G*(u - s) + s, state update s = 2y - s  (TPT)
    // Feedback resolved through the instantaneous response of the cascade.
    const float G2 = G * G, G4 = G2 * G2;
    const float beta = 1.0f - G; // contribution factor of each state
    const float S = G * G * G * beta * s[0] + G * G * beta * s[1] + G * beta * s[2] + beta * s[3];
    // passband gain compensation keeps level roughly stable as resonance rises
    const float in = softClip (x * (1.0f + 0.5f * k));
    float u = (in - k * S) / (1.0f + k * G4);
    if (! std::isfinite (u)) { reset(); u = 0; }
    float y = u;
    for (int i = 0; i < 4; ++i)
    {
        const float v = G * (y - s[i]);
        const float out = v + s[i];
        s[i] = out + v;
        y = out;
    }
    return y;
}

void DcBlocker::prepare (double sampleRate) noexcept
{
    r = (float) std::exp (-2.0 * kPi * 10.0 / sampleRate);
    reset();
}
} // namespace gfb::dsp

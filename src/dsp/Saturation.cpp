#include "dsp/Saturation.h"

#include <cmath>

namespace gfb::dsp
{
namespace
{
constexpr double kPi = 3.14159265358979323846;
}

Oversampler2x::Oversampler2x()
{
    // windowed sinc, cutoff at 1/4 of the oversampled rate (= base Nyquist), Blackman window
    const int M = kTaps - 1;
    double sum = 0;
    for (int n = 0; n < kTaps; ++n)
    {
        const double t = n - M / 2.0;
        const double sinc = t == 0 ? 0.5 : std::sin (kPi * 0.5 * t) / (kPi * t);
        const double w = 0.42 - 0.5 * std::cos (2 * kPi * n / M) + 0.08 * std::cos (4 * kPi * n / M);
        h[(size_t) n] = (float) (sinc * w);
        sum += sinc * w;
    }
    for (auto& c : h) c = (float) (c / sum);
}

void Oversampler2x::reset() noexcept
{
    upHist.fill (0);
    downHist.fill (0);
    upPos = downPos = 0;
}

float Oversampler2x::fir (std::array<float, kTaps * 2>& hist, int& pos, float x) noexcept
{
    // double-buffered circular history => contiguous reads
    hist[(size_t) pos] = x;
    hist[(size_t) (pos + kTaps)] = x;
    float acc = 0;
    for (int i = 0; i < kTaps; ++i) acc += h[(size_t) i] * hist[(size_t) (pos + kTaps - i)];
    pos = (pos + 1) % kTaps;
    return acc;
}

void Oversampler2x::up (float x, float& y0, float& y1) noexcept
{
    y0 = 2.0f * fir (upHist, upPos, x);
    y1 = 2.0f * fir (upHist, upPos, 0.0f);
}

float Oversampler2x::down (float a, float b) noexcept
{
    fir (downHist, downPos, a);
    return fir (downHist, downPos, b);
}

void DriveStage::prepare (double sampleRate) noexcept
{
    msCoef = (float) std::exp (-1.0 / (0.06 * sampleRate));
    compCoef = (float) std::exp (-1.0 / (0.03 * sampleRate));
    reset();
}

float DriveStage::process (float x, float drive) noexcept
{
    float a, b;
    os.up (x, a, b);
    if (drive <= 0.0005f) // fully transparent path keeps latency consistent
    {
        comp = 1.0f;
        return os.down (a, b);
    }
    const float g = 1.0f + 15.0f * drive * drive;
    const float mix = drive * 4.0f < 1.0f ? drive * 4.0f : 1.0f;
    const float sa = a + mix * (std::tanh (g * a) - a);
    const float sb = b + mix * (std::tanh (g * b) - b);
    dryMs = msCoef * dryMs + (1.0f - msCoef) * 0.5f * (a * a + b * b);
    wetMs = msCoef * wetMs + (1.0f - msCoef) * 0.5f * (sa * sa + sb * sb);
    float target = 1.0f;
    if (wetMs > 1.0e-10f) target = std::sqrt (dryMs / wetMs) * (1.0f + 0.25f * drive);
    target = target < 0.05f ? 0.05f : (target > 4.0f ? 4.0f : target);
    comp = compCoef * comp + (1.0f - compCoef) * target;
    return os.down (sa * comp, sb * comp);
}
} // namespace gfb::dsp

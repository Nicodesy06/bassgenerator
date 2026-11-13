#pragma once
// Drive stage: tanh waveshaper run at 2x sample rate (31-tap windowed-sinc
// half-band interpolation/decimation) to keep aliasing of the added
// harmonics down. Trade-off: ~8 samples of latency at the base rate and a
// little CPU; documented in docs/ARCHITECTURE.md.

#include <array>

namespace gfb::dsp
{
class Oversampler2x
{
public:
    Oversampler2x();
    void reset() noexcept;
    /** produce two upsampled samples for one input sample */
    void up (float x, float& y0, float& y1) noexcept;
    /** consume two oversampled samples, return one decimated sample */
    float down (float a, float b) noexcept;

private:
    static constexpr int kTaps = 31;
    std::array<float, kTaps> h {};
    std::array<float, kTaps * 2> upHist {}, downHist {};
    int upPos = 0, downPos = 0;
    float fir (std::array<float, kTaps * 2>& hist, int& pos, float x) noexcept;
};

/** Output level compensation: RMS followers (~60 ms) on the dry and the
    shaped signal; the shaped signal is scaled to the dry RMS (+ up to +2 dB
    at full drive) so Drive changes tone, not loudness. */
class DriveStage
{
public:
    void prepare (double sampleRate) noexcept;
    void reset() noexcept { os.reset(); dryMs = wetMs = 0; comp = 1; }
    /** drive 0..1; 0 is transparent (linear), 1 is heavy saturation. */
    float process (float x, float drive) noexcept;

private:
    Oversampler2x os;
    float dryMs = 0, wetMs = 0, comp = 1, msCoef = 0.9996f, compCoef = 0.9995f;
};
} // namespace gfb::dsp

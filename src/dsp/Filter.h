#pragma once
// Zero-delay-feedback (topology preserving transform) filters after
// V. Zavalishin, "The Art of VA Filter Design". Both are unconditionally
// stable for any cutoff below Nyquist; cutoff is clamped to 0.45 * fs.

namespace gfb::dsp
{
/** 12 dB/oct state variable low-pass (used by the SUB voice). */
class SvfLowpass
{
public:
    void prepare (double sampleRate) noexcept { sr = sampleRate; reset(); }
    void reset() noexcept { ic1 = ic2 = 0; }
    /** resonance 0..1 maps to Q 0.5..~8 */
    void set (float cutoffHz, float resonance) noexcept;
    float process (float x) noexcept;

private:
    double sr = 44100;
    float g = 0.1f, k = 1.4f, a1 = 0, a2 = 0, a3 = 0;
    float ic1 = 0, ic2 = 0;
};

/** 24 dB/oct ladder low-pass with resolved linear feedback and a soft-clipped
    input stage (used by the ACID voice). resonance 0..1 -> feedback 0..3.8
    (stays just below self-oscillation; bounded and NaN-safe). */
class LadderLowpass
{
public:
    void prepare (double sampleRate) noexcept { sr = sampleRate; reset(); }
    void reset() noexcept { s[0] = s[1] = s[2] = s[3] = 0; }
    void set (float cutoffHz, float resonance) noexcept;
    float process (float x) noexcept;

private:
    double sr = 44100;
    float G = 0.1f, g = 0.1f, k = 0.0f;
    float s[4] { 0, 0, 0, 0 };
};

/** One-pole DC blocker (~10 Hz). */
class DcBlocker
{
public:
    void prepare (double sampleRate) noexcept;
    void reset() noexcept { x1 = y1 = 0; }
    float process (float x) noexcept { const float y = x - x1 + r * y1; x1 = x; y1 = y; return y; }

private:
    float r = 0.999f, x1 = 0, y1 = 0;
};
} // namespace gfb::dsp

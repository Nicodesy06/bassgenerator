#pragma once
// Time-based envelopes (sample-rate independent). Exponential segments with
// a minimum attack/release to prevent clicks.

namespace gfb::dsp
{
class AmpEnvelope
{
public:
    void prepare (double sampleRate) noexcept;
    void setTimes (float attackSec, float decaySec, float sustain, float releaseSec) noexcept;
    /** Retrigger from the current level (no reset to zero: click-free). */
    void noteOn() noexcept;
    void noteOff() noexcept;
    void reset() noexcept { level = 0; stage = Stage::Idle; }
    float next() noexcept;
    bool isActive() const noexcept { return stage != Stage::Idle; }
    bool isGated() const noexcept { return stage == Stage::Attack || stage == Stage::Decay || stage == Stage::Sustain; }
    float getLevel() const noexcept { return level; }

private:
    enum class Stage { Idle, Attack, Decay, Sustain, Release };
    static float coefFor (float seconds, double sr) noexcept;
    double sr = 44100;
    Stage stage = Stage::Idle;
    float level = 0, sustain = 0.7f;
    float attackInc = 0.01f, decayCoef = 0.999f, releaseCoef = 0.99f;
};

/** Attack-decay envelope used for filter modulation and accent. */
class DecayEnvelope
{
public:
    void prepare (double sampleRate) noexcept { sr = sampleRate; }
    void setDecay (float seconds) noexcept;
    void trigger (float peak) noexcept { level = peak; }
    void reset() noexcept { level = 0; }
    float next() noexcept { const float v = level; level *= coef; if (level < 1.0e-6f) level = 0; return v; }
    float getLevel() const noexcept { return level; }

private:
    double sr = 44100;
    float level = 0, coef = 0.999f;
};
} // namespace gfb::dsp

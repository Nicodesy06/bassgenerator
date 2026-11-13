#include "dsp/Envelope.h"

#include <algorithm>
#include <cmath>

namespace gfb::dsp
{
float AmpEnvelope::coefFor (float seconds, double sampleRate) noexcept
{
    // time to fall to ~-60 dB (1/1000)
    const double n = std::max (1.0, (double) seconds * sampleRate);
    return (float) std::exp (std::log (0.001) / n);
}

void AmpEnvelope::prepare (double sampleRate) noexcept { sr = sampleRate; }

void AmpEnvelope::setTimes (float a, float d, float s, float r) noexcept
{
    a = std::max (a, 0.0008f);
    r = std::max (r, 0.004f);
    attackInc = (float) (1.0 / std::max (1.0, (double) a * sr));
    decayCoef = coefFor (std::max (d, 0.005f), sr);
    sustain = std::clamp (s, 0.0f, 1.0f);
    releaseCoef = coefFor (r, sr);
}

void AmpEnvelope::noteOn() noexcept { stage = Stage::Attack; }

void AmpEnvelope::noteOff() noexcept
{
    if (stage != Stage::Idle) stage = Stage::Release;
}

float AmpEnvelope::next() noexcept
{
    switch (stage)
    {
        case Stage::Idle: return 0.0f;
        case Stage::Attack:
            level += attackInc;
            if (level >= 1.0f) { level = 1.0f; stage = Stage::Decay; }
            break;
        case Stage::Decay:
            level = sustain + (level - sustain) * decayCoef;
            if (level - sustain < 1.0e-4f) { level = sustain; stage = Stage::Sustain; }
            break;
        case Stage::Sustain: level = sustain; break;
        case Stage::Release:
            level *= releaseCoef;
            if (level < 1.0e-5f) { level = 0; stage = Stage::Idle; }
            break;
    }
    return level;
}

void DecayEnvelope::setDecay (float seconds) noexcept
{
    const double n = std::max (1.0, (double) std::max (seconds, 0.002f) * sr);
    coef = (float) std::exp (std::log (0.001) / n);
}
} // namespace gfb::dsp

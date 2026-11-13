#include "dsp/BassVoice.h"

#include <algorithm>
#include <cmath>

namespace gfb::dsp
{
namespace
{
constexpr double kTwoPi = 6.283185307179586;

inline float polyBlep (double t, double dt) noexcept
{
    if (t < dt) { t /= dt; return (float) (t + t - t * t - 1.0); }
    if (t > 1.0 - dt) { t = (t - 1.0) / dt; return (float) (t * t + t + t + 1.0); }
    return 0.0f;
}
} // namespace

void BassVoice::prepare (double sampleRate) noexcept
{
    sr = sampleRate;
    amp.prepare (sr);
    fenv.prepare (sr);
    accentEnv.prepare (sr);
    punchEnv.prepare (sr);
    svf.prepare (sr);
    ladder.prepare (sr);
    dc.prepare (sr);
    drive.prepare (sr);
    smoothCoef = (float) std::exp (-1.0 / (0.005 * sr)); // 5 ms parameter smoothing
    reset();
}

void BassVoice::reset() noexcept
{
    amp.reset();
    fenv.reset();
    accentEnv.reset();
    punchEnv.reset();
    svf.reset();
    ladder.reset();
    drive.reset();
    dc.reset();
    note = -1;
    phase = sinePhase = 0;
    firstBlock = true;
}

void BassVoice::applyEnvelopeTimes (const SynthParams& p) noexcept
{
    const float d = synthmap::decaySeconds (p.decay);
    const bool acid = p.engine == SynthEngine::Acid;
    const float accentShorten = accentNote ? (1.0f - 0.45f * p.accent) : 1.0f;
    fenv.setDecay (d * accentShorten);
    accentEnv.setDecay (acid ? 0.18f : 0.08f);
    punchEnv.setDecay (0.012f);
    if (acid) amp.setTimes (accentNote ? 0.0009f : 0.0015f, d * 2.0f, 0.55f, 0.010f);
    else      amp.setTimes (accentNote ? 0.0010f : 0.0020f, d * 1.5f, 0.78f, 0.016f);
}

void BassVoice::noteOn (int midiNote, float vel, bool accent, bool legato, const SynthParams& p) noexcept
{
    const bool glide = legato && amp.isGated() && note >= 0;
    note = std::clamp (midiNote, 0, 127);
    targetSemis = (float) note;
    velocity = std::clamp (vel, 0.0f, 1.0f);
    if (glide)
    {
        glideCoef = (float) std::exp (-1.0 / (synthmap::glideSeconds (p.glide) * sr / 3.0));
        if (accent) accentEnv.trigger (1.0f); // accented slides still bite
        accentNote = accent;
        return; // no envelope retrigger on a slide
    }
    if (! amp.isActive()) semis = targetSemis;
    else semis = targetSemis; // non-slide: jump (phase stays continuous => click-free)
    glideCoef = 0.0f;
    accentNote = accent;
    applyEnvelopeTimes (p);
    amp.noteOn();
    fenv.trigger (1.0f);
    accentEnv.trigger (accent ? 1.0f : 0.0f);
    punchEnv.trigger (1.0f);
}

void BassVoice::noteOff (int midiNote) noexcept
{
    if (midiNote == note) amp.noteOff();
}

void BassVoice::allNotesOff (bool hard) noexcept
{
    if (hard) amp.setTimes (0.001f, 0.1f, 0.0f, 0.004f);
    amp.noteOff();
}

void BassVoice::render (float* out, int n, const SynthParams& p) noexcept
{
    if (firstBlock)
    {
        sCutoff = p.cutoff; sRes = p.resonance; sEnvMod = p.envMod; sDrive = p.drive; sShape = p.shape; sAccent = p.accent;
        firstBlock = false;
    }
    const bool acid = p.engine == SynthEngine::Acid;
    const float c = smoothCoef, ic = 1.0f - smoothCoef;
    for (int i = 0; i < n; ++i)
    {
        sCutoff = sCutoff * c + p.cutoff * ic;
        sRes = sRes * c + p.resonance * ic;
        sEnvMod = sEnvMod * c + p.envMod * ic;
        sDrive = sDrive * c + p.drive * ic;
        sShape = sShape * c + p.shape * ic;
        sAccent = sAccent * c + p.accent * ic;

        if (! amp.isActive())
        {
            out[i] = dc.process (drive.process (0.0f, sDrive));
            continue;
        }
        if (glideCoef > 0.0f) semis = targetSemis + (semis - targetSemis) * glideCoef;
        const float a = amp.next();
        const float fe = fenv.next();
        const float ae = accentEnv.next();
        const float pe = punchEnv.next();

        const double freq = 440.0 * std::exp2 ((semis - 69.0) / 12.0);
        const double dt = std::min (0.45, freq / sr);
        // band-limited saw/square
        const float saw = (float) (2.0 * phase - 1.0) - polyBlep (phase, dt);
        double ph2 = phase + 0.5; if (ph2 >= 1.0) ph2 -= 1.0;
        const float sq = (phase < 0.5 ? 1.0f : -1.0f) + polyBlep (phase, dt) - polyBlep (ph2, dt);
        const float osc = saw + (sq - saw) * sShape;
        phase += dt; if (phase >= 1.0) phase -= 1.0;

        const float accAmt = accentNote ? sAccent : 0.0f;
        float voice;
        if (acid)
        {
            const float oct = sEnvMod * fe * (4.5f + 2.5f * accAmt) + accAmt * ae * 2.0f;
            ladder.set (synthmap::cutoffHz (sCutoff) * 1.6f * std::exp2 (oct), sRes * (0.85f + 0.15f * accAmt));
            voice = ladder.process (osc * 0.7f) * 2.4f; // level-matched to the SUB engine (measured with render_demo)
        }
        else
        {
            // punchy sine core: short downward pitch transient on attack
            const double pf = freq * std::exp2 (pe * (3.0 + 5.0 * accAmt) / 12.0);
            sinePhase += pf / sr; if (sinePhase >= 1.0) sinePhase -= 1.0;
            const float sine = (float) std::sin (kTwoPi * sinePhase);
            const float oct = sEnvMod * fe * (3.5f + 1.5f * accAmt);
            svf.set (synthmap::cutoffHz (sCutoff) * std::exp2 (oct), sRes * 0.7f);
            const float harm = svf.process (osc);
            voice = 0.62f * sine + harm * (0.10f + 0.42f * sShape);
        }
        const float gain = a * (0.55f + 0.45f * velocity) * (1.0f + 0.6f * accAmt);
        const float driven = drive.process (voice * gain, std::min (1.0f, sDrive + 0.25f * accAmt * sDrive));
        float y = dc.process (driven);
        if (! std::isfinite (y)) { reset(); y = 0.0f; }
        out[i] = y;
    }
}
} // namespace gfb::dsp

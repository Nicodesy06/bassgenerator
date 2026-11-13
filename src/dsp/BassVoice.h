#pragma once
// Monophonic legato bass voice with two original engines:
//  SUB : phase-continuous sine core (bypasses the filter for a clean
//        fundamental) + polyBLEP saw/square harmonic layer through a 12 dB SVF,
//        short pitch "punch" transient on attack.
//  ACID: polyBLEP saw/square morph into a resonant 24 dB TPT ladder with
//        accent-boosted envelope sweep. Acid-inspired, not a circuit model.

#include "dsp/Envelope.h"
#include "dsp/Filter.h"
#include "dsp/Saturation.h"
#include "dsp/SynthParams.h"

namespace gfb::dsp
{
class BassVoice
{
public:
    void prepare (double sampleRate) noexcept;
    void reset() noexcept;

    /** legato=true glides from the current pitch without retriggering the
        envelopes (only honoured while the voice is gated). */
    void noteOn (int midiNote, float velocity01, bool accent, bool legato, const SynthParams& p) noexcept;
    /** Releases only if `midiNote` is the sounding note (mono semantics). */
    void noteOff (int midiNote) noexcept;
    /** Release whatever is sounding (fast if `hard`). */
    void allNotesOff (bool hard) noexcept;

    /** Render `n` samples (overwrites `out`). Parameter targets are smoothed internally. */
    void render (float* out, int n, const SynthParams& p) noexcept;

    bool isActive() const noexcept { return amp.isActive(); }
    bool isGated() const noexcept { return amp.isGated(); }
    int currentNote() const noexcept { return note; }
    float currentPitchSemis() const noexcept { return semis; }

private:
    void applyEnvelopeTimes (const SynthParams& p) noexcept;

    double sr = 44100.0;
    int note = -1;
    float semis = 36.0f, targetSemis = 36.0f, glideCoef = 0.0f;
    float velocity = 0.8f;
    bool accentNote = false;
    double phase = 0, sinePhase = 0;

    AmpEnvelope amp;
    DecayEnvelope fenv, accentEnv, punchEnv;
    SvfLowpass svf;
    LadderLowpass ladder;
    DriveStage drive;
    DcBlocker dc;

    // smoothed parameters
    float sCutoff = 0.5f, sRes = 0.2f, sEnvMod = 0.4f, sDrive = 0.1f, sShape = 0.3f, sAccent = 0.5f;
    float smoothCoef = 0.999f;
    bool firstBlock = true;
};
} // namespace gfb::dsp

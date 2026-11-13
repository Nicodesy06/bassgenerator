#pragma once
// Event-driven wrapper around BassVoice: renders sample-accurately between
// events, applies smoothed output volume and produces a stereo pair with an
// identical (mono-compatible) low end. Real-time safe: no allocation, locks
// or I/O in process().

#include <cstdint>

#include "dsp/BassVoice.h"

namespace gfb
{
struct SynthEvent
{
    enum class Type : uint8_t { NoteOn, NoteOff, AllNotesOff };
    int sampleOffset = 0;
    Type type = Type::NoteOn;
    uint8_t note = 0;
    uint8_t velocity = 100;
    bool accent = false;
    bool legato = false;
};

namespace dsp
{
class BassSynth
{
public:
    static constexpr int kMaxBlock = 8192;
    void prepare (double sampleRate, int maxBlockSize) noexcept;
    void reset() noexcept;
    /** events must be sorted by sampleOffset (0..numSamples-1). */
    void process (float* left, float* right, int numSamples, const SynthEvent* events, int numEvents, const SynthParams& p) noexcept;
    bool isActive() const noexcept { return voice.isActive(); }
    const BassVoice& getVoice() const noexcept { return voice; }

private:
    void handle (const SynthEvent& e, const SynthParams& p) noexcept;
    BassVoice voice;
    double sr = 44100;
    float volume = 0.5f, volCoef = 0.999f;
    bool volInit = false;
    float scratch[kMaxBlock] {};
};
} // namespace dsp
} // namespace gfb

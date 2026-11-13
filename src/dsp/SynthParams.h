#pragma once
// Plain synth parameter block (normalised 0..1 macros) and factory presets.
// Mapping functions are shared by the DSP and the UI value labels.

#include <array>
#include <string_view>

namespace gfb
{
enum class SynthEngine : int { Sub = 0, Acid = 1 };

struct SynthParams
{
    float volumeDb = -6.0f;  // -60..+6 dB
    float cutoff = 0.55f;    // 0..1 -> 30 Hz..12 kHz (log)
    float resonance = 0.25f; // 0..1
    float envMod = 0.45f;    // 0..1 filter envelope depth (0..6 octaves)
    float decay = 0.45f;     // 0..1 -> 30 ms..2.5 s (log)
    float accent = 0.5f;     // 0..1
    float drive = 0.15f;     // 0..1
    float glide = 0.35f;     // 0..1 -> 5 ms..500 ms (log)
    float shape = 0.3f;      // 0..1 SUB: harmonic layer amount/saw->square; ACID: saw->square
    SynthEngine engine = SynthEngine::Sub;
};

namespace synthmap
{
float cutoffHz (float norm) noexcept;
float decaySeconds (float norm) noexcept;
float glideSeconds (float norm) noexcept;
float dbToGain (float db) noexcept;
} // namespace synthmap

struct SynthPreset
{
    std::string_view name;
    SynthParams params;
};

std::array<SynthPreset, 8> factoryPresets() noexcept;
} // namespace gfb

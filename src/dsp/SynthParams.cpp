#include "dsp/SynthParams.h"

#include <algorithm>
#include <cmath>

namespace gfb
{
namespace synthmap
{
float cutoffHz (float n) noexcept { return 30.0f * std::pow (400.0f, std::clamp (n, 0.0f, 1.0f)); }
float decaySeconds (float n) noexcept { return 0.03f * std::pow (2.5f / 0.03f, std::clamp (n, 0.0f, 1.0f)); }
float glideSeconds (float n) noexcept { return 0.005f * std::pow (100.0f, std::clamp (n, 0.0f, 1.0f)); }
float dbToGain (float db) noexcept { return db <= -59.9f ? 0.0f : std::pow (10.0f, db / 20.0f); }
} // namespace synthmap

std::array<SynthPreset, 8> factoryPresets() noexcept
{
    using E = SynthEngine;
    //                     vol    cut   res   env   dec   acc   drv   gli   shp   engine
    return { {
        { "Round Sub",         { -6.0f, 0.38f, 0.10f, 0.20f, 0.40f, 0.40f, 0.05f, 0.30f, 0.10f, E::Sub } },
        { "Elastic House",     { -6.0f, 0.48f, 0.30f, 0.45f, 0.35f, 0.55f, 0.20f, 0.45f, 0.35f, E::Sub } },
        { "Punch Bass",        { -6.0f, 0.55f, 0.20f, 0.55f, 0.22f, 0.70f, 0.35f, 0.25f, 0.55f, E::Sub } },
        { "Warm Analog",       { -6.0f, 0.45f, 0.35f, 0.30f, 0.60f, 0.35f, 0.40f, 0.35f, 0.75f, E::Sub } },
        { "Rubber Acid",       { -8.0f, 0.30f, 0.62f, 0.55f, 0.40f, 0.60f, 0.25f, 0.45f, 0.00f, E::Acid } },
        { "Sharp Acid",        { -9.0f, 0.42f, 0.78f, 0.65f, 0.25f, 0.75f, 0.45f, 0.35f, 0.00f, E::Acid } },
        { "Dark Acid",         { -8.0f, 0.20f, 0.55f, 0.40f, 0.55f, 0.50f, 0.30f, 0.50f, 1.00f, E::Acid } },
        { "Resonant Sequence", { -9.0f, 0.35f, 0.86f, 0.70f, 0.30f, 0.65f, 0.15f, 0.30f, 0.30f, E::Acid } },
    } };
}
} // namespace gfb

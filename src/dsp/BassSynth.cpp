#include "dsp/BassSynth.h"

#include <algorithm>
#include <cmath>

namespace gfb::dsp
{
void BassSynth::prepare (double sampleRate, int) noexcept
{
    sr = sampleRate;
    voice.prepare (sr);
    volCoef = (float) std::exp (-1.0 / (0.02 * sr));
    volInit = false;
}

void BassSynth::reset() noexcept { voice.reset(); volInit = false; }

void BassSynth::handle (const SynthEvent& e, const SynthParams& p) noexcept
{
    switch (e.type)
    {
        case SynthEvent::Type::NoteOn: voice.noteOn (e.note, (float) e.velocity / 127.0f, e.accent, e.legato, p); break;
        case SynthEvent::Type::NoteOff: voice.noteOff (e.note); break;
        case SynthEvent::Type::AllNotesOff: voice.allNotesOff (true); break;
    }
}

void BassSynth::process (float* left, float* right, int n, const SynthEvent* ev, int numEv, const SynthParams& p) noexcept
{
    const float target = synthmap::dbToGain (p.volumeDb);
    if (! volInit) { volume = target; volInit = true; }
    int pos = 0, e = 0;
    while (pos < n)
    {
        while (e < numEv && ev[e].sampleOffset <= pos) handle (ev[e++], p);
        int end = n;
        if (e < numEv) end = std::clamp (ev[e].sampleOffset, pos + 1, n);
        end = std::min (end, pos + kMaxBlock);
        const int len = end - pos;
        voice.render (scratch, len, p);
        for (int i = 0; i < len; ++i)
        {
            volume = target + (volume - target) * volCoef;
            const float y = std::clamp (scratch[i] * volume, -2.0f, 2.0f); // hard safety ceiling (+6 dBFS)
            left[pos + i] = y;
            if (right != nullptr) right[pos + i] = y;
        }
        pos = end;
    }
    while (e < numEv) handle (ev[e++], p); // events at/after the end (defensive)
}
} // namespace gfb::dsp

// Offline CPU benchmark of the audio path (scheduler + synth), per engine and block size.
#include <chrono>
#include <cstdio>
#include <memory>
#include <vector>

#include "core/PatternGenerator.h"
#include "dsp/BassSynth.h"
#include "dsp/NoteScheduler.h"

using namespace gfb;

int main()
{
    std::printf ("| Engine | Sample rate | Block | Real-time factor | CPU %% of one core |\n|---|---|---|---|---|\n");
    for (int eng = 0; eng < 2; ++eng)
        for (double sr : { 48000.0, 96000.0 })
            for (int bs : { 32, 128, 512 })
            {
                GeneratorSettings s;
                s.family = (Family) eng; s.feel = Feel::Busy; s.bars = 4;
                const auto p = generatePattern (s);
                auto sched = std::make_unique<NoteSchedule>();
                sched->fromPattern (p, 1);
                NoteScheduler sc; sc.prepare (sr);
                auto syn = std::make_unique<dsp::BassSynth>(); syn->prepare (sr, bs);
                SynthParams prm; prm.engine = (SynthEngine) eng; prm.drive = 0.6f; prm.resonance = 0.7f;
                std::vector<float> l ((size_t) bs), r ((size_t) bs);
                SynthEvent ev[NoteScheduler::kMaxEvents];
                const int blocks = (int) (sr * 30.0 / bs); // 30 s of audio
                double ppq = 0;
                const auto t0 = std::chrono::steady_clock::now();
                for (int i = 0; i < blocks; ++i)
                {
                    const int n = sc.process (*sched, { true, ppq, 126.0 }, bs, ev, NoteScheduler::kMaxEvents);
                    syn->process (l.data(), r.data(), bs, ev, n, prm);
                    ppq += bs * 126.0 / 60.0 / sr;
                }
                const double secs = std::chrono::duration<double> (std::chrono::steady_clock::now() - t0).count();
                std::printf ("| %s | %.0f | %d | %.0fx | %.2f %% |\n", eng ? "ACID" : "SUB", sr, bs, 30.0 / secs, secs / 30.0 * 100.0);
            }
}

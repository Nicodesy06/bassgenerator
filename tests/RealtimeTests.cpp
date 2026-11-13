// Verifies that the audio path (scheduler + synth) never allocates.
#include <atomic>
#include <cstdlib>
#include <new>

#include "TestFramework.h"
#include "core/PatternGenerator.h"
#include "dsp/NoteScheduler.h"

namespace
{
std::atomic<bool> gTrack { false };
std::atomic<int> gAllocs { 0 };
} // namespace

void* operator new (std::size_t n)
{
    if (gTrack.load (std::memory_order_relaxed)) gAllocs.fetch_add (1);
    if (void* p = std::malloc (n ? n : 1)) return p;
    throw std::bad_alloc();
}
void* operator new[] (std::size_t n)
{
    if (gTrack.load (std::memory_order_relaxed)) gAllocs.fetch_add (1);
    if (void* p = std::malloc (n ? n : 1)) return p;
    throw std::bad_alloc();
}
void operator delete (void* p) noexcept { std::free (p); }
void operator delete[] (void* p) noexcept { std::free (p); }
void operator delete (void* p, std::size_t) noexcept { std::free (p); }
void operator delete[] (void* p, std::size_t) noexcept { std::free (p); }

using namespace gfb;

TEST_CASE (Realtime, AudioPathDoesNotAllocate)
{
    GeneratorSettings s;
    s.bars = 8; s.feel = Feel::Busy; s.family = Family::Acid;
    const auto p = generatePattern (s);
    auto* sched = new NoteSchedule();
    sched->fromPattern (p, 1);
    NoteScheduler sc;
    sc.prepare (48000);
    auto* syn = new dsp::BassSynth();
    syn->prepare (48000, 512);
    std::vector<float> l (512), r (512);
    SynthEvent ev[NoteScheduler::kMaxEvents];
    SynthParams params;
    params.engine = SynthEngine::Acid;
    double ppq = 0;
    gAllocs = 0;
    gTrack = true;
    for (int i = 0; i < 4000; ++i)
    {
        TransportState t { i % 1000 != 999, ppq, 125.0 };
        if (i == 2000) ppq = 3.0; // seek
        const int n = sc.process (*sched, t, 512, ev, NoteScheduler::kMaxEvents);
        syn->process (l.data(), r.data(), 512, ev, n, params);
        ppq += 512 * 125.0 / 60.0 / 48000.0;
    }
    gTrack = false;
    CHECK_EQ (gAllocs.load(), 0);
    delete syn;
    delete sched;
}

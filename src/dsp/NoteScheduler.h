#pragma once
// Converts the canonical PPQ pattern into sample-accurate synth events from
// the *host musical position of every block* (never by counting blocks), so
// seeks, loops and tempo changes stay correct. Real-time safe: fixed-size
// storage only.
//
// Policies:
//  - events in half-open tick range [blockStart, blockEnd): a note exactly on
//    a block boundary fires exactly once
//  - transport stop, position jump (seek/loop), or pattern swap => note-off
//    for every sounding note at offset 0, then continue from the new position
//  - starting playback in the middle of a note does not retrigger that note
//  - same-tick ordering: note-off before note-on; slide overlaps produce
//    note-on (legato) before the previous note's note-off
//  - tempo is taken from each block (block-constant)

#include <array>
#include <cstdint>

#include "core/PatternTypes.h"
#include "dsp/BassSynth.h"

namespace gfb
{
struct ScheduledNote
{
    int64_t start = 0, end = 0;
    uint8_t note = 0, velocity = 100;
    bool accent = false, slide = false;
};

/** Fixed-capacity, trivially copyable snapshot handed to the audio thread. */
struct NoteSchedule
{
    std::array<ScheduledNote, kMaxNotesPerPattern> notes {};
    int count = 0;
    int64_t lengthTicks = kTicksPerBar;
    uint64_t version = 0;

    void fromPattern (const Pattern& p, uint64_t newVersion) noexcept;
};

struct TransportState
{
    bool playing = false;
    double ppqPosition = 0.0; // quarter notes
    double bpm = 120.0;
};

class NoteScheduler
{
public:
    static constexpr int kMaxEvents = 512;
    static constexpr int kMaxActive = 8;

    void prepare (double sampleRate) noexcept { sr = sampleRate; reset(); }
    void reset() noexcept { wasPlaying = false; activeCount = 0; lastEndTick = 0; lastVersion = 0; }

    /** Returns number of events written to `out` (sorted by sampleOffset). */
    int process (const NoteSchedule& sched, const TransportState& t, int numSamples, SynthEvent* out, int maxOut) noexcept;

    /** Note-offs for everything sounding (e.g. on bypass/editor reset). */
    int flush (SynthEvent* out, int maxOut, int offset = 0) noexcept;

    int activeNotes() const noexcept { return activeCount; }
    bool isPlaying() const noexcept { return wasPlaying; }

private:
    struct Pending { double tick; int order; SynthEvent ev; };
    void addActive (uint8_t n) noexcept;
    bool removeActive (uint8_t n) noexcept;

    double sr = 44100.0;
    bool wasPlaying = false;
    double lastEndTick = 0;
    uint64_t lastVersion = 0;
    std::array<uint8_t, kMaxActive> active {};
    int activeCount = 0;
    std::array<Pending, kMaxEvents> pending {};
};
} // namespace gfb

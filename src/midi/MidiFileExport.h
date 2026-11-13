#pragma once
// Standard MIDI File writer (format 0, PPQ 960) and a strict minimal reader
// used for round-trip tests. Deterministic byte output for a given pattern.
//
// Content: track name, 4/4 time signature, tempo, optional key signature,
// note-on/off pairs (note-off as 0x80 with velocity 64), end-of-track at
// exactly bars * 3840 ticks. At equal ticks note-offs precede note-ons so a
// repeated same pitch never produces a stuck or swallowed note. Slides are
// encoded only as a short overlap (legato) - third-party synths may or may
// not glide on that; no pitch-bend is written in v1.

#include <cstdint>
#include <optional>
#include <string>
#include <vector>

#include "core/PatternTypes.h"

namespace gfb::midi
{
struct ExportOptions
{
    double tempoBpm = 124.0;
    int channel = 0;              // 0..15
    std::string trackName = "GrooveForge Bass";
    bool writeKeySignature = true;
};

std::vector<uint8_t> writeSmf (const Pattern& p, const ExportOptions& opts);

struct ParsedMidi
{
    int format = -1;
    int ppq = 0;
    int numTracks = 0;
    double tempoBpm = 0;
    int timeSigNum = 0, timeSigDen = 0;
    int64_t endOfTrackTick = -1;
    std::vector<NoteEvent> notes; // paired on/off, sorted by start
    std::string error;            // non-empty on parse failure
};

ParsedMidi readSmf (const std::vector<uint8_t>& bytes);

/** Suggested ASCII-safe file name, e.g. "GFB_Am_Dorian_ACID_ROLLING_4bar_S123_V45.mid". */
std::string suggestedFileName (const Pattern& p);
} // namespace gfb::midi

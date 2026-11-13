# Architecture

## Module boundaries

```
src/core   (pure C++20, no JUCE)  generator, theory, corpus, metrics, history, serialization
src/dsp    (pure C++20, no JUCE)  BassVoice, BassSynth, filters, envelopes, drive, NoteScheduler
src/midi   MidiFileExport (pure C++ SMF writer/reader) + MidiDragSource (JUCE file I/O)
src/state  StateCodec (project state, juce::ValueTree), FavoritesStore (JSON, juce_core)
src/ui     JUCE components: DesignSystem, PianoRoll, SeedBox, GeneratorPanel/ActionBar, SynthPanel, BrowserPanel
src/PluginProcessor / PluginEditor   glue
tests/     gfb_tests (pure), gfb_juce_tests, gfb_host_test (loads the built .vst3), gfb_ui_tests
tools/     pattern_diagnostics, render_demo
```

The generator, DSP and SMF writer are a static library `gfb_core` that builds and tests without JUCE, an audio device or a GUI.

## Canonical data

`Pattern` (src/core/PatternTypes.h): PPQ 960, bar = 3840 ticks, 16th = 240 ticks, integer ticks only. One immutable `Pattern` instance drives the piano roll, the synth schedule, MIDI out, `.mid` export and drag & drop — there is no divergent recomputation. Invariants are centralised in `validatePattern()` (sorted, unique starts / monophonic, valid ranges, end ≤ loop length, overlap only for `slideToNext` onto a different pitch and ≤ 60 ticks, last note never slides). `algorithmVersion` (=1) is stored with every pattern; project load restores stored notes verbatim and never regenerates unless the stored data is unusable.

## Threads

| Thread | Work |
|---|---|
| Message thread | UI, generation (p99 ≈ 0.4 ms, max ≈ 0.5–2.4 ms measured), favorites I/O, file export, drag file creation, state get/set |
| Audio thread | `processBlock`: read parameters (atomics), acquire schedule, NoteScheduler, BassSynth, MIDI out, meter/playhead atomics |

Generation runs on the message thread because it is sub-millisecond (see `reports/diagnostics_summary.md`); it never blocks the audio thread.

### Pattern hand-off (no locks, no allocation, no frees on the audio thread)

`ScheduleExchange` (PluginProcessor.h) is a single-writer/single-reader **triple buffer** of fixed-size `NoteSchedule` objects (`std::array<ScheduledNote, 512>`). The writer (message thread, serialised by a mutex the audio thread never touches) fills the back buffer and atomically swaps it with the "middle" index + dirty bit. The audio thread swaps middle↔front only when the dirty bit is set (wait-free). All three buffers are members, so nothing is ever deallocated on the audio thread. `tests/RealtimeTests.cpp` overrides global `operator new` and asserts zero allocations during 4 000 scheduler+synth blocks (including seeks and transport stops).

Generator/instance state (`InstanceState`, `PatternHistory`) is guarded by a `std::mutex` used only by non-audio threads.

## Transport and scheduling (src/dsp/NoteScheduler)

- Every block uses the **host PPQ position and tempo of that block** (never block counting), so seeks, cycle loops and tempo automation are followed.
- Events are taken from the half-open tick range `[blockStart, blockEnd)` with an epsilon-consistent sample-domain test: an event exactly on a block boundary fires once (tested: `Scheduler.BoundaryEventFiresExactlyOnce`, block sizes 32…1024).
- Continuity: if the new block start is within 2 samples of the previous end it is snapped to it (no gaps/double fires); a larger jump = seek/loop ⇒ note-off for all sounding notes at offset 0.
- Transport stop ⇒ note-offs. Pattern swap while running ⇒ note-offs, continue at the same musical position with the new pattern.
- Starting inside a note does not retrigger it (documented choice).
- At equal ticks note-off precedes note-on; slides produce the next note-on (legato flag) before the previous note-off.
- Tempo is treated as constant within a block (VST3 provides block-level tempo through JUCE).
- Modes: **SYNC** (follow host when it plays), **PREVIEW** (internal transport when the host is stopped; host tempo if reported, else the fallback BPM shown in the UI, default 124). Long previews wrap the internal PPQ to keep precision.
- Live MIDI input notes are merged and play the synth (useful for auditioning dragged clips routed back).

## Synth (src/dsp)

Mono legato voice. SUB: phase-continuous sine core (bypasses the filter, clean fundamental) + polyBLEP saw/square layer → 12 dB TPT SVF; 12 ms pitch "punch" on attack scaled by accent. ACID: polyBLEP saw→square morph → 24 dB TPT ladder (resolved linear feedback, soft-clipped input, k ≤ 3.8 i.e. below self-oscillation) with accent-boosted envelope sweep. Shared: exponential amp ADSR (≥0.8 ms attack, ≥4 ms release, retrigger from current level), filter AD envelope, glide in log-frequency domain only for legato/slide notes, 5 ms one-pole smoothing of all macros, 20 ms volume smoothing, **2× oversampled tanh drive** (31-tap Blackman windowed-sinc half-band up/down) with RMS-matched output compensation, 10 Hz DC blocker, NaN guard, ±2.0 hard ceiling. Output is identical L/R (mono-compatible low end). Latency of the oversampler (15 samples) is reported to the host.

Parameter IDs (stable, versioned `ParameterID{…, 1}`): `volume cutoff resonance envmod decay accent drive glide shape engine`. JUCE's VST3 MIDI-CC emulation parameters are disabled so the host sees only real parameters.

## State

- **Project (per instance)**: VST3 chunk = XML with the APVTS parameters + `GrooveForgeState` (schema 1): settings, locks, exact current notes, 64-entry history with cursor, preset, sync/MIDI-out/preview-BPM options, editor size, drawer state. Decoding is tolerant field-by-field; unknown future schema ⇒ ignored; corrupt chunk ⇒ current state kept (tested in host harness).
- **Favorites (global)**: `%APPDATA%\GrooveForgeBass\favorites.json`; read-modify-write under `juce::InterProcessLock` + a process-wide mutex, temp file + flush + `replaceFileIn`, last good file kept as `favorites.bak.json`, corrupt files moved aside, dedupe by notes+settings. Editors poll the file once per second to show other instances' changes.
- **Drag cache**: `%TEMP%\GrooveForgeBass\drag\GFB_*.mid`, written before the drag, unique per content+tempo, never deleted during a session; only files older than 24 h are removed at plugin start.

## GUI

Native JUCE components, vector drawing only (scales with Windows DPI / host scale factor). Resizable 1100×640 … 2600×1700, size persisted. Editor polls processor atomics at 30 Hz (state version, playhead, peak meter, transport). Keyboard: G/S/V/R/F/P, ←/→, Ctrl+Z/Y; all controls focusable with visible focus rings; tooltips on every control.

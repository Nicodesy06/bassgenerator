# Test plan

## Automated (CTest)

| CTest name | Binary | Covers |
|---|---|---|
| Theory | gfb_tests | scale shapes, 12 roots × 7 scales membership, diatonic stepping, registers, enharmonic spelling |
| Seed | gfb_tests | seed parsing/overflow, PRNG reproducibility, bounded/weighted sampling uniformity |
| Corpus | gfb_tests | ≥48 archetypes, grid grammar, rotation-distinctness, pools ≥3 per feel×family, densities |
| Rhythm | gfb_tests | every mutation respects bounds or leaves the cell untouched |
| Settings | gfb_tests | settings/locks serialization + repair, Roll/Randomise never changes locked fields |
| Generator | gfb_tests | all 12×7×2×7 combos valid & in scale; **10 000 random cases** (all bars, octaves ±3, kick contexts) 0 invalid / 0 out-of-scale / 0 silent; determinism; **golden hashes**; VAR vs STYLE structure; octave transposition; phrase endings and structural repetition; swing bounds; duplicates < 2 % |
| History | gfb_tests | 64 capacity, dedupe policy, recall restores settings, append-after-navigation |
| Serialization | gfb_tests | pattern string round trip, rejection of malformed data |
| Metrics | gfb_tests | Feel and SUB/ACID statistical distinctness (A16) |
| Midi | gfb_tests | SMF round trip for 1/2/4/8 bars × families × feels, tempo, 4/4, EOT length, deterministic bytes, same-pitch note-off ordering, slide overlap, corrupt-file rejection, safe file names |
| Scheduler | gfb_tests | sample-accurate onsets for 8 block sizes, boundary single fire, seek/loop/stop no stuck notes, pattern swap, tempo automation, 10-minute drift, legato ordering, stopped = silent |
| Dsp | gfb_tests | silence without notes, audible with notes & release, finite/bounded at parameter extremes ×44.1/48/96 kHz, SUB vs ACID spectral centroid, every macro measurable, block-size invariance, presets differ |
| Realtime | gfb_tests | zero heap allocations in scheduler+synth over 4 000 blocks incl. seeks/stops |
| State | gfb_juce_tests | exact project state round trip (64 history), stored notes not regenerated, corrupted/future data |
| Favorites | gfb_juce_tests | persistence across instances, dedupe, corrupt file → backup recovery, 8 threads × 20 adds, **4 processes × 15 adds** merge |
| JuceMidi | gfb_juce_tests | independent parse of our .mid with `juce::MidiFile` |
| HostIntegration | gfb_host_test | loads the built `.vst3` via JUCE's VST3 host: instrument discovery, params, audio while playing, silence after stop, MIDI out, state into fresh instance (pattern/settings/locks/params/options), corrupted chunk, 44.1/48/96 kHz × 32…1024, editor lifecycle |
| UiBindings | gfb_ui_tests | real editor + processor: GENERATE/NEW VAR/history/family/locks/knob/toggles change engine state, preview audio, drawer, resize limits, snapshots, close/reopen while processing |

Run: `ctest --preset windows-x64-release` (Windows) or `xvfb-run -a ctest --preset linux-release` (Linux).

## Diagnostics

`pattern_diagnostics --count N --out DIR` → `diagnostics.csv` (per pattern), `diagnostics_summary.md/.json`. `render_demo --out DIR` → 16 golden demo `.mid` + `.wav` + `demos.md`.

## Manual host checks (to do on Windows — not performed yet)

1. REAPER: insert GrooveForge Bass on a track, play → audio follows transport, loops, tempo changes.
2. Drag piano roll onto a REAPER track → MIDI item with identical notes; repeat in Ableton Live, FL Studio, Bitwig.
3. EXPORT → import `.mid` into each DAW; check note positions/lengths/velocity and tempo handling.
4. MIDI OUT: route the plugin's MIDI output to another instrument track (REAPER: send with MIDI; Ableton: "MIDI From" the GrooveForge track; FL: not expected for VST3 instruments → document).
5. Save project, close, reopen → identical pattern, seeds, locks, synth.
6. Two instances: add favorites in both → both lists show all.
7. 125 %, 150 %, 200 % Windows display scaling: editor crisp, resizable.
8. CPU at 48 kHz/128 samples; no xruns during GENERATE spam.

## Producer listening test protocol (not yet run)

1. `render_demo --audition 48 --out audition/` renders 48 unedited 4-bar clips (24 SUB, 24 ACID, balanced over the 6 feels, random keys among minor modes, random seeds), names anonymised; `answer_key.csv` must be kept by the test leader only.
2. ≥ 3 producers, randomised order, loop each clip against the same neutral 124 BPM 4/4 drum loop (house kick + hats), headphones and monitors.
3. Rate 1–5 in `rating_sheet.csv`: groove, usability in a track, articulation, loop coherence, originality; free notes.
4. Optional: mix in legally obtained reference basslines the raters own, unlabelled, to calibrate.
5. Report mean/median per family × feel, inter-rater spread, and the worst clips (fed back into corpus/weights). Until then quality status = **"not yet auditioned"**.

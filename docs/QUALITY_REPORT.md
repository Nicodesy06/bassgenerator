# Quality report

Status values: PASS (measured/observed), FAIL, PARTIAL, UNTESTED. Numbers below come from commands run in this session; reproduce them with the commands given.

## 1. Builds

| Target | Environment | Result | Evidence |
|---|---|---|---|
| Linux VST3 + Standalone + tests (Release, GCC 13, JUCE 9.0.5) | dev container | PASS | `build/GrooveForgeBass_artefacts/Release/VST3/GrooveForge Bass.vst3` |
| Linux job | GitHub Actions ubuntu-24.04 | PASS (build + 18/18 CTest) | run 43443851332, job "Linux (GCC, VST3 + tests)" |
| **Windows x64 VST3 + Standalone (MSVC 19.44, VS 2022)** | GitHub Actions windows-2022 | **PASS**: build, 18/18 CTest (incl. golden hashes from Linux ⇒ cross-toolchain determinism, VST3 host harness, GUI bindings), diagnostics, package | run 43457282440 (https://github.com/Nicodesy06/bassgenerator/actions/runs/43457282440); ZIP `dist/GrooveForgeBass-0.1.0-win64.zip`, SHA-256 `ddf88e7639b20078cf10438a1d907008c48273679bd5df5c8bb40951e7575659`, uploaded as artifact `GrooveForgeBass-win64` |
| pluginval 1.0.4 strictness 10, default (out-of-process), all tests incl. GUI/Editor Automation | GitHub Actions windows-2022 | **PASS** (SUCCESS, 14 s) | same run, step "pluginval strictness 10 (full, out-of-process)" |
| pluginval 1.0.4 strictness 10 `--skip-gui-tests` | GitHub Actions windows-2022 | **PASS** | same run |
| pluginval 1.0.4 strictness 10 `--validate-in-process` | GitHub Actions windows-2022 | **FAIL (open issue)**: process exits during "Editor Automation" in runs 43450522799 and 43453343461 (before/after deferring program changes); all earlier categories passed. Not reproducible on Linux: the equivalent stress through the real VST3 wrapper is clean under AddressSanitizer (118 262 blocks). Now tracked as an informational CI step. | runs 4–5 |
| **macOS universal (arm64 + x86_64) VST3 + AU + Standalone (Apple Clang)** | GitHub Actions macos-latest | **PASS**: build, 18/18 CTest, golden hashes on arm64 and on the x86_64 slice (Rosetta), DMG + .pkg built, package installed on the runner, **auval: AU VALIDATION SUCCEEDED**, installed VST3 passes the host harness | release workflow run 43547228796 |
| **Windows installer (Inno Setup .exe)** | GitHub Actions windows-2022 | **PASS**: silent install (all users), installed VST3 passes the host harness, silent uninstall removes the plug-in | release workflow runs 43538121670, 43545579683 |
| pluginval locally | dev container | not run (downloading and executing the prebuilt binary was not permitted in this environment) | – |

## 2. Automated tests (Linux, this session, 0.3.0)

20/20 CTest entries passed on Linux (incl. new suites `Taste` and `TasteStore`, and UI tests for LIKE / DISLIKE / TASTE). Windows/macOS results: see the CI runs for the 0.3.0 commit.

## 3. Generator statistics (algorithmVersion 3)

Source: `reports/diagnostics_summary.md` (4 200 patterns: 2 families × 7 feel settings × 300, bars cycling 1/2/4/8, random keys/scales/seeds, factory generator = no taste profile).

| Metric | Value |
|---|---|
| Invalid events / silent / out-of-scale / illegal overlaps | 0 / 0 / 0 / 0 |
| Same settings → identical notes & hash | 100 % |
| VAR-only change: onset Jaccard / note overlap | 0.96 / 0.95 |
| STYLE change: onset Jaccard / note overlap | 0.37 / 0.19 |
| Taste learning (12 synthetic ratings, `Taste.*`) | mean taste score 0.532 → 0.579; learned cells used in 39/150; disliked archetype 19 → 5 / 200 |

Feel/family means (selected features; mean note length in 16ths, sounding = fraction of the loop with a note sounding):

| Group | onsets/bar | syncopation | off-8th | mean len | sounding | longest rest | distinct pitches | step ratio | octave jumps | distinct bars |
|---|---|---|---|---|---|---|---|---|---|---|
| SUB Sparse | 3.42 | 0.58 | 0.50 | 2.14 | 0.45 | 5.62 | 2.82 | 0.20 | 0.03 | 0.51 |
| SUB Balanced | 6.05 | 0.55 | 0.44 | 1.70 | 0.63 | 2.70 | 3.65 | 0.24 | 0.07 | 0.51 |
| SUB Busy | 9.96 | 0.55 | 0.30 | 1.17 | 0.73 | 1.63 | 3.94 | 0.25 | 0.03 | 0.49 |
| SUB Rolling | 9.35 | 0.71 | 0.40 | 1.12 | 0.64 | 2.05 | 3.75 | 0.22 | 0.04 | 0.50 |
| SUB Syncopated | 6.00 | 0.66 | 0.35 | 1.86 | 0.69 | 2.90 | 3.72 | 0.27 | 0.04 | 0.51 |
| SUB Stabby | 4.99 | 0.57 | 0.40 | 0.45 | 0.14 | 5.36 | 3.39 | 0.17 | 0.05 | 0.51 |
| ACID Sparse | 3.58 | 0.46 | 0.52 | 2.21 | 0.48 | 5.21 | 3.37 | 0.52 | 0.03 | 0.50 |
| ACID Balanced | 6.08 | 0.54 | 0.42 | 1.72 | 0.65 | 2.64 | 3.99 | 0.48 | 0.03 | 0.50 |
| ACID Busy | 10.11 | 0.54 | 0.33 | 1.01 | 0.63 | 1.82 | 4.44 | 0.52 | 0.02 | 0.50 |
| ACID Rolling | 8.79 | 0.59 | 0.41 | 1.20 | 0.65 | 1.86 | 4.52 | 0.46 | 0.05 | 0.49 |
| ACID Syncopated | 5.96 | 0.63 | 0.37 | 1.85 | 0.68 | 2.76 | 3.80 | 0.48 | 0.03 | 0.49 |
| ACID Stabby | 5.47 | 0.59 | 0.31 | 0.40 | 0.14 | 4.94 | 3.77 | 0.48 | 0.02 | 0.51 |

Comparison with the user's 14 hand-made files (`tools/reference_analyzer`, aggregates only):

| Metric | user files | v1 (0.1.0) typical | v3 |
|---|---|---|---|
| mean note length (16ths) | 2.1 | 0.7–1.4 | 1.0–2.2 (Stabby 0.4 by design) |
| sounding ratio | 0.77 | 0.29–0.60 | 0.45–0.73 |
| 1–2 semitone steps | 0.43 | 0.05–0.08 SUB / 0.36–0.47 ACID | 0.17–0.27 SUB / 0.46–0.52 ACID |
| octave jumps | 0.02 | 0.08–0.19 | 0.02–0.07 |
| distinct bars | 0.44 | 0.74 | 0.49–0.51 |

SUB is deliberately kept more root-centred than the user's files (a sub line with 43 % steps stops sounding like a sub); taste learning moves it further if the user's ratings ask for it. Known weak spot: SUB Rolling vs SUB Busy differ mainly by syncopation, not density.

Golden demos: 16 patterns (`reports/demos/demos.md`, `.mid` committed, `.wav` reproducible with `render_demo`). Listening status: **not yet auditioned** (protocol in TEST_PLAN.md; 48-clip blind set reproducible with `render_demo --audition 48`).

## 4. MIDI

- SMF format 0, PPQ 960, tempo + 4/4 + key signature, end-of-track exactly at bars×3840: PASS for all bars × families × feels (our reader and `juce::MidiFile`).
- Same-pitch legato repeats: note-off before note-on, no stuck notes: PASS.
- Deterministic bytes: PASS.
- DAW import / drag: UNTESTED (no DAW available).

## 5. DSP

| Check | Result |
|---|---|
| Silence without notes (peak) | < 1e-6 PASS |
| Audible with notes, released to < 1e-4 RMS | PASS |
| Finite & ≤ 2.0 at parameter corners × 44.1/48/96 kHz | PASS |
| SUB vs ACID spectral centroid (same settings) | 206 Hz vs 477 Hz PASS |
| Cutoff/Reso/EnvMod/Decay/Accent/Drive/Volume/Glide measurable | PASS (`Dsp.EveryMacroHasMeasurableEffect`) |
| Drive level change at full drive | SUB ×1.34, ACID ×1.48 RMS (compensated) |
| Block-size invariance 32…1024 | max diff < 1e-5 PASS |
| Heap allocations on audio path | 0 PASS |
| CPU (single thread, Xeon 2.1 GHz) | ≈1.0–1.2 % of one core at 48 kHz, ≈1.9–2.3 % at 96 kHz (`reports/benchmark.md`) |
| Golden demo levels | peaks 0.32–0.77, no clipping (after ACID level matching in this session) |

Real-time behaviour on actual audio hardware (xruns at 48 kHz/128) is UNTESTED.

## 6. State, history, favorites, GUI

All PASS in automated tests: exact project round trip (also through the real VST3 bundle into a fresh instance), corrupted chunk ignored, 64-entry history policy, favorites persistence/dedupe/backup recovery, 8-thread and 4-process concurrent merges, GUI controls change engine state, editor close/reopen while processing, resize limits, 1.5×/2× scale snapshots. Windows DPI behaviour in a real host: UNTESTED.

## 6b. Cross-compiler determinism finding

The first macOS run exposed a real bug: two RNG draws were passed as sibling arguments of one function call; argument evaluation order is unspecified (GCC/MSVC right-to-left, Clang left-to-right), so Clang produced different patterns for 6/16 golden settings. Fixed by explicit sequencing (reference order kept, so stored/reference output is unchanged); reproduced and verified locally with Linux Clang 18. `-ffp-contract=off` was also added for gfb_core as a precaution against arm64 FMA.

## 7. Not done / known limitations

- No DAW validation (REAPER/Ableton/FL/Bitwig/Logic): all UNTESTED.
- Installers are unsigned (no Authenticode, no Apple Developer ID / notarisation).
- External drag into a DAW: implemented, UNTESTED.
- No producer listening test yet.
- pluginval not run locally (only in CI); in-process mode still fails in Editor Automation on Windows (see §1).
- JUCE licence decision pending (AGPLv3 vs commercial).
- No pitch-bend slide export mode; no microtiming panel; no manual note editing.

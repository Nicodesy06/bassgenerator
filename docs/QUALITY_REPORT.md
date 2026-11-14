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

## 2. Automated tests (Linux, this session)

18/18 CTest entries passed on Linux (`reports/ctest_linux.txt`) and on Windows CI; `gfb_tests`: 48 cases, 3 698 620 checks, 0 failures.

## 3. Generator statistics

Source: `reports/diagnostics_summary.md` (4 200 patterns: 2 families × 7 feel settings × 300, bars cycling 1/2/4/8, random keys/scales/seeds).

| Metric | Value |
|---|---|
| Invalid events / silent / out-of-scale / illegal overlaps | 0 / 0 / 0 / 0 |
| Fallbacks, candidate rejections | 0, 0 |
| Same settings → identical notes & hash | 500/500 |
| 10 000 random cases (all options, octave ±3, kick contexts) | 0 invalid, 0 out-of-scale, 0 silent |
| Exact duplicates (480 seeds × feel × family, 4 bars) | 0 |
| VAR-only change: onset Jaccard / note overlap | 0.92 / 0.91 (output changed in 300/300) |
| STYLE change: onset Jaccard / note overlap | 0.40 / 0.24 |
| Generation time p50 / p95 / p99 / max | 0.11 / 0.31 / 0.38 / 0.53 ms (Xeon 2.1 GHz) |
| Endings resolved (400 phrases) | 400/400 |
| 4/8-bar phrases with repetition but not frozen | 188/200 |

Feel/family means (selected features):

| Group | onsets/bar | syncopation | off-8th | short-note | sounding | longest rest (16ths) | max 16th chain | distinct pitches | step ratio |
|---|---|---|---|---|---|---|---|---|---|
| SUB Sparse | 3.22 | 0.63 | 0.51 | 0.46 | 0.29 | 7.55 | 1.40 | 3.25 | 0.07 |
| SUB Balanced | 5.98 | 0.61 | 0.53 | 0.55 | 0.46 | 3.56 | 2.27 | 3.65 | 0.06 |
| SUB Busy | 9.95 | 0.57 | 0.33 | 0.71 | 0.60 | 2.20 | 3.56 | 3.89 | 0.08 |
| SUB Rolling | 9.32 | 0.71 | 0.39 | 0.89 | 0.45 | 2.66 | 3.19 | 3.60 | 0.05 |
| SUB Syncopated | 6.07 | 0.71 | 0.36 | 0.47 | 0.56 | 3.58 | 2.19 | 3.56 | 0.08 |
| SUB Stabby | 5.12 | 0.52 | 0.45 | 0.95 | 0.13 | 5.63 | 1.61 | 3.85 | 0.06 |
| ACID Sparse | 3.50 | 0.56 | 0.50 | 0.52 | 0.28 | 7.54 | 1.57 | 4.18 | 0.37 |
| ACID Balanced | 6.12 | 0.56 | 0.52 | 0.58 | 0.42 | 3.58 | 2.26 | 4.80 | 0.36 |
| ACID Busy | 10.16 | 0.55 | 0.32 | 0.83 | 0.46 | 2.41 | 3.66 | 5.12 | 0.47 |
| ACID Rolling | 9.04 | 0.61 | 0.39 | 0.83 | 0.43 | 2.55 | 3.13 | 5.14 | 0.37 |
| ACID Syncopated | 6.08 | 0.66 | 0.44 | 0.49 | 0.52 | 3.40 | 2.21 | 4.81 | 0.40 |
| ACID Stabby | 5.51 | 0.57 | 0.33 | 0.92 | 0.12 | 5.34 | 1.74 | 4.80 | 0.45 |

Reading: density ladder Sparse < Balanced/Syncopated/Stabby < Rolling < Busy; Stabby has the shortest notes and lowest sounding ratio; Syncopated/Rolling the highest syncopation; Sparse the longest rests; ACID uses ~5–6× more scale steps, more distinct pitches and slides (0.06–0.09 vs ≤0.01) than SUB. Provisional thresholds and their rationale are the assertions in `tests/PatternMetricsTests.cpp`. Known weak spot: SUB Rolling vs SUB Busy differ mainly by syncopation/short-note ratio, not density.

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

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

Source: `reports/diagnostics_summary.md`, regenerated for algorithmVersion 2 (4 200 patterns: 2 families × 7 feel settings × 300, bars cycling 1/2/4/8, random keys/scales/seeds).

| Metric | Value |
|---|---|
| Invalid events / silent / out-of-scale / illegal overlaps | 0 / 0 / 0 / 0 |
| Fallbacks, candidate rejections | 0, 0 |
| Same settings → identical notes & hash | 500/500 (also identical across GCC, MSVC and Clang: golden test) |
| 10 000 random cases (all options, octave ±3, kick contexts) | 0 invalid, 0 out-of-scale, 0 silent |
| Exact duplicates (480 seeds × feel × family, 4 bars) | 0 |
| VAR-only change: onset Jaccard / note overlap | 0.91 / 0.90 (output changed in 300/300) |
| STYLE change: onset Jaccard / note overlap | 0.43 / 0.25 |
| Generation time p50 / p95 / p99 / max | 0.13 / 0.34 / 0.39 / 0.57 ms (Xeon 2.1 GHz, 16 style candidates) |
| Endings resolved (400 phrases) | 400/400 |
| 4/8-bar phrases with repetition but not frozen | 188/200 |

Feel/family means (selected features):

| Group | onsets/bar | syncopation | off-8th | short-note | sounding | longest rest (16ths) | max 16th chain | distinct pitches | step ratio |
|---|---|---|---|---|---|---|---|---|---|
| SUB Sparse | 3.10 | 0.68 | 0.59 | 0.13 | 0.31 | 7.38 | 1.52 | 2.87 | 0.08 |
| SUB Balanced | 5.62 | 0.68 | 0.57 | 0.04 | 0.49 | 3.53 | 2.12 | 3.78 | 0.11 |
| SUB Busy | 9.24 | 0.67 | 0.38 | 0.10 | 0.64 | 2.47 | 3.56 | 4.25 | 0.14 |
| SUB Rolling | 9.01 | 0.76 | 0.42 | 0.15 | 0.54 | 2.84 | 3.00 | 3.90 | 0.09 |
| SUB Syncopated | 5.70 | 0.77 | 0.41 | 0.07 | 0.56 | 3.66 | 2.09 | 3.66 | 0.13 |
| SUB Stabby | 4.67 | 0.65 | 0.53 | 0.88 | 0.19 | 5.89 | 1.80 | 3.66 | 0.10 |
| ACID Sparse | 3.26 | 0.68 | 0.58 | 0.09 | 0.30 | 7.33 | 1.64 | 3.92 | 0.36 |
| ACID Balanced | 5.72 | 0.64 | 0.57 | 0.05 | 0.44 | 3.89 | 2.18 | 4.54 | 0.34 |
| ACID Busy | 9.30 | 0.67 | 0.38 | 0.86 | 0.55 | 2.55 | 3.41 | 5.16 | 0.43 |
| ACID Rolling | 8.21 | 0.73 | 0.45 | 0.07 | 0.49 | 2.82 | 2.78 | 4.92 | 0.37 |
| ACID Syncopated | 5.86 | 0.72 | 0.49 | 0.03 | 0.54 | 3.54 | 2.25 | 4.73 | 0.37 |
| ACID Stabby | 5.09 | 0.71 | 0.43 | 0.87 | 0.19 | 5.75 | 1.84 | 4.48 | 0.42 |

Reading: density ladder Sparse < Balanced/Syncopated/Stabby < Rolling < Busy; Stabby has the shortest notes and lowest sounding ratio; Syncopated/Rolling the highest syncopation; Sparse the longest rests; ACID uses ~5–6× more scale steps, more distinct pitches and slides (0.06–0.09 vs ≤0.01) than SUB. Provisional thresholds and their rationale are the assertions in `tests/PatternMetricsTests.cpp`. Known weak spot: SUB Rolling vs SUB Busy differ mainly by syncopation/short-note ratio, not density.

Golden demos: 16 patterns (`reports/demos/demos.md`, `.mid` committed, `.wav` reproducible with `render_demo`). Listening status: **not yet auditioned** (protocol in TEST_PLAN.md; 48-clip blind set reproducible with `render_demo --audition 48`).

## 3b. Calibration against reference basslines (algorithmVersion 1 → 2)

Measured with `tools/reference_analyzer` (same metrics for references and generator; generator: 300 patterns per family × feel, 4 bars, four-on-the-floor). Full aggregate tables: `reports/reference/reference_vs_generator.md`.

| Measure | Reference house / tech house (100 files) | GFB v1 (all feels) | GFB v2 (all feels) |
|---|---|---|---|
| Notes on kick beats 2–4 | 0–1 % | 15 % (7–22 % per feel) | **2 %** (1–3 % in Balanced) |
| Onsets on 2.1 / 3.1 / 4.1 | 0 % / 0–2 % / 2–6 % | 29 % / 34 % / 38 % | **4 % / 4 % / 7 %** |
| Onsets on the off-8ths (x.3) | 68–96 % | 62–70 % | 64–80 % |
| Mean note length (16ths) | 1.21–1.83 | 1.00 | 1.18 (Balanced 1.26–1.40) |
| Time sounding | 49–58 % | 39 % | 44 % (Balanced 44–48 %) |
| Octave jumps | 8–9 % | 14 % (SUB 16–21 %) | **6 %** |
| 1–2 semitone steps | 21–22 % | 26 % (SUB 5–12 %, ACID 39–47 %) | 27 % (SUB 9–18 %, ACID 35–43 %) |
| Repeated pitch | 32–34 % | 33 % | 40 % |
| Distinct pitches / pattern | 5.8–6.0 | 4.8 | 4.8 (SUB 4.3, ACID 5.4) |
| Mean MIDI pitch | 49–50 | 41 | 46 (ACID 48, SUB 43) |
| Fifth as share of notes | 11 % | 15 % (SUB 20 %) | 16 % |

Still different (deliberate or open): ACID stays more step-wise than the references (acid idiom); SUB stays lower and slightly more repetitive; references use ~6–9 % chromatic notes, the generator none (scale is a hard constraint by design); the references still sustain more (49–58 % of the time sounding vs 44 %). Feel-specific tests were updated: Rolling is now distinguished from Balanced by shorter mean note length instead of the "<0.8 step" ratio, which no longer applies with fuller gates.

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

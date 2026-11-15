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

21/21 CTest entries passed on Linux for 0.4.0 (incl. suites `Taste`, `TasteStore`, `Complexity`, `Groove` and UI tests). Windows/macOS results: see the CI runs for the 0.3.0 commit.

## 3. Generator statistics (algorithmVersion 4)

Source: `reports/diagnostics_summary.md` (2800 patterns: 2 families × 7 feel settings × 200, bars cycling 1/2/4/8, random keys/scales/seeds, default four-on-the-floor kick context, factory generator). Design rationale: `docs/BASS_RESEARCH.md`.

| Metric | Value |
|---|---|
| Invalid events / silent / out-of-scale / illegal overlaps | 0 / 0 / 0 / 0 |
| Same settings → identical notes & hash | 100 % |
| VAR-only change: onset Jaccard / note overlap | 0.96 / 0.93 |
| STYLE change: onset Jaccard / note overlap | 0.54 / 0.43 |
| Four-on-the-floor: notes starting on a kick / sounding across a kick (non-slide) | 0 / 0 (`Groove.FourOnFloorKickLaneIsFree`) |
| Accents on the off-8th | 100 % (`Groove.AccentsSitOnTheOffEighth`) |
| Mean root share (4-bar, all feels) | SUB 0.77, ACID 0.67 (`Groove.RootCentredAndFewPitchesPerBar`) |

Feel/family means (kick = share of onsets on the four kick beats; mean length in 16ths):

| Group | onsets/bar | syncopation | off-8th | kick | mean len | sounding | distinct pitches | step ratio | octave jumps | root share | distinct bars |
|---|---|---|---|---|---|---|---|---|---|---|---|
| SUB Sparse | 3.58 | 0.67 | 0.67 | 0.00 | 1.33 | 0.30 | 2.62 | 0.02 | 0.17 | 0.78 | 0.55 |
| SUB Balanced | 5.96 | 0.71 | 0.58 | 0.00 | 1.23 | 0.45 | 2.75 | 0.02 | 0.10 | 0.69 | 0.57 |
| SUB Busy | 9.74 | 0.82 | 0.35 | 0.00 | 0.70 | 0.43 | 2.90 | 0.03 | 0.22 | 0.80 | 0.60 |
| SUB Rolling | 9.49 | 0.80 | 0.41 | 0.00 | 0.75 | 0.44 | 2.67 | 0.01 | 0.27 | 0.83 | 0.54 |
| SUB Syncopated | 5.82 | 0.83 | 0.34 | 0.00 | 1.13 | 0.41 | 2.79 | 0.07 | 0.15 | 0.76 | 0.56 |
| SUB Stabby | 5.04 | 0.80 | 0.40 | 0.00 | 0.44 | 0.14 | 2.70 | 0.03 | 0.18 | 0.76 | 0.58 |
| ACID Sparse | 3.53 | 0.75 | 0.49 | 0.00 | 1.23 | 0.27 | 3.19 | 0.14 | 0.13 | 0.68 | 0.59 |
| ACID Balanced | 6.07 | 0.68 | 0.63 | 0.00 | 1.00 | 0.37 | 3.46 | 0.09 | 0.12 | 0.65 | 0.58 |
| ACID Busy | 9.82 | 0.83 | 0.35 | 0.00 | 0.59 | 0.36 | 3.75 | 0.15 | 0.15 | 0.69 | 0.60 |
| ACID Rolling | 8.39 | 0.76 | 0.48 | 0.00 | 0.72 | 0.38 | 3.86 | 0.10 | 0.28 | 0.73 | 0.61 |
| ACID Syncopated | 6.00 | 0.81 | 0.37 | 0.00 | 1.07 | 0.40 | 3.53 | 0.14 | 0.09 | 0.62 | 0.56 |
| ACID Stabby | 5.26 | 0.84 | 0.32 | 0.00 | 0.38 | 0.12 | 3.37 | 0.13 | 0.11 | 0.63 | 0.59 |

Reading: the bass now lives between the kicks in every feel (kick = 0), so the syncopation index is uniformly high; feels differ by density, by how much of the line sits on the off-8th versus the e/a 16ths (Syncopated lowest off-8th share), by note length (Stabby shortest) and by rests (Sparse longest). Pitch content is root-centred with octave/fifth/♭7 colours; SUB uses almost no scale steps, ACID a moderate amount. Known limits: no listening test yet; v3's user-dataset priors are retired (v3 output was rejected by the user).

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

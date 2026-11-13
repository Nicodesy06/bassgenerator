# Plan, status and acceptance matrix

Environment of this work session: Linux x86_64 container (GCC 13, CMake 3.28, Ninja, Xvfb), no Windows machine, no DAW. Windows builds/tests run in GitHub Actions (`windows-2022`, MSVC). JUCE 9.0.5 pinned (commit d60b7a7).

## Phases

| Phase | Status | Evidence |
|---|---|---|
| 0 Toolchain + plan | done | CMakeLists.txt, CMakePresets.json, CI workflow |
| 1 Core model + seeds | done | src/core, tests Theory/Seed/Settings/Serialization |
| 2 Musical generator | done (quality not auditioned) | 57-archetype corpus, grammars, evaluator, reports/diagnostics_summary.md, 16 golden demos |
| 3 Synth + scheduler | done | src/dsp, tests Dsp/Scheduler/Realtime, reports/benchmark.md |
| 4 GUI | done | src/ui, gfb_ui_tests, docs/images |
| 5 MIDI workflow | done (DAW drag untested) | SMF writer + 2 independent readers, drag via JUCE external drag, VST3 MIDI out (harness) |
| 6 Persistence + robustness | done | StateCodec, FavoritesStore, multi-thread/multi-process tests, host harness state tests |
| 7 Polish + measurement | partial | pluginval passes (out-of-process) in CI; in-process Editor Automation crash open; no DAW matrix; no listening test |

## Acceptance matrix

| ID | Criterion | Status | Evidence |
|---|---|---|---|
| A01 | Windows x64 VST3 Release builds | PASS (CI) | run 43457282440, MSVC 19.44, bundle `GrooveForge Bass.vst3/Contents/x86_64-win/` |
| A02 | Loads in at least one real host | PARTIAL | Loads/plays/saves in JUCE's VST3 host implementation (gfb_host_test, Linux + Windows CI) and passes pluginval 1.0.4 strictness 10 (Windows CI, out-of-process); no DAW tested |
| A03 | Two audible SUB/ACID families | PASS | Dsp.SubAndAcidAreSpectrallyDistinct (centroid 206 Hz vs 477 Hz), demos |
| A04 | 8 controls alter audio | PASS | Dsp.EveryMacroHasMeasurableEffect (+ Shape) |
| A05 | Root 12, scales 7, family 2, feel 6+Random, bars 1/2/4/8, octave ±3 | PASS | Generator.AllOptionCombinationsValidAndInScale, OctaveTransposition |
| A06 | Corpus + structured engine | PASS | 57 archetypes, ALGORITHM.md, Corpus.* tests |
| A07 | STYLE/VAR editable, deterministic, different | PASS | SeedBox, golden hashes, VarPreservesStructure (0.92 vs 0.40 onset Jaccard) |
| A08 | Locks, reroll, 64 history | PASS | Settings.RollRespectsLocks, History.*, UiBindings |
| A09 | Cross-session favorites | PASS (file level) | Favorites.* incl. 4-process merge; not tested inside two DAW instances |
| A10 | Preview + transport sync | PASS (harness) / UNTESTED (DAW) | Scheduler.*, HostIntegration, UiBindings preview |
| A11 | Exported MIDI imports with correct pattern | PARTIAL | Round trip with our reader and juce::MidiFile; DAW import not tested |
| A12 | External drag to host | UNTESTED | Implemented (performExternalDragDropOfFiles); needs manual Windows DAW test |
| A13 | Live MIDI out | PARTIAL | Harness receives balanced note on/off; DAW routing untested |
| A14 | Project reload exact | PASS (harness) | HostIntegration: pattern, settings, locks, params, options in fresh instance |
| A15 | No glitches/stuck notes in stress | PARTIAL | Scheduler stress, 10-min drift, zero allocations, benchmark ≈1–2 % of one core; no hardware xrun test |
| A16 | Diagnostics distinguish feels/families | PASS | Metrics.* tests, reports/diagnostics_summary.md |
| A17 | Polished, resizable GUI | PASS (visual check by snapshots) / UNTESTED at real Windows DPI | docs/images, UiBindings resize/scale snapshots |
| A18 | Automated tests pass, report truthful | PASS | 18/18 CTest on Linux and Windows CI; pluginval in-process issue documented |
| A19 | Scripts + release ZIP | PASS (CI) | scripts/*.ps1; ZIP + .sha256 produced by package-windows.ps1 in CI (artifact GrooveForgeBass-win64) |
| A20 | Originality + licences documented | PASS | LICENSE-THIRD-PARTY.md |

## Next actions

0. Investigate the pluginval `--validate-in-process` crash in Editor Automation on Windows (run it locally on Windows under a debugger).
1. Install the CI ZIP on a Windows machine; run the manual checklist in TEST_PLAN.md (REAPER first), fill DAW_COMPATIBILITY.md.
2. Run the blind listening protocol (48 clips) and tune corpus weights/evaluator from the ratings.
3. Decide the JUCE licence (AGPLv3 open-source release vs commercial licence).
4. Optional: pitch-bend slide export mode, microtiming panel, chord-aware mode.

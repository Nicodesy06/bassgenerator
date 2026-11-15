# CLAUDE.md — developer guide for GrooveForge Bass

User-facing text (README, handoff) in Italian; code, comments, tests, technical docs in English.

## Commands

```bash
# Linux dev build (local JUCE checkout avoids network)
cmake -S . -B build -G Ninja -DCMAKE_BUILD_TYPE=Release -DGFB_JUCE_DIR=/path/to/JUCE-9.0.5
cmake --build build -j
xvfb-run -a ctest --test-dir build --output-on-failure      # GUI/host tests need a display
build/tests/gfb_tests Generator.                             # one suite (substring filter)
build/tools/pattern_diagnostics --count 300 --out reports    # statistics report
build/tools/pattern_diagnostics --explain 1234 5 ROLLING ACID 4
build/tools/render_demo --out reports/demos [--with-drums]   # golden demos (.mid/.wav, optional kick+hat mix)
build/tools/pattern_dump ROLLING SUB 4 4 [cx] [seed]         # read patterns as a 16th text grid
```

Windows: `cmake --preset windows-x64-release`, `cmake --build --preset windows-x64-release`, `ctest --preset windows-x64-release`, or `scripts/*.ps1`.

## Guardrails

- `src/core`, `src/dsp`, `src/midi/MidiFileExport.*` must stay JUCE-free.
- Never use `<random>` distributions or libm transcendental functions in the generator/evaluator (cross-platform determinism). Integer weights only. Keep `-ffp-contract=off` on gfb_core (arm64 FMA changes results). Never put two RNG draws in the arguments of one call (evaluation order differs between GCC/MSVC and Clang): sequence them into locals.
- Any change that alters generator output: bump `kAlgorithmVersion`, regenerate `tests/golden_hashes.inc` with `pattern_diagnostics --golden`, rerun diagnostics and update `docs/QUALITY_REPORT.md`. Stored projects keep their exact notes regardless.
- Audio thread: no allocation, locks, I/O, logging. `Realtime.AudioPathDoesNotAllocate` must stay green; the schedule reaches the audio thread only through `ScheduleExchange`.
- Parameter IDs (`volume cutoff resonance envmod decay accent drive glide shape engine`), plugin codes (`Gfau`/`Gfb1`) and state schema keys are stable. Add a migration in `StateCodec::decode` when changing the schema.
- Taste learning: an empty `TasteProfile` must reproduce the factory generator exactly (`Taste.EmptyProfileEqualsFactory`). Tests must never touch the user's real `taste.json`/`favorites.json` (use `GFB_TASTE_FILE` / explicit files).
- Reference datasets: never commit third-party or user MIDI files (the repo is public). Store only aggregate integers (`src/core/ReferencePriors.h` via `tools/reference_analyzer --emit-priors`). Style cells must be original, never transcriptions of an artist's track.
- Windows build uses the static MSVC runtime (`CMAKE_MSVC_RUNTIME_LIBRARY`); keep it, CI checks the VST3 imports. The installer is admin-only and installs to Common Files\VST3.
- Genre rules (v4, `docs/BASS_RESEARCH.md`): with FourOnFloor no onset on a kick step and no non-slide note across a kick (`Groove.*` tests); keep the tonic dominant and accents on the off-8th.
- Corpus edits: keep grids rotation-unique and inside the feel's density bounds (tests enforce it).
- Do not claim DAW compatibility without testing; update `docs/DAW_COMPATIBILITY.md`.
- JUCE is AGPLv3/commercial: see LICENSE-THIRD-PARTY.md before distributing binaries.

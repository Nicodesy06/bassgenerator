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
build/tools/render_demo --out reports/demos                  # golden demos (.mid/.wav)
```

Windows: `cmake --preset windows-x64-release`, `cmake --build --preset windows-x64-release`, `ctest --preset windows-x64-release`, or `scripts/*.ps1`.

## Guardrails

- `src/core`, `src/dsp`, `src/midi/MidiFileExport.*` must stay JUCE-free.
- Never use `<random>` distributions or libm transcendental functions in the generator/evaluator (cross-platform determinism). Integer weights only. Keep `-ffp-contract=off` on gfb_core (arm64 FMA changes results).
- Any change that alters generator output: bump `kAlgorithmVersion`, regenerate `tests/golden_hashes.inc` with `pattern_diagnostics --golden`, rerun diagnostics and update `docs/QUALITY_REPORT.md`. Stored projects keep their exact notes regardless.
- Audio thread: no allocation, locks, I/O, logging. `Realtime.AudioPathDoesNotAllocate` must stay green; the schedule reaches the audio thread only through `ScheduleExchange`.
- Parameter IDs (`volume cutoff resonance envmod decay accent drive glide shape engine`), plugin codes (`Gfau`/`Gfb1`) and state schema keys are stable. Add a migration in `StateCodec::decode` when changing the schema.
- Corpus edits: keep grids rotation-unique and inside the feel's density bounds (tests enforce it).
- Do not claim DAW compatibility without testing; update `docs/DAW_COMPATIBILITY.md`.
- JUCE is AGPLv3/commercial: see LICENSE-THIRD-PARTY.md before distributing binaries.

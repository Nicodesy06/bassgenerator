# Generation algorithm (algorithmVersion 2)

No machine learning model is used and none is claimed. Since algorithmVersion 2 the hand-authored grammar is calibrated against **aggregate statistics** of licensed reference basslines (see "Reference priors"). The engine is an **authored corpus + conditioned generative grammars + heuristic candidate evaluator**, fully deterministic and offline.

## Pipeline

```
settings + STYLE + VAR
  └─ STYLE stream ─ resolve feel (Random) ─ pick archetype A (+ answer archetype)
                  ─ motif vocabulary (2–6 pitch roles) ─ phrase template ─ answer type
                  ─ 12 core-motif candidates (bounded mutations + pitch assignment) → score → best
                  ─ answer bar B (transposed / inverted / contrasting rhythm)
  └─ VAR stream   ─ 16 phrase realisations (A', A'', B', fills, ending) → score → best
                  ─ expression stream: bounded velocity jitter
  └─ immutable Pattern (validated, hashed)
```

## 1. Rhythm corpus (src/core/RhythmCorpus.cpp)

57 original one-bar archetypes, 10 fill cells, written in a readable grid (`A` anchor, `X` accent, `x` note, `g` ghost, `o` octave punctuation, `-` hold, `~` hold+slide, `.` rest). Each has primary/secondary feel, family mask (SUB/ACID/both), kick-context fit, contour hint, max swing and a usage note. Distribution per (feel, family) pool: every pool has ≥ 3 entries (test `Corpus.EveryFeelFamilyPoolHasThreeOrMore`); **no two archetypes share the same onset vector under any rotation** (test `Corpus.SizeValidityAndDistinctness`), so the count is not inflated by shifted copies. Onset counts match the primary feel's bounds.

## 2. Feel profiles (src/core/RhythmGrammar.cpp)

Engineering hypotheses (not Bassurito numbers):

| Feel | onsets/bar | gate short/long ‰ | swing % | sync target ‰ | off-8th target ‰ | max internal rest |
|---|---|---|---|---|---|---|
| Sparse | 2–6 (3) | 900/960 | 52 | 250–750 | 300–850 | 13 |
| Balanced | 4–9 (6) | 880/950 | 54 | 250–650 | 300–750 | 10 |
| Busy | 8–13 (10) | 820/930 | 52 | 250–650 | 150–600 | 4 |
| Rolling | 6–12 (9) | 850/940 | 56 | 450–850 | 300–750 | 5 |
| Syncopated | 4–10 (6) | 900/960 | 54 | 500–950 | 150–700 | 9 |
| Stabby | 3–9 (5) | 520/600 | 52 | 250–800 | 150–700 | 10 |

Random = weighted choice from the STYLE stream (Balanced 30, Rolling 20, Syncopated 20, others 10). Mutation operators with hard bounds: remove/add onset (never onto a kick beat; Sparse/Stabby forbid adjacent onsets), split, merge (not in Stabby), anticipation (move a beat onset one 16th earlier, accented), delayed root entry, accent shift, ghost note, octave punctuation (max 1/bar SUB, 2 ACID), bar-end pickup, slide toggle (SUB max one glide per bar). A failed or out-of-bounds mutation leaves the cell untouched. Each feel weights operators differently; ACID boosts slide/accent ops, SUB boosts octave punctuation.

Measured distinguishing features (≥5 per feel, `reports/diagnostics_summary.md`): onsets/bar, syncopation index, off-8th ratio, short-note ratio, sounding ratio, longest rest, longest 16th chain, ghost ratio. Assertions in `tests/PatternMetricsTests.cpp`.

## 3. Pitch (src/core/PitchGrammar.cpp)

Root and scale are hard constraints: every pitch is a scale member by construction (0 out-of-scale notes in 10 000+ random cases and 4 200 diagnostic patterns). A phrase vocabulary of 2–4 (SUB) or 3–6 (ACID) roles is drawn from {root, octave, fifth, fifth below, 7th below, 3rd, 4th, 2nd, 6th, 6th below, octave 3rd} with family weights and contour must-haves. Assignment is conditioned on the rhythmic role: anchors → root (SUB 90 %, ACID 76 %) else octave/fifth; octave role → +12; ghosts repeat the previous pitch; pickups → approach tones to the next downbeat root; normal notes → transition weights: SUB favours 0/5/7/12 intervals, ACID favours 1–2 semitone steps; after a leap > 5 semitones the same-direction leap is forbidden and turning back is favoured; repetition caps (ACID 3, SUB 4, pedal 6); accented notes lean on stable tones; contour hints bias the result. Register: SUB −7…+12, ACID −5…+17 semitones around the reference root (SUB roots G1–F♯2, ACID C2–B2), whole-line octave folding keeps every note in MIDI 0–127 when Octave ±3 pushes it out.

## 4. Phrase (src/core/PhraseComposer.cpp)

Templates chosen by STYLE: 1 bar `A*`; 2 bars `A A'` / `A B`; 4 bars `A A' B F`, `A B A' F`, `A A' A F`; 8 bars `A A' B F A A'' B' F2`, `A A' A B A A' B' F2`, `A B A' F A B' A'' F2`. A is identical in every A bar (structural repetition); A' = ≤1 rhythm op + ≤1 pitch change, A'' ≤2+2; B is the answer (diatonic transposition by +3/−2/+4/−1 steps, diatonic inversion, or a contrasting archetype with the same vocabulary); F replaces the last 4 steps (F2: 8 steps) with a fill cell whose pitches follow its shape (run up to the step below the root, run down from the octave, octave jump, approach, pedal, break). The final note is resolved (root/fifth/approach). Last note never rings over the loop end or slides into the wrap.

## 5. Articulation

Swing delays only odd 16ths by `(swing−50)·4.8` ticks (≤ 76 ticks at 66 %), never negative; default swing is per feel/archetype (ACID −2 %), overridable 50–66 % in Advanced. Duration = steps × 240 × gate (SUB +60 ‰ fuller, ACID 1-step −40 ‰ tighter, ghosts ≤ 120 ticks), clamped to the next start; slide notes extend to next start + 30 ticks. Velocities: SUB 112/94/60, ACID 120/86/58 (accent/normal/ghost), anchors +4, then a separate expression stream adds ±3 (accents 0…+4).

## 6. Evaluation (src/core/PatternEvaluator.cpp)

Hard rejects: invalid, silent, out-of-scale, empty bar, ≥12-step 16th chain. Score = weighted mean of 14 components (density vs feel target, syncopation band, off-beat band, implied-kick collisions, motif recurrence for multi-bar, register span, interval character per family, root anchoring, ending, duration contrast, accent ratio, repetition, anti-machine-gun chain bound, longest rest), weights depend on feel and family. Ties keep the lower candidate index. `pattern_diagnostics --explain STYLE VAR FEEL FAMILY BARS` prints every candidate's components. The score is a proxy; it does not replace listening.

## 6b. Reference priors (algorithmVersion 2)

Source: 153 unique licensed MIDI basslines supplied by the user (Weapon Sounds "House" and "Tech House Bassline MIDI", 100 files: royalty-free, files may not be redistributed; THM "Free Tech House Basslines Vol.3", 48 files; 5 other files). The files are **not** in the repository. `tools/reference_analyzer <dir>` measures them with the same metrics as the generator and writes only aggregates (`reports/reference/reference_vs_generator.md`); `--emit-priors` produces the 2 × (16 + 25) integers in `src/core/ReferencePriors.h`: onset probability per 16th step and melodic-interval distribution for a four-on-the-floor context (the 100 house/tech-house files) and a broken/open context (the rest). No reference note sequence or pattern is stored or reproduced.

How they are used:
- Evaluator component `groove_prior` (weight 3, 2 for Sparse/Stabby): mean reference probability of the candidate's onset steps relative to the most likely step (Open context blends it 50 %).
- Evaluator component `interval_prior` (weight SUB 2, ACID 1): mean reference probability of its intervals.
- Kick-beat collision tolerance lowered from 12 % to 5 % (references: ~0–1 % of onsets on beats 2–4).
- `rhythm::clearKickBeats`: in four-on-the-floor context 65 % of style candidates (and fills, except their downbeat) move notes off beats 2–4 (anticipate by a 16th or drop) and let a downbeat anchor enter late on the "and"; the prior-weighted evaluator decides which candidate wins.
- Fuller gates (references fill their slots: 64 % of notes last one 16th, 22 % two), ghost notes no longer clipped to half a step.
- Pitch transition weights re-balanced (SUB: fewer octave/fifth jumps, more repeats and steps; ACID: fewer consecutive steps), larger SUB vocabulary, repeat cap 3 (pedal 5).
- Registers moved up (reference mean MIDI pitch ≈ 50).

Results are in QUALITY_REPORT.md §3b.

## 7. Seeds and determinism

- PRNG: SplitMix64 seeding + xoshiro256**; bounded integers by Lemire multiply-shift with exact rejection; weighted choice with **integer weights only**. No `<random>` distributions.
- Domain separation: `deriveSeed(seed, "style-core" | "style-candidate"#i | "answer" | "var-candidate"#j | "expression")`; the VAR base is `var ^ mix64(style)`.
- Floating point in the evaluator uses only + − × ÷ and comparisons (no libm transcendental), compiled without FP contraction (`-ffp-contract=off` on GCC/Clang — required on arm64 where Clang would otherwise emit FMA — and MSVC `/fp:precise`).
- Golden fixture (regenerated for v2): 16 exact hashes (`tests/golden_hashes.inc`) recorded on Linux/GCC x86-64 and checked on Windows/MSVC x64 and macOS/Apple Clang arm64 + x86-64 in CI. (The first macOS run caught a mismatch on arm64 caused by FMA contraction; fixed by `-ffp-contract=off`.)
- Semantics (measured, 500 pairs): changing only VAR keeps 0.92 onset Jaccard / 0.91 note overlap; changing STYLE drops to 0.40 / 0.24; VAR changes the output in 300/300 cases (`Generator.VarPreservesStructureStyleChangesConcept`).
- Roll: GENERATE = new STYLE+VAR unless locked; NEW STYLE / NEW VAR = one seed; RANDOMISE = every unlocked setting with club weighting (minor modes favoured, bars 4 most likely, octave −1…+1). New seeds are drawn from 1…999 999 for readability; any uint64 can be typed (decimal or 0x hex, overflow rejected).

## Limitations

- One harmonic centre (no chord progressions) by design for v1.
- Microtiming beyond swing is not implemented; default grooves are tight.
- Rhythmic "kick context" is a prior, not audio analysis.
- Musical quality has not yet been validated by producer listening tests (protocol in TEST_PLAN.md).

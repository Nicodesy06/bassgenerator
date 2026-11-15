# Generation algorithm (algorithmVersion 3)

The engine is an **authored corpus + conditioned generative grammars + heuristic candidate evaluator**, fully deterministic and offline. Since v3 it is calibrated on the user's own hand-made basslines and can be steered by **local taste learning** from LIKE / DISLIKE ratings (simple feature statistics, not a neural network).

History: v1 = initial release (0.1.0). v2 (0.2.0) was calibrated on third-party reference packs; the user rejected that sound, so v2 was **withdrawn** and v3 restarts from v1, calibrated only on the user's own MIDI files. Stored projects keep their exact notes regardless of version.

## v3 changes at a glance

- **User priors** (`src/core/ReferencePriors.h`): 16-step onset distribution and interval histogram aggregated from the user's 14 hand-made MIDI files (`tools/reference_analyzer --emit-priors`). Only aggregate integers are stored; the source MIDI files are not in the repository. The evaluator adds `groove_prior` and `interval_prior` components (L1 similarity to these tables) and a `legato` component (sounding ratio 0.55–0.95; user data: 77 %).
- **Rhythm**: new `extend` mutation (ties a note into the following 1–2 rest steps); longer gates; Sub gate +30 ‰.
- **Pitch**: more step-wise motion (user data: 43 % 1–2 semitone steps, 2 % octave jumps, mean pitch MIDI 47); Sub roots C2–B2, Acid F2–E3 (`36+pc` / `pc>=5 ? 36+pc : 48+pc`).
- **Phrase**: weighted templates with more literal repetition (A B A B, A A A A', A A' A A' …) — the user's lines repeat a one-bar idea with small answers (44 % distinct bars).
- **Corpus**: 81 cells. 701–712 are rhythm cells taken from the user's own files (the user owns them); 801–812 are **original** cells written for the groovy house / tech-house idiom the user named as a reference (warm, swung, rolling, call-and-response). They are not transcriptions of any artist's track.
- **Chromatic approach notes** (opt-in, `GeneratorSettings::chromatic`, LIBRARY → ADVANCED): on a separate `chromatic` RNG stream, with 300 ‰ chance per bar, one weak-position note directly before a target is replaced by the semitone approach; the target note is locked so the approach always resolves. Off by default; out-of-scale checks treat these notes as intentional.

## Complexity (since 0.3.1, `GeneratorSettings::complexity`, 0..100, default 50)

`withComplexity(feelProfile(feel), c)` returns an adjusted profile used for mutations, density enforcement and evaluation; c = 50 returns the profile unchanged, so default output (and the golden hashes) are identical to 0.3.0. Above 50 the onset target moves towards max+2, the minimum rises, the longest tolerated rest shrinks, add/split/ghost mutations gain weight and the core motif may take up to 2 extra mutations; below 50 the reverse (target towards min−1, remove/merge/extend favoured, longer rests). The STYLE archetype pool is additionally weighted by closeness of each archetype's onset count to the adjusted target. Integer arithmetic only. Measured (`Complexity.MoreComplexityMeansMoreNotes`, 60 patterns per point), SUB Balanced onsets/bar: 4.49 / 5.43 / 5.90 / 6.68 / 8.22 at 0 / 25 / 50 / 75 / 100; monotonic for every feel × family. Not affected by RANDOMISE.

## Taste learning (src/core/TasteProfile.*, src/state/TasteStore.*)

- Each rating extracts normalised features of the rated pattern: 16-step onset histogram, interval histogram (±12), duration buckets (6), density bucket (4). They are added to the liked or disliked accumulators.
- `taste::score(p)` = clamp(0.5 + 1.5·(simLike − simDislike), 0, 1), sim = 1 − L1/2 weighted steps 3, intervals 1, durations 1.5, density 1. It enters the evaluator as the `taste` component with weight `10 · confidence`, confidence = min(1, ratings / 8).
- Archetype weights in the STYLE pool are scaled by `(2+likes)/(2+dislikes)` (clamped 0.15–4×).
- Liked patterns contribute their first bar as a **learned cell** (max 96, weight 1–9, dislikes decrement). Learned cells join the archetype pool with weight `6 + 3·w`, so the personal dataset grows with every LIKE.
- Determinism: for a given profile and seeds the output is identical; an empty profile (or TASTE off) reproduces the factory generator bit for bit (`Taste.EmptyProfileEqualsFactory`). Each Pattern records the `tasteRevision` it was generated with.
- Persistence: `%APPDATA%/GrooveForgeBass/taste.json` (schema 1: profile string, liked patterns ≤ 1000, rated hashes ≤ 4000), atomic write + backup, shared by all instances, never touched by the audio thread. `GFB_TASTE_FILE` overrides the path (tests). A disliked pattern immediately triggers a new generation. LIBRARY → TASTE exports all liked patterns as .mid files.
- Measured effect (`tests/TasteTests.cpp`): after 12 synthetic ratings the mean taste score of new patterns rises 0.532 → 0.579 and learned cells are used in 39 / 150 generations; a disliked archetype drops from 19 to 5 occurrences in 200.

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

81 one-bar cells (57 original v1 archetypes, 12 from the user's files, 12 original style cells), 10 fill cells, written in a readable grid (`A` anchor, `X` accent, `x` note, `g` ghost, `o` octave punctuation, `-` hold, `~` hold+slide, `.` rest). Each has primary/secondary feel, family mask (SUB/ACID/both), kick-context fit, contour hint, max swing and a usage note. Distribution per (feel, family) pool: every pool has ≥ 3 entries (test `Corpus.EveryFeelFamilyPoolHasThreeOrMore`); **no two archetypes share the same onset vector under any rotation** (test `Corpus.SizeValidityAndDistinctness`), so the count is not inflated by shifted copies. Onset counts match the primary feel's bounds.

## 2. Feel profiles (src/core/RhythmGrammar.cpp)

Engineering hypotheses (not Bassurito numbers):

| Feel | onsets/bar | gate short/long ‰ | swing % | sync target ‰ | off-8th target ‰ | max internal rest |
|---|---|---|---|---|---|---|
| Sparse | 2–6 (3) | 700/920 | 52 | 250–750 | 300–850 | 13 |
| Balanced | 4–9 (6) | 650/880 | 54 | 250–650 | 300–750 | 10 |
| Busy | 8–13 (10) | 540/840 | 52 | 250–650 | 150–600 | 4 |
| Rolling | 6–12 (9) | 600/860 | 56 | 450–850 | 300–750 | 5 |
| Syncopated | 4–10 (6) | 720/920 | 54 | 500–950 | 150–700 | 9 |
| Stabby | 3–9 (5) | 330/450 | 52 | 250–800 | 150–700 | 10 |

Random = weighted choice from the STYLE stream (Balanced 30, Rolling 20, Syncopated 20, others 10). Mutation operators with hard bounds: remove/add onset (never onto a kick beat; Sparse/Stabby forbid adjacent onsets), split, merge (not in Stabby), anticipation (move a beat onset one 16th earlier, accented), delayed root entry, accent shift, ghost note, octave punctuation (max 1/bar SUB, 2 ACID), bar-end pickup, slide toggle (SUB max one glide per bar). A failed or out-of-bounds mutation leaves the cell untouched. Each feel weights operators differently; ACID boosts slide/accent ops, SUB boosts octave punctuation.

Measured distinguishing features (≥5 per feel, `reports/diagnostics_summary.md`): onsets/bar, syncopation index, off-8th ratio, short-note ratio, sounding ratio, longest rest, longest 16th chain, ghost ratio. Assertions in `tests/PatternMetricsTests.cpp`.

## 3. Pitch (src/core/PitchGrammar.cpp)

Root and scale are hard constraints: every pitch is a scale member by construction (0 out-of-scale notes in 10 000+ random cases and 4 200 diagnostic patterns). A phrase vocabulary of 2–4 (SUB) or 3–6 (ACID) roles is drawn from {root, octave, fifth, fifth below, 7th below, 3rd, 4th, 2nd, 6th, 6th below, octave 3rd} with family weights and contour must-haves. Assignment is conditioned on the rhythmic role: anchors → root (SUB 90 %, ACID 76 %) else octave/fifth; octave role → +12; ghosts repeat the previous pitch; pickups → approach tones to the next downbeat root; normal notes → transition weights: SUB favours 0/5/7/12 intervals, ACID favours 1–2 semitone steps; after a leap > 5 semitones the same-direction leap is forbidden and turning back is favoured; repetition caps (ACID 3, SUB 4, pedal 6); accented notes lean on stable tones; contour hints bias the result. Register: SUB −7…+12, ACID −5…+17 semitones around the reference root (SUB roots G1–F♯2, ACID C2–B2), whole-line octave folding keeps every note in MIDI 0–127 when Octave ±3 pushes it out.

## 4. Phrase (src/core/PhraseComposer.cpp)

Templates chosen by STYLE with integer weights (`pickTemplateIndex`): 1 bar `A*`; 2 bars `A A'` / `A B`; 4 bars `A A' B F`, `A B A' F`, `A A' A F`, `A B A B`, `A A A A'`, `A A' A A'`; 8 bars also `A B A B A B A B'`, `A A A A' A A A F`, `A A' A A' A A' A F2`, plus 8 bars `A A' B F A A'' B' F2`, `A A' A B A A' B' F2`, `A B A' F A B' A'' F2`. A is identical in every A bar (structural repetition); A' = ≤1 rhythm op + ≤1 pitch change, A'' ≤2+2; B is the answer (diatonic transposition by +3/−2/+4/−1 steps, diatonic inversion, or a contrasting archetype with the same vocabulary); F replaces the last 4 steps (F2: 8 steps) with a fill cell whose pitches follow its shape (run up to the step below the root, run down from the octave, octave jump, approach, pedal, break). The final note is resolved (root/fifth/approach). Last note never rings over the loop end or slides into the wrap.

## 5. Articulation

Swing delays only odd 16ths by `(swing−50)·4.8` ticks (≤ 76 ticks at 66 %), never negative; default swing is per feel/archetype (ACID −2 %), overridable 50–66 % in Advanced. Duration = steps × 240 × gate (SUB +60 ‰ fuller, ACID 1-step −40 ‰ tighter, ghosts ≤ 120 ticks), clamped to the next start; slide notes extend to next start + 30 ticks. Velocities: SUB 112/94/60, ACID 120/86/58 (accent/normal/ghost), anchors +4, then a separate expression stream adds ±3 (accents 0…+4).

## 6. Evaluation (src/core/PatternEvaluator.cpp)

Hard rejects: invalid, silent, out-of-scale, empty bar, ≥12-step 16th chain. Score = weighted mean of 17 components (+ `taste` when a profile exists) (density vs feel target, syncopation band, off-beat band, implied-kick collisions, motif recurrence for multi-bar, register span, interval character per family, root anchoring, ending, duration contrast, accent ratio, repetition, anti-machine-gun chain bound, longest rest), weights depend on feel and family. Ties keep the lower candidate index. `pattern_diagnostics --explain STYLE VAR FEEL FAMILY BARS` prints every candidate's components. The score is a proxy; it does not replace listening.

## 7. Seeds and determinism

- PRNG: SplitMix64 seeding + xoshiro256**; bounded integers by Lemire multiply-shift with exact rejection; weighted choice with **integer weights only**. No `<random>` distributions.
- Domain separation: `deriveSeed(seed, "style-core" | "style-candidate"#i | "answer" | "var-candidate"#j | "expression")`; the VAR base is `var ^ mix64(style)`.
- Floating point in the evaluator uses only + − × ÷ and comparisons (no libm transcendental), compiled without FP contraction (`-ffp-contract=off` on GCC/Clang — required on arm64 where Clang would otherwise emit FMA — and MSVC `/fp:precise`).
- Golden fixture: 16 exact hashes (`tests/golden_hashes.inc`) recorded on Linux/GCC x86-64 and checked on Windows/MSVC x64 and macOS/Apple Clang arm64 + x86-64 in CI. (The first macOS run caught a mismatch on arm64 caused by FMA contraction; fixed by `-ffp-contract=off`.)
- Semantics (measured, 500 pairs): changing only VAR keeps 0.92 onset Jaccard / 0.91 note overlap; changing STYLE drops to 0.40 / 0.24; VAR changes the output in 300/300 cases (`Generator.VarPreservesStructureStyleChangesConcept`).
- Roll: GENERATE = new STYLE+VAR unless locked; NEW STYLE / NEW VAR = one seed; RANDOMISE = every unlocked setting with club weighting (minor modes favoured, bars 4 most likely, octave −1…+1). New seeds are drawn from 1…999 999 for readability; any uint64 can be typed (decimal or 0x hex, overflow rejected).

## Limitations

- One harmonic centre (no chord progressions) by design.
- Taste learning is a statistical bias, not a model of musical meaning; it needs roughly 8–10 ratings to have an audible effect.
- Microtiming beyond swing is not implemented; default grooves are tight.
- Rhythmic "kick context" is a prior, not audio analysis.
- Musical quality has not yet been validated by producer listening tests (protocol in TEST_PLAN.md).

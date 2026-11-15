# Writing tech-house and minimal-tech basslines — research notes (algorithm v4)

Purpose: turn what producers, tutorials and groove research say about tech-house / minimal-tech bass into rules a deterministic generator can follow, and explain why algorithm v3 produced lines a producer would reject. Sources are listed at the end; most are producer tutorials (practical, not peer-reviewed), plus groove research for the rhythmic principles. No artist's track was transcribed; artist names (Pawsa, Franky Rizardo, Marco Carola, Paco Osuna, ANOTR, Chris Stussy) describe the target idiom only.

## 1. Diagnosis of v3 (from `tools/pattern_dump`)

| Problem | Example (A minor, intervals in semitones, `=` held, `|` kick beats) | Why it fails |
|---|---|---|
| Notes on the kick and holds across it | `0> = = =` on beat 1; `-5> =` crossing beat 3 | The genre's bass lives *between* kicks; overlapping the kick muddies the low end and kills the bounce. |
| Melodic "walking" pitch choice | `0 2 = . 5 = = = 0 7` | Tech-house bass is root-centred with a few colour tones, not scale walking. |
| Whole-bar transposed answers | bar 2 = bar 1 moved by a 4th/5th | Sounds like a chord change on a one-chord groove. |
| Accents everywhere, near-straight swing (52–56 %) | | Groove in the genre comes from swung 16ths (≈ 55–58 %) and accents on the off-8th. |
| Corpus cells written for broken/open kicks were still picked with four-on-the-floor | `A---`, `X--x--X` | Context prior was only a ×2 weight, not a constraint. |

## 2. What the genre does

### 2.1 Relationship with the kick (the main rule)
- Kick on every quarter note at 124–130 BPM; the bass is placed on the off positions and cut before the next kick transient (Loopcloud/Loopmasters tutorial; Function Loops; boombox). Bass and kick "trade space" (call and response between kick and bass).
- Sidechain/ducking is normal, but tutorials stress that the *notes themselves* should avoid the kick: weak 16ths such as 1.1.2 and 1.1.4 give a bouncy feel; a note may be displaced to just after the kick (MusicRadar minimal kick/bass; Evosounds).
- Consequence for the generator: with a four-on-the-floor context, **no onsets on steps 1/5/9/13 and no sustain across them** (release ≈ 1/32 before the kick).

### 2.2 Rhythm vocabulary (per beat: K e & a)
- Classic house/tech offbeat: one note on every `&` (`..X-..X-..X-..X-`).
- Syncopated offbeat: bass on the "and" positions rather than every quarter (Producer School, "5 tech house bass patterns": syncopated offbeat, rolling 8ths, ghost notes, slides, stab-and-sustain).
- Rolling bass: "one note on almost every 16th" except the kick step — `.xxx.xxx.xxx.xxx` and its thinned versions; mostly 16ths for most of the bar (Producer School rolling-bass guide; Attack "warehouse rolling" techno bass).
- Minimal / rominimal: one or two core tones, elastic and subby, evolving by articulation and filter rather than new notes; long hypnotic loops with micro-variation (Melodigging rominimal profile; Electronic Beats rominimal guide).
- Ghost notes on 16ths between the main notes at 20–40 % of the main velocity, short (Beatstorapon/Abletonic guides).
- Tech house bass plays a 3–5 note pattern per bar, often syncopated against the kick (Producer School genre comparison).
- Groove research: medium syncopation maximises "wanting to move" and pleasure (inverted U; Witek et al. 2014, PLOS ONE). Too straight is boring, too syncopated loses the pulse — the evaluator should target a band, not "more".

### 2.3 Pitch vocabulary
- Minor key; the line revolves around the **tonic**. The archetypal modern rolling line uses **tonic, perfect fifth, octave** (variant: tonic, minor third, fifth); octave and fifth fill gaps between kicks and the tonic returns before the next kick (Producer School on Chris Stussy; Splice).
- Octave jumps are a core funk/disco-derived device (Attack "Jumping between octaves"; Daft Punk/Chic lineage) — fits the funky Pawsa/ANOTR side, used as flicks, not as a melody.
- Occasional chromatic passing tones add grit (Splice) — keep them rare, on weak positions, resolving.
- Variation is small: change **one note slot per bar** over a 4-bar cycle (bar 1 base, bar 2 one slot altered, bar 3 one note omitted, bar 4 base) or a short riff/half-step drop every two bars (Producer School); "keep the melody simple and repetitive with slight changes every 4–8 bars" (SampleFocus Chris Lake guide).

### 2.4 Articulation and timing
- Swing ≈ **56–58 %** on 16ths for tech house; minimal often swings the whole project; "the same notes sound stiff with shuffle off" (Producer School). Only a few micro-shifts per bar.
- Note length is a groove parameter: "most tutorials say keep notes short — that's wrong, notes need varied length"; two lengths (short/long) define the groove (Beatstorapon; Splice). Long notes are offbeat notes held up to the next kick, short notes are 16th flicks.
- Microrhythm research (Danielsen et al., TIME project; "The Organic Grid", Music Theory Spectrum 2022): envelope shape and sidechain change perceived timing — even on a strict grid, note length/attack create groove. Supports varied gates per position.
- Register: character in ≈ 80–250 Hz with a separate sub layer (Producer School): E1–A2 fundamentals.

### 2.5 Phrase
- 1–2 bar riff repeated; changes every 4–8 bars; fills at phrase ends (bar 4/8), e.g. descending scale notes in the last bar (Attack house bassline breakdown). Arrangement works by subtraction (Pawsa breakdown: bass removed in the break).

## 3. Rules adopted for algorithm v4

| # | Rule | Where |
|---|---|---|
| R1 | Four-on-the-floor: corpus pool restricted to kick-safe cells; any onset produced by a mutation on a kick step is moved to the next `e` or removed; every note released ≥ 40 ticks before the next kick. | `PatternGenerator`, `RhythmGrammar::clearKickSteps`, `PhraseComposer::renderBars` |
| R2 | Beat-position roles: `&` = main (accent, root/octave), `e` = roll/ghost, `a` = pickup into the next beat (root, colour or semitone approach). | `PitchGrammar::assignPitches` |
| R3 | Pitch: tonic ≥ ~55 % of notes; per riff 1–2 colour tones from {octave, 5th, ♭7 below, ♭3, ♭7 above, 5th below, 4th}; no 2nd-step walking except an `a` pickup resolving to the tonic; ≤ 3 distinct pitches per bar (4 for Busy/Rolling ACID). | `PitchGrammar` |
| R4 | Phrase: A A A A′ / A A′ A A″ style; B bars keep the rhythm and change only colour notes or drop to ♭VII/♭VI for one bar out of four; whole-riff 4th/5th transposition removed. | `PhraseComposer`, `PatternGenerator` |
| R5 | Two-length articulation: `&` notes long (to the next kick), `e`/`a` short; ghosts ≤ ½ step. | `renderBars` |
| R6 | Default swing 55–58 % by feel. | `RhythmGrammar` profiles |
| R7 | Evaluator: kick-lane, root-share and off-8th components; syncopation scored as a band (inverted U). | `PatternEvaluator` |

## Sources
- Loopcloud / Loopmasters — How to make a tech house beat & bass: https://www.loopcloud.com/articles/4300-How-To-Make-A-Tech-House-Beat-Bass-Tutorial
- Function Loops — How to make a tech house bassline: https://blog.functionloops.com/?p=78
- boombox — How to make tech house bassline loops: https://boombox.io/blog/how-to-make-tech-house-bassline-loops/
- The Producer School — 5 tech house bass patterns: https://theproducerschool.com/blogs/featured-blogs/5-tech-house-bass-patterns-every-producer-needs-to-master
- The Producer School — Rolling bassline like Chris Stussy (3-note pattern): https://theproducerschool.com/blogs/featured-blogs/building-a-rolling-bassline-like-chris-stussy-the-3-note-pattern-that-defines-modern-tech-house
- The Producer School — Tech house vs deep house vs minimal: https://theproducerschool.com/blogs/featured-blogs/the-producers-guide-to-tech-house-vs-deep-house-vs-minimal-the-differences-that-actually-matter
- The Producer School — Minimal house like Chris Stussy: https://theproducerschool.com/blogs/featured-blogs/how-to-make-minimal-house-like-chris-stussy-up-the-stuss-tutorial
- The Producer School — Music like PAWSA: https://theproducerschool.com/blogs/featured-blogs/how-to-make-music-like-pawsa-too-cool-to-be-careless
- Splice — How to write a bassline for different genres: https://splice.com/blog/how-to-make-basslines/
- Attack Magazine — 5 tips to improve your synth basslines: https://www.attackmagazine.com/technique/passing-notes/5-tips-to-improve-your-synth-basslines/
- Attack Magazine — Warehouse rolling techno bass: https://www.attackmagazine.com/technique/tutorials/warehouse-rolling-techno-bass/
- MusicRadar — Minimal kick/bass pattern: https://www.musicradar.com/how-to/how-to-create-a-minimal-kickbass-pattern-in-your-daw
- Evosounds — Rolling techno basslines that fit your kick: https://www.evosounds.com/post/how-to-make-rolling-techno-basslines-that-fit-your-kick
- Hot Groove Samples — Rolling basslines: https://hotgroovesamples.com/blogs/news/create-rolling-basslines-quickly
- Beatstorapon — Tech house groove and bassline: https://beatstorapon.com/blog/tech-house-production-groove-bassline/
- Abletonic — Tech house in Ableton Live: https://abletonic.vercel.app/en/blog/how-to-make-tech-house-ableton
- SampleFocus — Tech house like Chris Lake: https://samplefocus.com/blog/make-tech-house-track-chris-lake/
- Electronic Beats — Beginner's guide to rominimal: https://electronicbeats.net/guide-to-rominimal
- Melodigging — Rominimal: https://www.melodigging.com/genre/rominimal
- Witek et al. (2014), Syncopation, body-movement and pleasure in groove music, PLOS ONE 9(4): e94446: https://research.birmingham.ac.uk/en/publications/syncopation-body-movement-and-pleasure-in-groove-music/
- Brøvig-Hanssen, Sandvik, Aareskjold-Drecker, Danielsen (2022), The Organic Grid, Music Theory Spectrum 44(1): https://munin.uit.no/handle/10037/23586
- Danielsen, Brøvig, Bøhler (2024), There's more to timing than time, Music Perception: https://researchportal.plymouth.ac.uk/en/publications/theres-more-to-timing-than-time-investigating-musical-microrhythm/
- Butler (2006), Unlocking the Groove (Indiana University Press); Butler (2001), Turning the beat around, MTO 7.6: https://mtosmt.org/issues/mto.01.7.6/mto.01.7.6.butler.html
- Haki & Jordà (2019), A bassline generation system based on sequence-to-sequence learning, NIME: https://nime.org/proc/nime2019_haki/

Limits: page fetching was blocked in the development environment, so tutorial details come from search-result summaries; no audio analysis of commercial tracks was done.

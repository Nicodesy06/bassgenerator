# Golden demonstration patterns

Reproducible generator outputs (fixed settings in `tools/GoldenDemos.h`), rendered offline with the built-in synth at 48 kHz. These are QA examples, not a pattern library used by the plugin.

| Name | Settings | Feel (resolved) | Plan | Preset | BPM | Notes | Hash | Peak dBFS | RMS dBFS | Grid (X accent, x note, g ghost, - hold, ~ slide) |
|---|---|---|---|---|---|---|---|---|---|---|
| 01_sub_balanced_Am | A Natural Minor, SUB, BALANCED, 4 bar, S1201 V7 | BALANCED | A B A' F | Elastic House | 124 | 24 | 816ff0107a288d25 | -6.78671 | -16.7347 | `..Xx..X-..Xx..X.` `..Xx..X-..Xx..X.` `..X-..X-..Xx..X.` `..Xx..X-..Xx..xx`  |
| 02_sub_sparse_Fm | F Natural Minor, SUB, SPARSE, 4 bar, S3317 V2 | SPARSE | A A' B F | Round Sub | 122 | 11 | 51b4e21f16dad5a0 | -7.34026 | -19.7061 | `..X-..x-..X-..x-` `......x-..X-..x-` `..X-..........g.` `..X-...........X`  |
| 03_sub_rolling_Gm_dorian | G Dorian, SUB, ROLLING, 4 bar, S902 V11 | ROLLING | A A' B F | Elastic House | 125 | 32 | d16025d649109739 | -8.27974 | -15.7279 | `.gX-.gX-.gX-.gXx` `.gX-.gX-.gX-.gXx` `..Xx..X-..Xx..X.` `.gX-.gX-.gX-..xx`  |
| 04_sub_syncopated_Cm | C Natural Minor, SUB, SYNCOPATED, 8 bar, S4471 V5 | SYNCOPATED | A B A' F A B' A'' F2 | Punch Bass | 124 | 43 | a00b6fc16433dfd1 | -4.25374 | -16.2694 | `..X-...x-..X-.gx` `..X-...x-..X-.gx` `..Xx...x-..X-.gx` `..X-...x-..X...X` `..X-...x-..X-.gx` `..X-...x-..X-xgx` `..X-...x-g.Xg.gx` `..X-...x..gx..X-`  |
| 05_sub_stabby_Dm_phryg | D Phrygian, SUB, STABBY, 4 bar, S7717 V3 | STABBY | A B A' F | Punch Bass | 126 | 19 | f500b0d9fdab7372 | -6.27553 | -19.5549 | `.x.X..X....X..X.` `X..X.X....x..x..` `.x.X..X....X..X.` `.x.X..X....X....`  |
| 06_sub_busy_Ebm_penta | Eb Minor Pentatonic, SUB, BUSY, 2 bar, S55 V9 | BUSY | A B | Warm Analog | 123 | 20 | f1a38f4c2ad56d14 | -4.1273 | -13.6611 | `X.xxX.xX-.xX..xx` `X.xxX.xX-.xX..xx`  |
| 07_sub_balanced_Fmaj_mixo | F Mixolydian, SUB, BALANCED, 8 bar, S610 V1 | BALANCED | A B A' F A B' A'' F2 | Elastic House | 122 | 50 | 14d08f76a05e9595 | -6.71253 | -16.2384 | `X-.x..X-.x...xX.` `X-.x..X-.x...xX.` `X-xx..X-.x...xX.` `X-.x..X-.x.X-.xx` `X-.x..X-.x...xX.` `X-....X-.x...xX.` `X-.x..X-.x...xX.` `X-.x..X-..xX-.xx`  |
| 08_sub_random_Bbm | Bb Natural Minor, SUB, RANDOM, 4 bar, S8080 V4 | BALANCED | A A' B F | Round Sub | 124 | 22 | b3f7cee1b445b3ee | -7.57122 | -17.3321 | `X-.x..X-.x...xX.` `X-.x..X-.x...xX.` `..X-.xX-..X-.xx-` `X-.x..X-.x......`  |
| 09_acid_rolling_Am | A Natural Minor, ACID, ROLLING, 4 bar, S303 V12 | ROLLING | A B A' F | Rubber Acid | 128 | 32 | 668eaa2df9a70f74 | -6.11612 | -15.1256 | `..Xx..Xx..Xx..Xx` `.xX~x.X-.xX-x.X-` `..Xx..Xx..Xx..Xx` `..Xx..Xx..Xx..xx`  |
| 10_acid_busy_Em_phryg | E Phrygian, ACID, BUSY, 4 bar, S1987 V6 | BUSY | A A' A F | Sharp Acid | 130 | 39 | 433f0fa8698f51ce | -5.81047 | -17.5818 | `..Xx.gxx.gXx..xx` `...x.gxx.gXx..xx` `..Xx.gxx.gXx..xx` `..Xx.gxx.gXx..xx`  |
| 11_acid_syncopated_Dm | D Dorian, ACID, SYNCOPATED, 8 bar, S2614 V8 | SYNCOPATED | A B A' F A B' A'' F2 | Dark Acid | 126 | 48 | 2d1fc6b186992836 | -1.86924 | -8.35922 | `.x-X--.X-.Xx-.X.` `.x-X--.X-.Xx-.X.` `.x-X--.X-.Xx-.X.` `.x-X--.X-.Xx...X` `.x-X--.X-.Xx-.X.` `.x-X--.X-.Xx-.X.` `.x-X--.Xg.Xx-.X.` `.x-X--.X..x...X.`  |
| 12_acid_balanced_Cm_harm | C Harmonic Minor, ACID, BALANCED, 4 bar, S4040 V2 | BALANCED | A B A' F | Resonant Sequence | 127 | 25 | 75424aa7d7b1f5b5 | -5.71139 | -18.3287 | `X-.x..X-.x...xX.` `X-.x..X-.x...xX.` `X-.x..X-.x...xX.` `X-.x..X-..xX-.xx`  |
| 13_acid_stabby_Gm | G Natural Minor, ACID, STABBY, 2 bar, S919 V13 | STABBY | A B | Sharp Acid | 128 | 11 | 63ede97337095ce0 | -7.06872 | -19.9371 | `..xX.X....X..x..` `X.x...x.X.X...X.`  |
| 14_acid_sparse_Fsm | F# Minor Pentatonic, ACID, SPARSE, 4 bar, S5150 V1 | SPARSE | A A' A F | Rubber Acid | 125 | 15 | eeb4ce490d674a4c | -6.04083 | -17.6656 | `..X-..x-...x..x-` `..X-..x-...x..x-` `..X-..x-...x..x-` `..X-..x-...x....`  |
| 15_acid_rolling_Bm_swing | B Natural Minor, ACID, ROLLING, 4 bar, S777 V3 | ROLLING | A B A' F | Resonant Sequence | 124 | 31 | 298b079b3435e9fd | -6.13799 | -18.5784 | `..Xx..X-..Xx..Xg` `..Xx..X-..Xx..Xg` `.xXx..X-..Xx..Xg` `.xXx..X-..xX-xXx`  |
| 16_acid_random_Am_1bar | A Dorian, ACID, RANDOM, 1 bar, S31337 V1 | BUSY | A* | Dark Acid | 130 | 10 | 7895624691044cac | -2.89186 | -8.68869 | `x~xx.XX~x.xX~xx.`  |

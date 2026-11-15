# Golden demonstration patterns

Reproducible generator outputs (fixed settings in `tools/GoldenDemos.h`), rendered offline with the built-in synth at 48 kHz. These are QA examples, not a pattern library used by the plugin.

| Name | Settings | Feel (resolved) | Plan | Preset | BPM | Notes | Hash | Peak dBFS | RMS dBFS | Grid (X accent, x note, g ghost, - hold, ~ slide) |
|---|---|---|---|---|---|---|---|---|---|---|
| 01_sub_balanced_Am | A Natural Minor, SUB, BALANCED, 4 bar, S1201 V7 | BALANCED | A A' A A' | Elastic House | 124 | 24 | c6d127863666a987 | -6.78671 | -17.8084 | `..Xx..X-..Xx..X.` `..Xx..X-..Xx..X.` `..Xx..X-..Xx..X.` `..Xx..X-..Xx..X.`  |
| 02_sub_sparse_Fm | F Natural Minor, SUB, SPARSE, 4 bar, S3317 V2 | SPARSE | A A' A F | Round Sub | 122 | 14 | 2cb32d92f6b2edc | -7.31358 | -20.234 | `..X-..........gg` `..X-..........gg` `..X-..........gg` `..X-.....xX-.xX-`  |
| 03_sub_rolling_Gm_dorian | G Dorian, SUB, ROLLING, 4 bar, S902 V11 | ROLLING | A A' A F | Elastic House | 125 | 36 | e6d8f32274e71fa | -6.79132 | -18.2521 | `..Xg.xX...Xg.xXg` `..Xg.xX...Xg.xXg` `..Xg.xX...Xg.xXg` `..Xg.xX..xgx.xX-`  |
| 04_sub_syncopated_Cm | C Natural Minor, SUB, SYNCOPATED, 8 bar, S4471 V5 | SYNCOPATED | A A' A A' A A' A F2 | Punch Bass | 124 | 47 | c893cb63968dc0bc | -6.15496 | -17.4688 | `.x-x.xX-......Xx` `.x-x.xX-.x....Xx` `.x-x.xX-......Xx` `.x-x.xX-......Xx` `.x-x.xX-......Xx` `.x-x.xX-......Xx` `.x-x.xX-......Xx` `.x-x.xX-........`  |
| 05_sub_stabby_Dm_phryg | D Phrygian, SUB, STABBY, 4 bar, S7717 V3 | STABBY | A A A A' | Punch Bass | 126 | 20 | 7cf892e0280e105d | -6.27553 | -21.4417 | `.x.x..X....x..X.` `.x.x..X....x..X.` `.x.x..X....x..X.` `.x.x..X....x..X.`  |
| 06_sub_busy_Ebm_penta | Eb Minor Pentatonic, SUB, BUSY, 2 bar, S55 V9 | BUSY | A B | Warm Analog | 123 | 17 | b30b99c330a2bdb9 | -4.75911 | -15.9203 | `.x.x.x.x.xgx.xX.` `.x.x.xX-.xX-.xX-`  |
| 07_sub_balanced_Fmaj_mixo | F Mixolydian, SUB, BALANCED, 8 bar, S610 V1 | BALANCED | A A A A' A A A F | Elastic House | 122 | 49 | 102bc358ea26ba17 | -6.77791 | -17.06 | `..Xg..X-..Xx..X-` `..Xg..X-..Xx..X-` `..Xg..X-..Xx..X-` `..Xg..X-..X...X-` `..Xg..X-..Xx..X-` `..Xg..X-..Xx..X-` `..Xg..X-..Xx..X-` `..Xg..X-..Xx.xXx`  |
| 08_sub_random_Bbm | Bb Natural Minor, SUB, RANDOM, 4 bar, S8080 V4 | BALANCED | A A' A F | Round Sub | 124 | 20 | c8b046c249329669 | -7.33381 | -17.4508 | `.xX-..X-......X-` `.xX-..X-......X-` `.xX-..X-......X-` `.xX-..X-.xX..xXx`  |
| 09_acid_rolling_Am | A Natural Minor, ACID, ROLLING, 4 bar, S303 V12 | ROLLING | A A A A' | Rubber Acid | 128 | 29 | c420b5f5db691cbe | -6.11612 | -17.8255 | `..Xx..Xx..Xx..X.` `..Xx..Xx..Xx..X.` `..Xx..Xx..Xx..X.` `..Xx..Xx..Xx.gX.`  |
| 10_acid_busy_Em_phryg | E Phrygian, ACID, BUSY, 4 bar, S1987 V6 | BUSY | A A' A F | Sharp Acid | 130 | 40 | 269fbafa35bb3dae | -6.75756 | -17.5129 | `.x.x.xXg.xgx.xX-` `.x.x.xXg.xg..xX-` `.x.x.xXg.xgx.xX-` `.x.x.xXg.xgx.xXx`  |
| 11_acid_syncopated_Dm | D Dorian, ACID, SYNCOPATED, 8 bar, S2614 V8 | SYNCOPATED | A A' B F A A'' B' F2 | Dark Acid | 126 | 50 | 5832656ca68e1d34 | -2.56177 | -11.8334 | `..Xx..X..x-..xX.` `..Xx..X..x-g.xX.` `.x.x.xX-......Xx` `.x.x.xX-.....x.x` `..Xx..X..x-..xX.` `..Xx..X..x-..xX.` `.x.x.xX-......Xx` `..Xx..X..xX..xX.`  |
| 12_acid_balanced_Cm_harm | C Harmonic Minor, ACID, BALANCED, 4 bar, S4040 V2 | BALANCED | A A A A' | Resonant Sequence | 127 | 21 | 95df27eee591f178 | -6.639 | -20.0687 | `..X-..X-..Xg..X-` `..X-..X-..Xg..X-` `..X-..X-..Xg..X-` `.xX-..X-..Xg..X-`  |
| 13_acid_stabby_Gm | G Natural Minor, ACID, STABBY, 2 bar, S919 V13 | STABBY | A A' | Sharp Acid | 128 | 10 | 5d11c1d15e97ebe0 | -6.65328 | -22.6676 | `.x.x.x....X..x..` `.x.x.x....X..x..`  |
| 14_acid_sparse_Fsm | F# Minor Pentatonic, ACID, SPARSE, 4 bar, S5150 V1 | SPARSE | A B A B | Rubber Acid | 125 | 14 | e8cf6ff08639d811 | -6.13409 | -16.1359 | `.x--..X-..X-..X-` `..X-......X-..g.` `.x--..X-..X-..X-` `..X-......X-..g.`  |
| 15_acid_rolling_Bm_swing | B Natural Minor, ACID, ROLLING, 4 bar, S777 V3 | ROLLING | A B A B | Resonant Sequence | 124 | 30 | 41b1c89159220178 | -6.33603 | -20.8626 | `..Xx..X-..Xx..Xg` `.xX-.xX-.xX-..Xx` `..Xx..X-..Xx..Xg` `.xX-.xX-.xX-..Xx`  |
| 16_acid_random_Am_1bar | A Dorian, ACID, RANDOM, 1 bar, S31337 V1 | BUSY | A* | Dark Acid | 130 | 9 | 61a25c95946fa830 | -2.4222 | -11.8896 | `.xXx.xX..xX..xX.`  |

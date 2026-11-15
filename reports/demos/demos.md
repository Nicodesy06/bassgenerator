# Golden demonstration patterns

Reproducible generator outputs (fixed settings in `tools/GoldenDemos.h`), rendered offline with the built-in synth at 48 kHz. These are QA examples, not a pattern library used by the plugin.

| Name | Settings | Feel (resolved) | Plan | Preset | BPM | Notes | Hash | Peak dBFS | RMS dBFS | Grid (X accent, x note, g ghost, - hold, ~ slide) |
|---|---|---|---|---|---|---|---|---|---|---|
| 01_sub_balanced_Am | A Natural Minor, SUB, BALANCED, 4 bar, S1201 V7 | BALANCED | A B A' F | Elastic House | 124 | 26 | 1614a451faee40e5 | -6.90649 | -17.2706 | `..Xx..X-..Xx..X.` `..Xx..X-..Xx..X.` `..Xx..X-..Xx..X.` `..Xx..X-x.gxx.X-`  |
| 02_sub_sparse_Fm | F Natural Minor, SUB, SPARSE, 4 bar, S3317 V2 | SPARSE | A A' B F | Round Sub | 122 | 14 | cc637f278bb19f4e | -7.87663 | -19.5151 | `..x-..x-......X-` `..x-..X-......x-` `..x-..x-......X-` `..x-..x-....x.xx`  |
| 03_sub_rolling_Gm_dorian | G Dorian, SUB, ROLLING, 4 bar, S902 V11 | ROLLING | A B A' F | Elastic House | 125 | 33 | 4bc004f7dbb9acd8 | -8.00355 | -16.5663 | `.gX-.gX-.gx-.gXX` `..Xx..X-..Xx..X.` `.gX-.gX-.gX-.gXX` `.gX-.gX-.gx-x.xx`  |
| 04_sub_syncopated_Cm | C Natural Minor, SUB, SYNCOPATED, 8 bar, S4471 V5 | SYNCOPATED | A B A' F A B' A'' F2 | Punch Bass | 124 | 50 | 36e9c5491fa47833 | -6.9132 | -15.3556 | `.gX-.X-x-..X-..x` `.gX-.X-x-..X-..x` `.gX-.x-x-..X-..X` `.gX-.x-x-..X....` `.gX-.X-x-..X-..x` `.gX-.X-x-..X-..x` `.gX-.X-x-..X-..x` `.gX-.X-xx.x.X.xx`  |
| 05_sub_stabby_Dm_phryg | D Phrygian, SUB, STABBY, 4 bar, S7717 V3 | STABBY | A B A' F | Punch Bass | 126 | 19 | 261154cb216e6413 | -4.41682 | -21.1249 | `...X..X..x.X..X.` `X..X.X....x..x..` `...X..X..x.X..X.` `...X..X..x.X....`  |
| 06_sub_busy_Ebm_penta | Eb Minor Pentatonic, SUB, BUSY, 2 bar, S55 V9 | BUSY | A B | Warm Analog | 123 | 20 | 89e62487c7d884aa | -4.1273 | -14.6825 | `X.xxX.x.X.xxX-.x` `X.xxX.x.X.xxX-.x`  |
| 07_sub_balanced_Fmaj_mixo | F Mixolydian, SUB, BALANCED, 8 bar, S610 V1 | BALANCED | A A' A B A A' B' F2 | Elastic House | 122 | 46 | 2c3f6be08b7e1376 | -6.71253 | -16.9105 | `X-.x..X-.x...xX.` `X-.x..X-.x...xX.` `X-.x..X-.x...xX.` `X-.x..X-.x...xX.` `X-.x..X-.x...xX.` `X-.x..X-.x...xX.` `X-....X-.x...xX.` `X-.x..X-..X-..x.`  |
| 08_sub_random_Bbm | Bb Natural Minor, SUB, RANDOM, 4 bar, S8080 V4 | BALANCED | A A' B F | Round Sub | 124 | 22 | aec361f63128e4d2 | -7.60676 | -18.105 | `X-.x..X-.x...xX.` `X-.x..X-.x...xX.` `X-.x..X-.x...xX.` `X-.x..X-.x......`  |
| 09_acid_rolling_Am | A Natural Minor, ACID, ROLLING, 4 bar, S303 V12 | ROLLING | A A' B F | Rubber Acid | 128 | 34 | ac040e12e1389b4e | -5.6399 | -14.5479 | `..Xx..Xx..Xx..Xx` `..xx..XX..Xx..Xx` `.xX-x.X-.xX~x.X-` `.xX-x.X-.xX~XxXx`  |
| 10_acid_busy_Em_phryg | E Phrygian, ACID, BUSY, 4 bar, S1987 V6 | BUSY | A A' A F | Sharp Acid | 130 | 41 | d73b74bf57486083 | -5.49619 | -18.1813 | `..Xxx~xx..XxXgX.` `..Xxxgxx..XxXgX.` `..Xxx~xx..XxXgX.` `..Xxx~xx..Xxx.xx`  |
| 11_acid_syncopated_Dm | D Dorian, ACID, SYNCOPATED, 8 bar, S2614 V8 | SYNCOPATED | A B A' F A B' A'' F2 | Dark Acid | 126 | 51 | 414d8934dd07f5a1 | -2.28039 | -10.0219 | `.X-X--.X-.xX-.x.` `.X-X--.X-.xX-.x.` `.X-X--.X-.xX-.x.` `.X-X--.X-.xXX-.X` `.X-X--.X-.xX-.x.` `.X-X--.X-.xX-.x.` `.X-x--.X-.xX-.X.` `.X-X--.Xx.x.X.xx`  |
| 12_acid_balanced_Cm_harm | C Harmonic Minor, ACID, BALANCED, 4 bar, S4040 V2 | BALANCED | A B A' F | Resonant Sequence | 127 | 24 | f57b45624af9539 | -6.62195 | -21.4324 | `X-.x..X-.x...xX.` `X-.x..X-.x...xX.` `X-.x..X-.x...xX.` `X-.x..X-.x..X-.X`  |
| 13_acid_stabby_Gm | G Natural Minor, ACID, STABBY, 2 bar, S919 V13 | STABBY | A A' | Sharp Acid | 128 | 10 | e515008752cdbc8a | -5.56608 | -22.4243 | `X..x.X....X..x..` `x..X.X....X..x..`  |
| 14_acid_sparse_Fsm | F# Minor Pentatonic, ACID, SPARSE, 4 bar, S5150 V1 | SPARSE | A A' A F | Rubber Acid | 125 | 17 | b5f624089c0b852f | -6.04083 | -16.5127 | `..X-..x-..X-..x-` `..X-..x-..X-....` `..X-..x-..X-..x-` `..X-..x-..X-x.xx`  |
| 15_acid_rolling_Bm_swing | B Natural Minor, ACID, ROLLING, 4 bar, S777 V3 | ROLLING | A B A' F | Resonant Sequence | 124 | 34 | 13b37ddc737ff05a | -6.42879 | -20.5859 | `..Xx.xX-.gXx..X.` `..Xx.xX-.gXx..X.` `..Xx.xX-.gXx..Xg` `..Xx.xX-.gXxX-.X`  |
| 16_acid_random_Am_1bar | A Dorian, ACID, RANDOM, 1 bar, S31337 V1 | BUSY | A* | Dark Acid | 130 | 10 | 98e01c7e2c86c485 | -3.92373 | -8.86249 | `X~xx.xX~x.xX~xx.`  |

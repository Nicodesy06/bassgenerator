# Audio path CPU benchmark

Machine: Linux container, Intel Xeon @ 2.10 GHz (4 vCPU), GCC 13 -O3 Release, single thread, Busy 4-bar pattern, drive 0.6 / resonance 0.7, 30 s rendered per row. Command: `build/tools/gfb_benchmark`.

| Engine | Sample rate | Block | Real-time factor | CPU % of one core |
|---|---|---|---|---|
| SUB | 48000 | 32 | 88x | 1.13 % |
| SUB | 48000 | 128 | 90x | 1.11 % |
| SUB | 48000 | 512 | 79x | 1.27 % |
| SUB | 96000 | 32 | 45x | 2.22 % |
| SUB | 96000 | 128 | 46x | 2.16 % |
| SUB | 96000 | 512 | 48x | 2.07 % |
| ACID | 48000 | 32 | 100x | 1.00 % |
| ACID | 48000 | 128 | 102x | 0.98 % |
| ACID | 48000 | 512 | 106x | 0.94 % |
| ACID | 96000 | 32 | 51x | 1.94 % |
| ACID | 96000 | 128 | 51x | 1.95 % |
| ACID | 96000 | 512 | 54x | 1.87 % |

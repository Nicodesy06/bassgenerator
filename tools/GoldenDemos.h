#pragma once
// Curated QA demonstration settings. These are reproducible generator
// outputs (fixed seeds), NOT a hidden pattern library: the plugin never reads
// this list at runtime.
#include <string>
#include <vector>

#include "core/GeneratorSettings.h"

namespace gfb::demo
{
struct Demo { std::string name; GeneratorSettings s; int presetIndex; double bpm; };

inline GeneratorSettings mk (int root, ScaleType sc, Family f, Feel feel, int bars, uint64_t style, uint64_t var, int swing = kAutoSwing)
{
    GeneratorSettings s;
    s.root = root; s.scale = sc; s.family = f; s.feel = feel; s.bars = bars; s.styleSeed = style; s.varSeed = var; s.swing = swing;
    return s;
}

inline std::vector<Demo> goldenDemos()
{
    using S = ScaleType; using F = Family; using E = Feel;
    return {
        { "01_sub_balanced_Am",        mk (9, S::NaturalMinor, F::Sub, E::Balanced, 4, 1201, 7),      1, 124 },
        { "02_sub_sparse_Fm",          mk (5, S::NaturalMinor, F::Sub, E::Sparse, 4, 3317, 2),        0, 122 },
        { "03_sub_rolling_Gm_dorian",  mk (7, S::Dorian, F::Sub, E::Rolling, 4, 902, 11),             1, 125 },
        { "04_sub_syncopated_Cm",      mk (0, S::NaturalMinor, F::Sub, E::Syncopated, 8, 4471, 5),    2, 124 },
        { "05_sub_stabby_Dm_phryg",    mk (2, S::Phrygian, F::Sub, E::Stabby, 4, 7717, 3),            2, 126 },
        { "06_sub_busy_Ebm_penta",     mk (3, S::MinorPentatonic, F::Sub, E::Busy, 2, 55, 9),         3, 123 },
        { "07_sub_balanced_Fmaj_mixo", mk (5, S::Mixolydian, F::Sub, E::Balanced, 8, 610, 1),         1, 122 },
        { "08_sub_random_Bbm",         mk (10, S::NaturalMinor, F::Sub, E::Random, 4, 8080, 4),       0, 124 },
        { "09_acid_rolling_Am",        mk (9, S::NaturalMinor, F::Acid, E::Rolling, 4, 303, 12),      4, 128 },
        { "10_acid_busy_Em_phryg",     mk (4, S::Phrygian, F::Acid, E::Busy, 4, 1987, 6),             5, 130 },
        { "11_acid_syncopated_Dm",     mk (2, S::Dorian, F::Acid, E::Syncopated, 8, 2614, 8),         6, 126 },
        { "12_acid_balanced_Cm_harm",  mk (0, S::HarmonicMinor, F::Acid, E::Balanced, 4, 4040, 2),    7, 127 },
        { "13_acid_stabby_Gm",         mk (7, S::NaturalMinor, F::Acid, E::Stabby, 2, 919, 13),       5, 128 },
        { "14_acid_sparse_Fsm",        mk (6, S::MinorPentatonic, F::Acid, E::Sparse, 4, 5150, 1),    4, 125 },
        { "15_acid_rolling_Bm_swing",  mk (11, S::NaturalMinor, F::Acid, E::Rolling, 4, 777, 3, 58),  7, 124 },
        { "16_acid_random_Am_1bar",    mk (9, S::Dorian, F::Acid, E::Random, 1, 31337, 1),            6, 130 },
    };
}
} // namespace gfb::demo

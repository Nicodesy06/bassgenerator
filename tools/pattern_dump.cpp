// Prints generated patterns as a 16th-step text grid (one line per bar) so a
// human can read rhythm + pitch at a glance.
//   pattern_dump <feel> <family> <count> [bars] [complexity] [firstStyleSeed]
// Cell legend: interval from the key root in semitones (e.g. 0, 7, 12, -2),
// '>' accent, '~' slide to next, '=' note still sounding, '.' silence.
// Kick beats (steps 1,5,9,13) are marked with '|' in the header.
#include <cstdio>
#include <cstdlib>
#include <string>

#include "core/PatternGenerator.h"
#include "core/PatternMetrics.h"
#include "core/ScaleTheory.h"

using namespace gfb;

int main (int argc, char** argv)
{
    if (argc < 4) { std::fprintf (stderr, "usage: pattern_dump FEEL FAMILY COUNT [BARS] [COMPLEXITY] [SEED]\n"); return 1; }
    GeneratorSettings s;
    if (auto f = feelFromString (argv[1])) s.feel = *f;
    if (auto f = familyFromString (argv[2])) s.family = *f;
    const int count = std::atoi (argv[3]);
    s.bars = argc > 4 ? std::atoi (argv[4]) : 2;
    s.complexity = argc > 5 ? std::atoi (argv[5]) : 50;
    const uint64_t first = argc > 6 ? std::strtoull (argv[6], nullptr, 10) : 1;
    s.root = 9;
    s.scale = ScaleType::NaturalMinor;
    for (int i = 0; i < count; ++i)
    {
        s.styleSeed = first + (uint64_t) i * 7919;
        s.varSeed = 1 + (uint64_t) i;
        const auto p = generatePattern (s);
        const auto m = computeMetrics (p);
        const int rootMidi = theory::referenceRootMidi (s.root, s.family) + 12 * s.octave;
        std::printf ("#%d S%llu V%llu feel=%s arch=%d plan=[%s] notes/bar=%.1f sounding=%.2f\n", i, (unsigned long long) s.styleSeed,
                     (unsigned long long) s.varSeed, std::string (toString (p.resolvedFeel)).c_str(), p.archetypeId, p.phrasePlan.c_str(),
                     m.onsetsPerBar, m.soundingRatio);
        std::printf ("      |1         |2         |3         |4\n");
        for (int b = 0; b < p.bars; ++b)
        {
            std::string cells[16];
            for (auto& c : cells) c = ".";
            for (const auto& n : p.notes)
            {
                const int64_t st = n.startTick - (int64_t) b * kTicksPerBar;
                if (st < 0) continue;
                const int step = (int) (st / kTicksPerSixteenth); // swing only delays: floor gives the grid step
                if (step < 0 || step >= 16) continue;
                std::string c = std::to_string ((int) n.midiNote - rootMidi);
                if (n.accent) c += ">";
                if (n.slideToNext) c += "~";
                cells[step] = c;
                const int len = (int) ((n.durationTicks + kTicksPerSixteenth / 2) / kTicksPerSixteenth);
                for (int k = 1; k < len && step + k < 16; ++k) if (cells[step + k] == ".") cells[step + k] = "=";
            }
            std::printf ("  b%d  ", b + 1);
            for (int k = 0; k < 16; ++k) std::printf ("%-3s", cells[k].c_str());
            std::printf ("\n");
        }
    }
}

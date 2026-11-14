// Reference MIDI analyzer.
//
// Reads a folder of third-party bassline MIDI files the user is licensed to
// use, and writes ONLY aggregated statistics (no note data, no patterns):
// onset probability per 16th step, density, durations, intervals, scale
// degrees, register, microtiming/swing, bar repetition. The same measurements
// are computed for the GrooveForge generator so the two can be compared and
// the generator tuned. Optionally emits a C++ table of aggregate priors.
//
//   reference_analyzer <midi-dir> [--out DIR] [--emit-priors FILE] [--gen N]
//
// The reference files themselves are never copied into the repository.
#include <algorithm>
#include <cctype>
#include <array>
#include <cmath>
#include <cstdio>
#include <cstring>
#include <filesystem>
#include <fstream>
#include <map>
#include <regex>
#include <set>
#include <sstream>
#include <string>
#include <vector>

#include "core/PatternGenerator.h"
#include "core/PatternMetrics.h"
#include "core/ScaleTheory.h"
#include "midi/MidiFileExport.h"

using namespace gfb;
namespace fs = std::filesystem;

namespace
{
struct Stats
{
    int files = 0, bars = 0, notes = 0;
    std::array<double, 16> stepOnsets {};       // onsets per step position (summed over bars)
    std::array<double, 25> intervals {};        // -12..+12 semitones (clamped)
    std::array<double, 12> degrees {};          // pitch class relative to root
    std::array<double, 12> downbeatDegrees {};  // same, onsets on beats (steps 0,4,8,12)
    std::array<double, 6> durBuckets {};        // <=0.5, <=1, <=1.5, <=2, <=4, >4 (16ths)
    std::map<int, int> barCounts;
    double sumOnsetsPerBar = 0, sumSync = 0, sumOffbeat = 0, sumKick = 0, sumSounding = 0, sumMaxChain = 0,
           sumLongestRest = 0, sumDistinct = 0, sumRange = 0, sumStep = 0, sumLeap = 0, sumOct = 0, sumRepeat = 0,
           sumBarSim = 0, sumDistinctBarsRatio = 0, sumOutOfScale = 0, sumRootRatio = 0, sumFirstRoot = 0,
           sumEndsResolved = 0, sumMeanPitch = 0, sumVelDistinct = 0, sumAccent = 0, sumMeanDur = 0;
    int multiBarFiles = 0;
    std::vector<double> offGridTicks;           // |offset from nearest 16th| in ticks
    double oddOffsetSum = 0, evenOffsetSum = 0;
    int oddN = 0, evenN = 0;
};

std::string category (const fs::path& p)
{
    const std::string s = p.string();
    for (auto c : { "Knock", "Melodic", "Rolling", "Rumble", "Sustained" })
        if (s.find (std::string ("/") + c + " Basslines") != std::string::npos) return std::string ("THM ") + c;
    if (s.find ("Tech House Bassline MIDI") != std::string::npos) return "WS TechHouse";
    if (s.find ("House Bassline MIDI") != std::string::npos) return "WS House";
    return "Other";
}

/** "Gm", "D#", "F#min", "Amin" next to the BPM -> root pc + minor flag. */
bool keyFromName (const std::string& name, int& root, bool& minor)
{
    static const std::regex re ("(?:^|[ _-])([A-G])([#b]?)(min|m)?(?: |$|\\.| -)");
    std::smatch m;
    std::string s = name;
    for (auto it = std::sregex_iterator (s.begin(), s.end(), re); it != std::sregex_iterator(); ++it)
        m = *it; // last match is the key (names start with words like "Bass")
    if (m.empty()) return false;
    static const int base[] = { 9, 11, 0, 2, 4, 5, 7 }; // A..G
    root = base[m[1].str()[0] - 'A'];
    if (m[2] == "#") root = (root + 1) % 12;
    if (m[2] == "b") root = (root + 11) % 12;
    minor = m[3].matched;
    return true;
}

int estimateRoot (const std::vector<NoteEvent>& n)
{
    std::array<double, 12> w {};
    for (auto& e : n) w[(size_t) (e.midiNote % 12)] += (double) e.durationTicks + ((e.startTick % kTicksPerBar) == 0 ? 2000.0 : 0.0);
    return (int) (std::max_element (w.begin(), w.end()) - w.begin());
}

void accumulate (Stats& st, const Pattern& p, const std::vector<int>& rawOffsets)
{
    const auto m = computeMetrics (p);
    st.files++;
    st.bars += p.bars;
    st.notes += m.noteCount;
    st.barCounts[p.bars]++;
    st.sumOnsetsPerBar += m.onsetsPerBar;
    st.sumSync += m.syncopation;
    st.sumOffbeat += m.offbeatRatio;
    st.sumKick += m.kickCollision;
    st.sumSounding += m.soundingRatio;
    st.sumMaxChain += m.maxChain;
    st.sumLongestRest += m.longestRestSteps;
    st.sumDistinct += m.distinctPitches;
    st.sumRange += m.pitchRange;
    st.sumStep += m.stepRatio;
    st.sumLeap += m.leapRatio;
    st.sumOct += m.octaveRatio;
    st.sumRepeat += m.repeatRatio;
    st.sumOutOfScale += m.noteCount ? (double) m.outOfScale / m.noteCount : 0;
    st.sumRootRatio += m.rootRatio;
    st.sumFirstRoot += m.firstIsRoot ? 1 : 0;
    st.sumEndsResolved += m.endsResolved ? 1 : 0;
    st.sumMeanDur += m.meanDurSteps;
    if (p.bars > 1)
    {
        st.multiBarFiles++;
        st.sumBarSim += m.barSimilarity;
        st.sumDistinctBarsRatio += (double) m.distinctBars / p.bars;
    }
    double meanPitch = 0;
    std::set<int> vels;
    double velMean = 0;
    for (auto& e : p.notes) { meanPitch += e.midiNote; vels.insert (e.velocity); velMean += e.velocity; }
    if (! p.notes.empty()) { meanPitch /= (double) p.notes.size(); velMean /= (double) p.notes.size(); }
    st.sumMeanPitch += meanPitch;
    st.sumVelDistinct += (double) vels.size();
    int acc = 0;
    for (auto& e : p.notes) acc += e.velocity >= velMean + 10 ? 1 : 0;
    st.sumAccent += p.notes.empty() ? 0 : (double) acc / (double) p.notes.size();

    const int root = p.settings.root;
    for (size_t i = 0; i < p.notes.size(); ++i)
    {
        const auto& e = p.notes[i];
        const int step = tickToStep (e.startTick) % 16;
        st.stepOnsets[(size_t) step] += 1;
        const int deg = ((e.midiNote - root) % 12 + 12) % 12;
        st.degrees[(size_t) deg] += 1;
        if (step % 4 == 0) st.downbeatDegrees[(size_t) deg] += 1;
        const double d = (double) e.durationTicks / kTicksPerSixteenth;
        st.durBuckets[d <= 0.5 ? 0 : d <= 1 ? 1 : d <= 1.5 ? 2 : d <= 2 ? 3 : d <= 4 ? 4 : 5] += 1;
        if (i > 0) st.intervals[(size_t) (std::clamp ((int) e.midiNote - (int) p.notes[i - 1].midiNote, -12, 12) + 12)] += 1;
        const int off = rawOffsets.empty() ? 0 : rawOffsets[i];
        st.offGridTicks.push_back (std::abs (off));
        if (step % 2) { st.oddOffsetSum += off; st.oddN++; } else { st.evenOffsetSum += off; st.evenN++; }
    }
}

template <size_t N>
std::array<double, N> norm (const std::array<double, N>& a)
{
    double s = 0;
    for (double x : a) s += x;
    std::array<double, N> r {};
    for (size_t i = 0; i < N; ++i) r[i] = s > 0 ? a[i] / s : 0;
    return r;
}

std::string pct (double x) { char b[32]; std::snprintf (b, sizeof b, "%.0f%%", 100.0 * x); return b; }
std::string f2 (double x) { char b[32]; std::snprintf (b, sizeof b, "%.2f", x); return b; }

void writeSummaryRow (std::ostream& o, const std::string& name, const Stats& s)
{
    const double F = std::max (1, s.files), M = std::max (1, s.multiBarFiles);
    o << "| " << name << " | " << s.files << " | " << f2 ((double) s.bars / F) << " | " << f2 (s.sumOnsetsPerBar / F) << " | " << f2 (s.sumSync / F)
      << " | " << pct (s.sumOffbeat / F) << " | " << pct (s.sumKick / F) << " | " << f2 (s.sumMeanDur / F) << " | " << pct (s.sumSounding / F)
      << " | " << f2 (s.sumMaxChain / F) << " | " << f2 (s.sumLongestRest / F) << " | " << f2 (s.sumDistinct / F) << " | " << f2 (s.sumRange / F)
      << " | " << pct (s.sumStep / F) << " | " << pct (s.sumOct / F) << " | " << pct (s.sumRepeat / F) << " | " << pct (s.sumRootRatio / F)
      << " | " << pct (s.sumOutOfScale / F) << " | " << (s.multiBarFiles ? f2 (s.sumBarSim / M) : "-") << " | "
      << (s.multiBarFiles ? pct (s.sumDistinctBarsRatio / M) : "-") << " | " << f2 (s.sumVelDistinct / F) << " | " << f2 (s.sumMeanPitch / F) << " |\n";
}

const char* kHeader = "| Group | files | bars | onsets/bar | sync | off-8th | on beats 2-4 | mean dur (16ths) | sounding | max chain | longest rest | distinct pitches | range | steps 1-2 | octave jumps | repeats | root notes | out of scale | bar similarity | distinct bars | velocity values | mean MIDI pitch |\n"
                      "|---|---|---|---|---|---|---|---|---|---|---|---|---|---|---|---|---|---|---|---|---|---|\n";

std::string stepRow (const std::string& name, const Stats& s)
{
    std::ostringstream o;
    o << "| " << name << " |";
    for (int i = 0; i < 16; ++i) o << ' ' << pct (s.bars ? s.stepOnsets[(size_t) i] / s.bars : 0) << " |";
    return o.str() + "\n";
}
} // namespace

int main (int argc, char** argv)
{
    if (argc < 2) { std::printf ("usage: reference_analyzer <midi-dir> [--out DIR] [--emit-priors FILE] [--gen N]\n"); return 2; }
    std::string out = "reports/reference", priorsFile;
    int genN = 300;
    for (int i = 2; i < argc; ++i)
    {
        if (! std::strcmp (argv[i], "--out") && i + 1 < argc) out = argv[++i];
        else if (! std::strcmp (argv[i], "--emit-priors") && i + 1 < argc) priorsFile = argv[++i];
        else if (! std::strcmp (argv[i], "--gen") && i + 1 < argc) genN = std::atoi (argv[++i]);
    }
    fs::create_directories (out);

    std::map<std::string, Stats> groups;
    Stats all, fourOnFloor, broken; // priors: Weapon Sounds house/tech house vs THM + other
    std::set<std::string> seen;
    int failed = 0, chordTrimmed = 0, keyFromFile = 0;
    std::vector<fs::path> files;
    for (auto& e : fs::recursive_directory_iterator (argv[1]))
        if (e.is_regular_file() && (e.path().extension() == ".mid" || e.path().extension() == ".MID" || e.path().extension() == ".midi"))
            files.push_back (e.path());
    std::sort (files.begin(), files.end());
    for (const auto& path : files)
    {
        std::ifstream f (path, std::ios::binary);
        std::vector<uint8_t> bytes ((std::istreambuf_iterator<char> (f)), {});
        const std::string digest = std::to_string (std::hash<std::string> {} (std::string (bytes.begin(), bytes.end())));
        if (! seen.insert (digest).second) continue; // duplicate content
        auto r = midi::readSmf (bytes);
        if (! r.error.empty() || r.notes.empty() || r.ppq <= 0) { ++failed; std::printf ("skip %s: %s\n", path.filename().c_str(), r.error.c_str()); continue; }
        // rescale to PPQ 960, keep the lowest note of simultaneous onsets (mono bass line)
        std::vector<NoteEvent> notes;
        std::vector<int> offsets;
        for (auto n : r.notes)
        {
            n.startTick = n.startTick * kPPQ / r.ppq;
            n.durationTicks = std::max<int64_t> (1, n.durationTicks * kPPQ / r.ppq);
            if (! notes.empty() && notes.back().startTick == n.startTick)
            {
                ++chordTrimmed;
                if (n.midiNote < notes.back().midiNote) notes.back() = n;
                continue;
            }
            notes.push_back (n);
        }
        // shift so the clip starts on a bar line (some files have a leading offset)
        const int64_t firstBar = (notes.front().startTick / kTicksPerBar) * kTicksPerBar;
        for (auto& n : notes) n.startTick -= firstBar;
        for (auto& n : notes) offsets.push_back ((int) (n.startTick - (int64_t) tickToStep (n.startTick) * kTicksPerSixteenth));
        Pattern p;
        int64_t end = 0;
        for (auto& n : notes) end = std::max (end, n.endTick());
        p.bars = (int) std::clamp<int64_t> ((end + kTicksPerBar - 1 - 120) / kTicksPerBar, 1, 64);
        p.notes = notes;
        int root;
        bool minor;
        if (keyFromName (path.filename().string(), root, minor)) ++keyFromFile;
        else { root = estimateRoot (notes); minor = true; }
        p.settings.root = root;
        p.settings.scale = minor ? ScaleType::NaturalMinor : ScaleType::Major;
        const auto cat = category (path);
        accumulate (groups[cat], p, offsets);
        accumulate (all, p, offsets);
        accumulate (cat.rfind ("WS", 0) == 0 ? fourOnFloor : broken, p, offsets);
    }

    // generator, same measurements (4 bars, random keys/seeds, both families)
    std::map<std::string, Stats> gen;
    for (int fam = 0; fam < 2; ++fam)
        for (int feel = 1; feel < kNumFeels; ++feel)
            for (int i = 0; i < genN; ++i)
            {
                GeneratorSettings s;
                s.family = (Family) fam; s.feel = (Feel) feel; s.bars = 4; s.root = i % 12;
                s.scale = i % 2 ? ScaleType::NaturalMinor : ScaleType::Dorian;
                s.styleSeed = (uint64_t) i * 7919 + 13; s.varSeed = (uint64_t) i * 31 + 7;
                const auto p = generatePattern (s);
                std::vector<int> offs;
                for (auto& n : p.notes) offs.push_back ((int) (n.startTick - (int64_t) tickToStep (n.startTick) * kTicksPerSixteenth));
                const std::string key = std::string ("GFB ") + std::string (toString ((Family) fam)) + " " + std::string (toString ((Feel) feel));
                accumulate (gen[key], p, offs);
                accumulate (gen["GFB all"], p, offs);
            }

    std::ostringstream md;
    md << "# Reference bassline statistics vs. GrooveForge generator\n\n"
       << "Aggregated measurements only - no reference note data is stored. Reference set: " << all.files << " unique MIDI files ("
       << failed << " unreadable skipped, " << keyFromFile << " with key in the file name, " << chordTrimmed << " stacked notes reduced to the lowest pitch). "
       << "Generator: " << genN << " patterns per family x feel, 4 bars.\n\n## Global descriptors\n\n" << kHeader;
    writeSummaryRow (md, "**REFERENCE all**", all);
    for (auto& [k, s] : groups) writeSummaryRow (md, "ref " + k, s);
    for (auto& [k, s] : gen) writeSummaryRow (md, k, s);

    md << "\n## Onset probability per 16th step (share of bars with a note starting there)\n\n| Group |";
    for (int i = 0; i < 16; ++i) md << ' ' << (i / 4 + 1) << '.' << (i % 4 + 1) << " |";
    md << "\n|---|";
    for (int i = 0; i < 16; ++i) md << "---|";
    md << "\n" << stepRow ("**REFERENCE all**", all);
    for (auto& [k, s] : groups) md << stepRow ("ref " + k, s);
    for (auto& [k, s] : gen) md << stepRow (k, s);

    auto distRow = [&] (const std::string& name, auto arr) {
        std::ostringstream o;
        o << "| " << name << " |";
        for (double x : norm (arr)) o << ' ' << pct (x) << " |";
        return o.str() + "\n";
    };
    md << "\n## Note durations (16ths)\n\n| Group | <=0.5 | <=1 | <=1.5 | <=2 | <=4 | >4 |\n|---|---|---|---|---|---|---|\n" << distRow ("**REFERENCE all**", all.durBuckets);
    for (auto& [k, s] : groups) md << distRow ("ref " + k, s.durBuckets);
    for (auto& [k, s] : gen) md << distRow (k, s.durBuckets);

    static const char* deg[12] = { "1", "b2", "2", "b3", "3", "4", "#4", "5", "b6", "6", "b7", "7" };
    md << "\n## Pitch class relative to the key root (all notes | notes on beats)\n\n| Group |";
    for (auto d : deg) md << ' ' << d << " |";
    md << "\n|---|";
    for (int i = 0; i < 12; ++i) md << "---|";
    md << "\n" << distRow ("**REFERENCE all**", all.degrees) << distRow ("REFERENCE on beats", all.downbeatDegrees);
    for (auto& [k, s] : gen) if (k == "GFB all" || k.find ("BALANCED") != std::string::npos) md << distRow (k, s.degrees);

    md << "\n## Melodic intervals between consecutive notes (semitones -12..+12)\n\n| Group |";
    for (int i = -12; i <= 12; ++i) md << ' ' << i << " |";
    md << "\n|---|";
    for (int i = 0; i < 25; ++i) md << "---|";
    md << "\n" << distRow ("**REFERENCE all**", all.intervals);
    for (auto& [k, s] : groups) md << distRow ("ref " + k, s.intervals);
    md << distRow ("GFB all", gen["GFB all"].intervals);

    auto timing = [] (const Stats& s) {
        std::vector<double> v = s.offGridTicks;
        std::sort (v.begin(), v.end());
        std::ostringstream o;
        const double onGrid = v.empty() ? 0 : (double) std::count_if (v.begin(), v.end(), [] (double x) { return x <= 2; }) / (double) v.size();
        o << "on grid (+-2 ticks): " << pct (onGrid) << ", median |offset| " << (v.empty() ? 0 : v[v.size() / 2]) << " ticks, mean offset odd 16ths "
          << f2 (s.oddN ? s.oddOffsetSum / s.oddN : 0) << " vs even " << f2 (s.evenN ? s.evenOffsetSum / s.evenN : 0) << " ticks (PPQ 960)";
        return o.str();
    };
    md << "\n## Timing\n\n- REFERENCE: " << timing (all) << "\n- GFB: " << timing (gen["GFB all"]) << "\n\n## Phrase lengths (reference)\n\n";
    for (auto& [b, c] : all.barCounts) md << "- " << b << " bar(s): " << c << " files\n";

    std::ofstream (out + "/reference_vs_generator.md") << md.str();
    std::printf ("%s", md.str().c_str());

    if (! priorsFile.empty())
    {
        // Aggregate priors only (16 step probabilities + 25 interval probabilities, permille).
        std::ofstream pf (priorsFile);
        pf << "// Generated by tools/reference_analyzer from " << all.files
           << " licensed reference MIDI files. Aggregate statistics only: no notes or patterns.\n";
        pf << "// onset probability per 16th step (permille of bars), and interval distribution (permille, -12..+12)\n";
        auto emit = [&] (const char* name, const Stats& s) {
            pf << "inline constexpr std::array<int, 16> " << name << "Steps { ";
            for (int i = 0; i < 16; ++i) pf << (int) std::lround (1000.0 * (s.bars ? s.stepOnsets[(size_t) i] / s.bars : 0)) << (i < 15 ? ", " : " ");
            pf << "};\ninline constexpr std::array<int, 25> " << name << "Intervals { ";
            const auto n = norm (s.intervals);
            for (int i = 0; i < 25; ++i) pf << (int) std::lround (1000.0 * n[(size_t) i]) << (i < 24 ? ", " : " ");
            pf << "};\n";
        };
        pf << "// FourOnFloor: " << fourOnFloor.files << " house/tech-house files; Broken: " << broken.files << " other files.\n";
        emit ("kRefAll", all);
        emit ("kRefFourOnFloor", fourOnFloor);
        emit ("kRefBroken", broken);
        std::printf ("priors written to %s\n", priorsFile.c_str());
    }
    return 0;
}

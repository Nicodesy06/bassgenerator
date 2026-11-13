// Offline renderer: golden QA demos and blind-audition sets.
//   render_demo --out DIR                 16 golden demos (.mid + .wav + demos.md)
//   render_demo --audition N --out DIR    N random unedited examples, anonymised
//                                         file names + separate answer key CSV
#include <cmath>
#include <cstdio>
#include <cstring>
#include <filesystem>
#include <fstream>
#include <sstream>
#include <string>
#include <vector>

#include "GoldenDemos.h"
#include "core/PatternGenerator.h"
#include "core/PatternMetrics.h"
#include "core/SeedRng.h"
#include "dsp/BassSynth.h"
#include "dsp/NoteScheduler.h"
#include "midi/MidiFileExport.h"
#include "core/ScaleTheory.h"
#include <algorithm>
#include <memory>

using namespace gfb;

namespace
{
void writeWav (const std::string& path, const std::vector<float>& l, const std::vector<float>& r, int sr)
{
    std::ofstream f (path, std::ios::binary);
    auto u32 = [&] (uint32_t v) { f.write ((const char*) &v, 4); };
    auto u16 = [&] (uint16_t v) { f.write ((const char*) &v, 2); };
    const uint32_t frames = (uint32_t) l.size();
    f.write ("RIFF", 4); u32 (36 + frames * 4); f.write ("WAVE", 4);
    f.write ("fmt ", 4); u32 (16); u16 (1); u16 (2); u32 ((uint32_t) sr); u32 ((uint32_t) sr * 4); u16 (4); u16 (16);
    f.write ("data", 4); u32 (frames * 4);
    for (uint32_t i = 0; i < frames; ++i)
        for (float v : { l[i], r[i] })
        {
            const int16_t s = (int16_t) std::lround (std::clamp (v, -1.0f, 1.0f) * 32767.0f);
            f.write ((const char*) &s, 2);
        }
}

struct RenderStats { double peak = 0, rms = 0; bool finite = true; };

RenderStats renderPattern (const Pattern& p, const SynthParams& params, double bpm, int loops, int sr, std::vector<float>& l, std::vector<float>& r)
{
    const double seconds = loops * p.bars * 4 * 60.0 / bpm + 0.5;
    const int total = (int) (seconds * sr);
    l.assign ((size_t) total, 0.0f);
    r.assign ((size_t) total, 0.0f);
    auto sched = std::make_unique<NoteSchedule>();
    sched->fromPattern (p, 1);
    NoteScheduler sc;
    sc.prepare (sr);
    auto syn = std::make_unique<dsp::BassSynth>();
    syn->prepare (sr, 256);
    SynthEvent ev[NoteScheduler::kMaxEvents];
    const double loopPpq = p.bars * 4.0 * loops;
    for (int pos = 0; pos < total; pos += 256)
    {
        const int n = std::min (256, total - pos);
        const double ppq = pos * bpm / 60.0 / sr;
        TransportState t { ppq < loopPpq, ppq, bpm };
        const int k = sc.process (*sched, t, n, ev, NoteScheduler::kMaxEvents);
        syn->process (l.data() + pos, r.data() + pos, n, ev, k, params);
    }
    RenderStats st;
    double acc = 0;
    for (float v : l) { st.finite = st.finite && std::isfinite (v); st.peak = std::max (st.peak, (double) std::abs (v)); acc += (double) v * v; }
    st.rms = std::sqrt (acc / std::max<size_t> (1, l.size()));
    return st;
}

std::string pitchSummary (const Pattern& p)
{
    std::ostringstream o;
    for (int b = 0; b < p.bars; ++b)
    {
        std::string g (16, '.');
        for (auto& n : p.notes)
        {
            const int st = tickToStep (n.startTick);
            if (st / 16 != b) continue;
            g[(size_t) (st % 16)] = n.accent ? 'X' : (n.velocity < 70 ? 'g' : 'x');
            const int len = (int) ((n.durationTicks + 120) / 240);
            for (int k = 1; k < len && st % 16 + k < 16; ++k) g[(size_t) (st % 16 + k)] = (n.slideToNext && k == len - 1) ? '~' : '-';
        }
        o << "`" << g << "` ";
    }
    return o.str();
}
} // namespace

int main (int argc, char** argv)
{
    std::string out = "reports/demos";
    int audition = 0;
    for (int i = 1; i < argc; ++i)
    {
        if (! std::strcmp (argv[i], "--out") && i + 1 < argc) out = argv[++i];
        else if (! std::strcmp (argv[i], "--audition") && i + 1 < argc) audition = std::atoi (argv[++i]);
    }
    std::filesystem::create_directories (out);
    const auto presets = factoryPresets();
    const int sr = 48000;
    std::vector<float> l, r;
    int failures = 0;

    if (audition > 0)
    {
        // Blind protocol: random unedited examples, balanced over 2 families x 6 feels,
        // anonymised names; answer key kept in a separate file for the test leader.
        std::ofstream key (out + "/answer_key.csv");
        std::ofstream sheet (out + "/rating_sheet.csv");
        key << "clip,family,feel,bars,root,scale,style,var,preset\n";
        sheet << "clip,groove(1-5),usability(1-5),articulation(1-5),loop_coherence(1-5),originality(1-5),notes\n";
        Rng rng (deriveSeed ((uint64_t) audition, "audition"));
        for (int i = 0; i < audition; ++i)
        {
            GeneratorSettings s;
            s.family = (Family) (i % 2);
            s.feel = (Feel) (1 + (i / 2) % 6);
            s.bars = 4;
            s.root = rng.range (0, 11);
            const int scales[] = { 0, 0, 2, 3, 6, 5 };
            s.scale = (ScaleType) scales[rng.below (6)];
            s.styleSeed = (uint64_t) rng.range (1, 999999);
            s.varSeed = (uint64_t) rng.range (1, 999999);
            const auto p = generatePattern (s);
            const int preset = s.family == Family::Sub ? rng.range (0, 3) : rng.range (4, 7);
            char clip[32];
            std::snprintf (clip, sizeof clip, "clip_%03u", (unsigned) (deriveSeed (s.styleSeed, "clip", (uint64_t) i) % 1000));
            const std::string name = std::string (clip) + "_" + std::to_string (i);
            const auto st = renderPattern (p, presets[(size_t) preset].params, 124, 2, sr, l, r);
            failures += st.finite ? 0 : 1;
            writeWav (out + "/" + name + ".wav", l, r, sr);
            key << name << ',' << toString (s.family) << ',' << toString (s.feel) << ',' << s.bars << ',' << s.root << ',' << toString (s.scale) << ','
                << s.styleSeed << ',' << s.varSeed << ',' << presets[(size_t) preset].name << "\n";
            sheet << name << ",,,,,,\n";
        }
        std::printf ("wrote %d audition clips to %s (answer_key.csv is for the test leader only)\n", audition, out.c_str());
        return failures == 0 ? 0 : 1;
    }

    std::filesystem::create_directories (out + "/midi");
    std::filesystem::create_directories (out + "/audio");
    std::ofstream md (out + "/demos.md");
    md << "# Golden demonstration patterns\n\nReproducible generator outputs (fixed settings in `tools/GoldenDemos.h`), rendered offline with the built-in synth at 48 kHz. "
          "These are QA examples, not a pattern library used by the plugin.\n\n"
          "| Name | Settings | Feel (resolved) | Plan | Preset | BPM | Notes | Hash | Peak dBFS | RMS dBFS | Grid (X accent, x note, g ghost, - hold, ~ slide) |\n"
          "|---|---|---|---|---|---|---|---|---|---|---|\n";
    for (const auto& d : demo::goldenDemos())
    {
        auto p = generatePattern (d.s);
        p.tempoBpm = d.bpm;
        midi::ExportOptions eo;
        eo.tempoBpm = d.bpm;
        eo.trackName = "GrooveForge " + d.name;
        const auto bytes = midi::writeSmf (p, eo);
        std::ofstream (out + "/midi/" + d.name + ".mid", std::ios::binary).write ((const char*) bytes.data(), (std::streamsize) bytes.size());
        const auto& preset = presets[(size_t) d.presetIndex];
        const auto st = renderPattern (p, preset.params, d.bpm, std::max (1, 8 / p.bars), sr, l, r);
        failures += (st.finite && st.peak < 1.0 && st.rms > 0.005) ? 0 : 1;
        writeWav (out + "/audio/" + d.name + ".wav", l, r, sr);
        md << "| " << d.name << " | " << theory::rootName (d.s.root) << ' ' << scaleDisplayName (d.s.scale) << ", " << toString (d.s.family) << ", "
           << toString (d.s.feel) << ", " << d.s.bars << " bar, S" << d.s.styleSeed << " V" << d.s.varSeed << " | " << toString (p.resolvedFeel) << " | "
           << p.phrasePlan << " | " << preset.name << " | " << d.bpm << " | " << p.notes.size() << " | " << std::hex << p.hash << std::dec << " | "
           << 20 * std::log10 (std::max (1e-9, st.peak)) << " | " << 20 * std::log10 (std::max (1e-9, st.rms)) << " | " << pitchSummary (p) << " |\n";
        std::printf ("%-28s notes %3zu peak %.3f rms %.4f %s\n", d.name.c_str(), p.notes.size(), st.peak, st.rms, st.finite ? "" : "NON-FINITE");
    }
    return failures == 0 ? 0 : 1;
}

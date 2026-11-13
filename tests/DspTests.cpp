#include <cmath>
#include <vector>

#include "TestFramework.h"
#include "dsp/BassSynth.h"

using namespace gfb;

namespace
{
constexpr double kPi = 3.14159265358979323846;
struct Note { int at; int len; int note; int vel; bool accent; bool legato; };

std::vector<float> render (const SynthParams& p, const std::vector<Note>& notes, double sr, int totalSamples, int blockSize = 256)
{
    dsp::BassSynth syn;
    syn.prepare (sr, blockSize);
    std::vector<float> out ((size_t) totalSamples, 0.0f), right ((size_t) totalSamples, 0.0f);
    std::vector<std::pair<int, SynthEvent>> all;
    for (auto& n : notes)
    {
        SynthEvent on; on.type = SynthEvent::Type::NoteOn; on.note = (uint8_t) n.note; on.velocity = (uint8_t) n.vel; on.accent = n.accent; on.legato = n.legato;
        SynthEvent off; off.type = SynthEvent::Type::NoteOff; off.note = (uint8_t) n.note;
        all.push_back ({ n.at, on });
        all.push_back ({ n.at + n.len, off });
    }
    std::stable_sort (all.begin(), all.end(), [] (auto& a, auto& b) { return a.first < b.first; });
    size_t ei = 0;
    for (int pos = 0; pos < totalSamples; pos += blockSize)
    {
        const int n = std::min (blockSize, totalSamples - pos);
        std::vector<SynthEvent> ev;
        while (ei < all.size() && all[ei].first < pos + n) { auto e = all[ei++].second; e.sampleOffset = std::max (0, all[ei - 1].first - pos); ev.push_back (e); }
        syn.process (out.data() + pos, right.data() + pos, n, ev.data(), (int) ev.size(), p);
        for (int i = 0; i < n; ++i) CHECK (out[(size_t) (pos + i)] == right[(size_t) (pos + i)]); // mono-compatible
    }
    return out;
}

double rms (const std::vector<float>& x, size_t from = 0, size_t to = 0)
{
    if (to == 0 || to > x.size()) to = x.size();
    double s = 0;
    for (size_t i = from; i < to; ++i) s += (double) x[i] * x[i];
    return std::sqrt (s / std::max<size_t> (1, to - from));
}

/** spectral centroid (Hz) via naive DFT on a window */
double centroid (const std::vector<float>& x, double sr, size_t from, size_t len = 4096)
{
    if (from + len > x.size()) len = x.size() - from;
    double num = 0, den = 0;
    for (size_t k = 1; k < len / 2; k += 2)
    {
        double re = 0, im = 0;
        for (size_t n = 0; n < len; ++n)
        {
            const double w = 0.5 - 0.5 * std::cos (2 * kPi * (double) n / (double) len);
            const double ph = 2 * kPi * (double) k * (double) n / (double) len;
            re += w * x[from + n] * std::cos (ph);
            im -= w * x[from + n] * std::sin (ph);
        }
        const double mag = std::sqrt (re * re + im * im);
        num += mag * (double) k * sr / (double) len;
        den += mag;
    }
    return den > 0 ? num / den : 0;
}

SynthParams base (SynthEngine e)
{
    SynthParams p;
    p.engine = e;
    p.volumeDb = -6;
    return p;
}

const std::vector<Note> oneNote { { 100, 20000, 40, 100, false, false } };
} // namespace

TEST_CASE (Dsp, SilentWithoutNotes)
{
    for (int e = 0; e < 2; ++e)
    {
        const auto y = render (base ((SynthEngine) e), {}, 48000, 48000);
        double peak = 0;
        for (float v : y) peak = std::max (peak, (double) std::abs (v));
        CHECK (peak < 1e-6);
    }
}

TEST_CASE (Dsp, AudibleWithNotesAndReturnsToSilence)
{
    for (int e = 0; e < 2; ++e)
    {
        const auto y = render (base ((SynthEngine) e), oneNote, 48000, 48000);
        CHECK (rms (y, 1000, 15000) > 0.02);
        CHECK (rms (y, 30000, 48000) < 1e-4); // released
        double mean = 0;
        for (size_t i = 1000; i < 15000; ++i) mean += y[i];
        CHECK (std::abs (mean / 14000.0) < 0.01); // no significant DC
    }
}

TEST_CASE (Dsp, FiniteAndBoundedAtParameterExtremes)
{
    for (double sr : { 44100.0, 48000.0, 96000.0 })
        for (int e = 0; e < 2; ++e)
            for (int mask = 0; mask < 64; mask += 3)
            {
                SynthParams p = base ((SynthEngine) e);
                p.cutoff = (mask & 1) ? 1.f : 0.f; p.resonance = (mask & 2) ? 1.f : 0.f; p.envMod = (mask & 4) ? 1.f : 0.f;
                p.decay = (mask & 8) ? 1.f : 0.f; p.accent = (mask & 16) ? 1.f : 0.f; p.drive = (mask & 32) ? 1.f : 0.f;
                p.glide = 1.f; p.shape = (mask & 1) ? 0.f : 1.f; p.volumeDb = 6.f;
                std::vector<Note> ns { { 0, 3000, 24, 127, true, false }, { 2900, 3000, 96, 127, true, true }, { 6000, 5000, 127, 1, false, false } };
                const auto y = render (p, ns, sr, (int) (sr * 0.3));
                bool finite = true; double peak = 0;
                for (float v : y) { finite = finite && std::isfinite (v); peak = std::max (peak, (double) std::abs (v)); }
                CHECK (finite);
                CHECK (peak <= 2.0);
            }
}

TEST_CASE (Dsp, SubAndAcidAreSpectrallyDistinct)
{
    auto sp = base (SynthEngine::Sub), ap = base (SynthEngine::Acid);
    sp.cutoff = ap.cutoff = 0.5f; sp.resonance = ap.resonance = 0.5f; sp.envMod = ap.envMod = 0.5f; sp.shape = 0.3f; ap.shape = 0.0f;
    const auto ys = render (sp, oneNote, 48000, 24000), ya = render (ap, oneNote, 48000, 24000);
    const double cs = centroid (ys, 48000, 2000), ca = centroid (ya, 48000, 2000);
    std::printf ("    centroid SUB %.0f Hz, ACID %.0f Hz\n", cs, ca);
    CHECK (ca > cs * 1.8);
}

TEST_CASE (Dsp, EveryMacroHasMeasurableEffect)
{
    const double sr = 48000;
    for (int e = 0; e < 2; ++e)
    {
        const auto eng = (SynthEngine) e;
        auto lo = base (eng), hi = base (eng);
        // Cutoff
        lo.cutoff = 0.2f; hi.cutoff = 0.8f; lo.envMod = hi.envMod = 0.0f;
        CHECK (centroid (render (hi, oneNote, sr, 12000), sr, 4000) > centroid (render (lo, oneNote, sr, 12000), sr, 4000) * 1.3);
        // Resonance: spectrum/level changes
        lo = hi = base (eng); lo.resonance = 0.0f; hi.resonance = 1.0f; lo.cutoff = hi.cutoff = 0.45f;
        { const auto a = render (lo, oneNote, sr, 12000), b = render (hi, oneNote, sr, 12000);
          double diff = 0; for (size_t i = 4000; i < 12000; ++i) diff += std::abs (a[i] - b[i]);
          CHECK (diff / 8000.0 > 0.003); }
        // Env Mod: brighter attack
        lo = hi = base (eng); lo.envMod = 0.0f; hi.envMod = 1.0f; lo.cutoff = hi.cutoff = 0.3f;
        CHECK (centroid (render (hi, oneNote, sr, 6000), sr, 200, 2048) > centroid (render (lo, oneNote, sr, 6000), sr, 200, 2048) * 1.15);
        // Decay: longer decay keeps the filter open/amp higher later in the note
        lo = hi = base (eng); lo.decay = 0.0f; hi.decay = 1.0f; lo.envMod = hi.envMod = 0.8f; lo.cutoff = hi.cutoff = 0.3f;
        { const auto a = render (lo, oneNote, sr, 20000), b = render (hi, oneNote, sr, 20000);
          CHECK (centroid (b, sr, 8000, 2048) > centroid (a, sr, 8000, 2048) * 1.1 || rms (b, 8000, 12000) > rms (a, 8000, 12000) * 1.1); }
        // Accent: accented notes are louder/brighter when Accent > 0, no difference at Accent == 0
        { auto p = base (eng); p.accent = 1.0f;
          const std::vector<Note> acc { { 100, 8000, 40, 100, true, false } };
          CHECK (rms (render (p, acc, sr, 9000), 200, 4000) > rms (render (p, oneNote, sr, 9000), 200, 4000) * 1.2);
          p.accent = 0.0f;
          CHECK_NEAR (rms (render (p, acc, sr, 9000), 200, 4000), rms (render (p, oneNote, sr, 9000), 200, 4000), 0.02); }
        // Drive: more harmonics, level kept within +-6 dB
        lo = hi = base (eng); lo.drive = 0.0f; hi.drive = 1.0f; lo.cutoff = hi.cutoff = 0.4f;
        { const auto a = render (lo, oneNote, sr, 12000), b = render (hi, oneNote, sr, 12000);
          CHECK (centroid (b, sr, 4000) > centroid (a, sr, 4000) * 1.1);
          const double ratio = rms (b, 4000, 12000) / rms (a, 4000, 12000);
          std::printf ("    %s drive level ratio %.2f\n", e ? "ACID" : "SUB", ratio);
          CHECK (ratio > 0.5 && ratio < 2.0); }
        // Volume: -12 dB vs 0 dB
        lo = hi = base (eng); lo.volumeDb = -12.0f; hi.volumeDb = 0.0f;
        CHECK_NEAR (rms (render (hi, oneNote, sr, 12000), 4000, 12000) / rms (render (lo, oneNote, sr, 12000), 4000, 12000), 3.98, 0.2);
        // Glide: legato slide takes longer with higher glide
        for (float g : { 0.0f, 1.0f })
        {
            dsp::BassVoice v;
            v.prepare (sr);
            auto p = base (eng); p.glide = g;
            std::vector<float> buf (4800);
            v.noteOn (40, 0.8f, false, false, p);
            v.render (buf.data(), 2400, p);
            v.noteOn (52, 0.8f, false, true, p);
            v.render (buf.data(), 480, p); // 10 ms later
            const float semis = v.currentPitchSemis();
            if (g == 0.0f) CHECK (semis > 51.0f);
            else CHECK (semis < 46.0f && semis > 40.0f);
        }
    }
}

TEST_CASE (Dsp, BlockSizeDoesNotChangeOutput)
{
    for (int e = 0; e < 2; ++e)
    {
        std::vector<Note> ns { { 0, 5000, 40, 100, true, false }, { 4900, 3000, 47, 90, false, true }, { 9000, 2000, 35, 60, false, false } };
        const auto ref = render (base ((SynthEngine) e), ns, 48000, 16000, 1024);
        for (int bs : { 32, 64, 128, 256, 512 })
        {
            const auto y = render (base ((SynthEngine) e), ns, 48000, 16000, bs);
            double maxDiff = 0;
            for (size_t i = 0; i < y.size(); ++i) maxDiff = std::max (maxDiff, (double) std::abs (y[i] - ref[i]));
            CHECK_MSG (maxDiff < 1e-5, "bs=" + std::to_string (bs) + " diff=" + std::to_string (maxDiff));
        }
    }
}

TEST_CASE (Dsp, PresetsDiffer)
{
    const auto presets = factoryPresets();
    std::vector<double> cents;
    for (auto& pr : presets) cents.push_back (centroid (render (pr.params, oneNote, 48000, 12000), 48000, 3000));
    int distinct = 0;
    for (size_t i = 0; i < cents.size(); ++i)
        for (size_t j = i + 1; j < cents.size(); ++j) distinct += std::abs (cents[i] - cents[j]) > 5.0 ? 1 : 0;
    CHECK (distinct >= 24); // nearly all 28 pairs differ
}

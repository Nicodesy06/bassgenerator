#include "midi/MidiFileExport.h"

#include <algorithm>
#include <cmath>

#include "core/ScaleTheory.h"

namespace gfb::midi
{
namespace
{
void be16 (std::vector<uint8_t>& v, uint32_t x) { v.push_back ((uint8_t) (x >> 8)); v.push_back ((uint8_t) x); }
void be32 (std::vector<uint8_t>& v, uint32_t x) { for (int s = 24; s >= 0; s -= 8) v.push_back ((uint8_t) (x >> s)); }
void vlq (std::vector<uint8_t>& v, uint32_t x)
{
    uint8_t buf[5];
    int n = 0;
    buf[n++] = (uint8_t) (x & 0x7F);
    while ((x >>= 7) != 0) buf[n++] = (uint8_t) ((x & 0x7F) | 0x80);
    while (n > 0) v.push_back (buf[--n]);
}

struct Ev { int64_t tick; int order; std::vector<uint8_t> data; };

// key signature: sharps/flats count of the parent major key, minor flag
void keySig (int root, ScaleType s, int8_t& sf, uint8_t& mi)
{
    static const int8_t majorSf[12] = { 0, -5, 2, -3, 4, -1, 6, 1, -4, 3, -2, 5 }; // C Db D Eb E F F# G Ab A Bb B
    int parent = root;
    switch (s)
    {
        case ScaleType::NaturalMinor: case ScaleType::HarmonicMinor: case ScaleType::MinorPentatonic: parent = root + 3; break;
        case ScaleType::Dorian: parent = root + 10; break;
        case ScaleType::Phrygian: parent = root + 8; break;
        case ScaleType::Mixolydian: parent = root + 5; break;
        case ScaleType::Major: break;
    }
    sf = majorSf[((parent % 12) + 12) % 12];
    mi = (s == ScaleType::Major || s == ScaleType::Mixolydian) ? 0 : 1;
}
} // namespace

std::vector<uint8_t> writeSmf (const Pattern& p, const ExportOptions& o)
{
    const uint8_t ch = (uint8_t) std::clamp (o.channel, 0, 15);
    std::vector<Ev> evs;
    // meta events at tick 0
    {
        std::vector<uint8_t> d { 0xFF, 0x03 };
        std::string name = o.trackName.substr (0, 120);
        vlq (d, (uint32_t) name.size());
        d.insert (d.end(), name.begin(), name.end());
        evs.push_back ({ 0, -4, d });
    }
    evs.push_back ({ 0, -3, { 0xFF, 0x58, 0x04, 0x04, 0x02, 0x18, 0x08 } }); // 4/4, 24 clocks, 8 32nds
    {
        const double bpm = std::clamp (o.tempoBpm, 20.0, 999.0);
        const uint32_t mpq = (uint32_t) std::lround (60000000.0 / bpm);
        evs.push_back ({ 0, -2, { 0xFF, 0x51, 0x03, (uint8_t) (mpq >> 16), (uint8_t) (mpq >> 8), (uint8_t) mpq } });
    }
    if (o.writeKeySignature)
    {
        int8_t sf; uint8_t mi;
        keySig (p.settings.root, p.settings.scale, sf, mi);
        evs.push_back ({ 0, -1, { 0xFF, 0x59, 0x02, (uint8_t) sf, mi } });
    }
    for (const auto& n : p.notes)
    {
        evs.push_back ({ n.startTick, 1, { (uint8_t) (0x90 | ch), (uint8_t) (n.midiNote & 0x7F), (uint8_t) std::clamp<int> (n.velocity, 1, 127) } });
        evs.push_back ({ n.endTick(), 0, { (uint8_t) (0x80 | ch), (uint8_t) (n.midiNote & 0x7F), 64 } });
    }
    std::stable_sort (evs.begin(), evs.end(), [] (const Ev& a, const Ev& b) {
        if (a.tick != b.tick) return a.tick < b.tick;
        return a.order < b.order;
    });

    std::vector<uint8_t> trk;
    int64_t last = 0;
    for (const auto& e : evs)
    {
        vlq (trk, (uint32_t) (e.tick - last));
        trk.insert (trk.end(), e.data.begin(), e.data.end());
        last = e.tick;
    }
    const int64_t endTick = std::max<int64_t> (p.lengthTicks(), last);
    vlq (trk, (uint32_t) (endTick - last));
    trk.insert (trk.end(), { 0xFF, 0x2F, 0x00 });

    std::vector<uint8_t> out { 'M', 'T', 'h', 'd' };
    be32 (out, 6);
    be16 (out, 0);    // format 0
    be16 (out, 1);    // one track
    be16 (out, (uint32_t) kPPQ);
    out.insert (out.end(), { 'M', 'T', 'r', 'k' });
    be32 (out, (uint32_t) trk.size());
    out.insert (out.end(), trk.begin(), trk.end());
    return out;
}

ParsedMidi readSmf (const std::vector<uint8_t>& b)
{
    ParsedMidi r;
    size_t pos = 0;
    auto need = [&] (size_t n) { return pos + n <= b.size(); };
    auto rd16 = [&] { uint32_t v = (uint32_t) (b[pos] << 8 | b[pos + 1]); pos += 2; return v; };
    auto rd32 = [&] { uint32_t v = (uint32_t) b[pos] << 24 | (uint32_t) b[pos + 1] << 16 | (uint32_t) b[pos + 2] << 8 | b[pos + 3]; pos += 4; return v; };
    auto fail = [&] (const char* m) { r.error = m; return r; };
    if (! need (14) || b[0] != 'M' || b[1] != 'T' || b[2] != 'h' || b[3] != 'd') return fail ("bad header");
    pos = 4;
    if (rd32() != 6) return fail ("bad header length");
    r.format = (int) rd16();
    r.numTracks = (int) rd16();
    r.ppq = (int) rd16();
    if (r.ppq & 0x8000) return fail ("SMPTE timing unsupported");
    struct Open { int64_t tick; uint8_t vel; };
    std::vector<std::vector<Open>> open (128);
    for (int t = 0; t < r.numTracks; ++t)
    {
        if (! need (8) || b[pos] != 'M' || b[pos + 1] != 'T' || b[pos + 2] != 'r' || b[pos + 3] != 'k') return fail ("bad track header");
        pos += 4;
        const uint32_t len = rd32();
        if (! need (len)) return fail ("truncated track");
        const size_t end = pos + len;
        int64_t tick = 0;
        uint8_t status = 0;
        bool sawEot = false;
        while (pos < end)
        {
            uint32_t dv = 0;
            for (int i = 0; i < 4; ++i)
            {
                if (pos >= end) return fail ("truncated delta");
                const uint8_t c = b[pos++];
                dv = (dv << 7) | (c & 0x7F);
                if (! (c & 0x80)) break;
                if (i == 3) return fail ("bad vlq");
            }
            tick += dv;
            if (pos >= end) return fail ("truncated event");
            uint8_t st = b[pos];
            if (st & 0x80) { ++pos; status = st; } else st = status;
            if (st == 0xFF)
            {
                if (pos + 2 > end) return fail ("truncated meta");
                const uint8_t type = b[pos++];
                const uint8_t ml = b[pos++];
                if (pos + ml > end) return fail ("truncated meta data");
                if (type == 0x51 && ml == 3) r.tempoBpm = 60000000.0 / (double) ((uint32_t) b[pos] << 16 | (uint32_t) b[pos + 1] << 8 | b[pos + 2]);
                if (type == 0x58 && ml >= 2) { r.timeSigNum = b[pos]; r.timeSigDen = 1 << b[pos + 1]; }
                if (type == 0x2F) { r.endOfTrackTick = tick; sawEot = true; }
                pos += ml;
                continue;
            }
            const uint8_t kind = st & 0xF0;
            if (kind == 0x90 || kind == 0x80)
            {
                if (pos + 2 > end) return fail ("truncated note");
                const uint8_t note = b[pos], vel = b[pos + 1];
                pos += 2;
                if (kind == 0x90 && vel > 0) open[note].push_back ({ tick, vel });
                else
                {
                    if (open[note].empty()) return fail ("note-off without note-on");
                    const auto o = open[note].front();
                    open[note].erase (open[note].begin());
                    NoteEvent e;
                    e.startTick = o.tick;
                    e.durationTicks = tick - o.tick;
                    e.midiNote = note;
                    e.velocity = o.vel;
                    r.notes.push_back (e);
                }
            }
            else if (kind == 0xC0 || kind == 0xD0) pos += 1;
            else if (kind == 0xA0 || kind == 0xB0 || kind == 0xE0) pos += 2;
            else return fail ("unsupported event");
        }
        if (! sawEot) return fail ("missing end of track");
    }
    for (auto& v : open)
        if (! v.empty()) return fail ("stuck note (note-on without note-off)");
    std::sort (r.notes.begin(), r.notes.end(), [] (const NoteEvent& a, const NoteEvent& c) { return a.startTick < c.startTick; });
    return r;
}

std::string suggestedFileName (const Pattern& p)
{
    static const char* scaleShort[] = { "Minor", "Major", "Dorian", "Phrygian", "Mixo", "HarmMinor", "MinPenta" };
    std::string root = theory::rootName (p.settings.root);
    for (auto& c : root) if (c == '#') c = 's';
    std::string name = "GFB_" + root + "_" + scaleShort[(int) p.settings.scale % kNumScales] + "_" + std::string (toString (p.settings.family)) + "_"
                     + std::string (toString (p.resolvedFeel)) + "_" + std::to_string (p.bars) + "bar_S" + seedToString (p.settings.styleSeed)
                     + "_V" + seedToString (p.settings.varSeed) + ".mid";
    for (auto& c : name)
        if (! ((c >= 'A' && c <= 'Z') || (c >= 'a' && c <= 'z') || (c >= '0' && c <= '9') || c == '_' || c == '.' || c == '-')) c = '_';
    return name;
}
} // namespace gfb::midi

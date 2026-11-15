#include <algorithm>
#include "core/Serialization.h"

#include <charconv>

namespace gfb::serial
{
namespace
{
template <typename T>
std::optional<T> parseNum (std::string_view s)
{
    T v {};
    auto [ptr, ec] = std::from_chars (s.data(), s.data() + s.size(), v);
    if (ec != std::errc() || ptr != s.data() + s.size()) return std::nullopt;
    return v;
}

std::vector<std::string_view> split (std::string_view s, char sep)
{
    std::vector<std::string_view> out;
    size_t start = 0;
    while (start <= s.size())
    {
        const size_t p = s.find (sep, start);
        if (p == std::string_view::npos) { if (start < s.size()) out.push_back (s.substr (start)); break; }
        out.push_back (s.substr (start, p - start));
        start = p + 1;
    }
    return out;
}
} // namespace

std::string notesToString (const std::vector<NoteEvent>& notes)
{
    std::string out;
    out.reserve (notes.size() * 20);
    for (const auto& n : notes)
    {
        out += std::to_string (n.startTick) + ':' + std::to_string (n.durationTicks) + ':' + std::to_string ((int) n.midiNote) + ':'
             + std::to_string ((int) n.velocity) + ':' + std::to_string ((n.accent ? 1 : 0) | (n.slideToNext ? 2 : 0)) + ';';
    }
    return out;
}

std::optional<std::vector<NoteEvent>> notesFromString (std::string_view text)
{
    std::vector<NoteEvent> out;
    for (auto tok : split (text, ';'))
    {
        if (tok.empty()) continue;
        auto f = split (tok, ':');
        if (f.size() != 5) return std::nullopt;
        auto st = parseNum<int64_t> (f[0]);
        auto du = parseNum<int64_t> (f[1]);
        auto nt = parseNum<int> (f[2]);
        auto ve = parseNum<int> (f[3]);
        auto fl = parseNum<int> (f[4]);
        if (! st || ! du || ! nt || ! ve || ! fl) return std::nullopt;
        if (*st < 0 || *st > (int64_t) 8 * kTicksPerBar || *du <= 0 || *du > (int64_t) 8 * kTicksPerBar) return std::nullopt;
        if (*nt < 0 || *nt > 127 || *ve < 1 || *ve > 127 || *fl < 0 || *fl > 3) return std::nullopt;
        out.push_back ({ *st, *du, (uint8_t) *nt, (uint8_t) *ve, (*fl & 1) != 0, (*fl & 2) != 0 });
        if (out.size() > (size_t) kMaxNotesPerPattern) return std::nullopt;
    }
    return out;
}

std::map<std::string, std::string> settingsToMap (const GeneratorSettings& s)
{
    return {
        { "root", std::to_string (s.root) },
        { "scale", std::string (toString (s.scale)) },
        { "family", std::string (toString (s.family)) },
        { "feel", std::string (toString (s.feel)) },
        { "bars", std::to_string (s.bars) },
        { "octave", std::to_string (s.octave) },
        { "style", seedToString (s.styleSeed) },
        { "var", seedToString (s.varSeed) },
        { "kick", std::string (toString (s.kickContext)) },
        { "swing", std::to_string (s.swing) },
        { "chrom", s.chromatic ? "1" : "0" },
        { "cx", std::to_string (s.complexity) },
    };
}

GeneratorSettings settingsFromMap (const std::map<std::string, std::string>& kv, const GeneratorSettings& d)
{
    GeneratorSettings s = d;
    auto get = [&] (const char* k) -> std::optional<std::string_view> {
        auto it = kv.find (k);
        if (it == kv.end()) return std::nullopt;
        return std::string_view (it->second);
    };
    if (auto v = get ("root")) if (auto n = parseNum<int> (*v); n && *n >= 0 && *n < 12) s.root = *n;
    if (auto v = get ("scale")) if (auto e = scaleFromString (*v)) s.scale = *e;
    if (auto v = get ("family")) if (auto e = familyFromString (*v)) s.family = *e;
    if (auto v = get ("feel")) if (auto e = feelFromString (*v)) s.feel = *e;
    if (auto v = get ("bars")) if (auto n = parseNum<int> (*v); n && isValidBars (*n)) s.bars = *n;
    if (auto v = get ("octave")) if (auto n = parseNum<int> (*v); n && *n >= kMinOctave && *n <= kMaxOctave) s.octave = *n;
    if (auto v = get ("style")) if (auto n = parseSeed (*v)) s.styleSeed = *n;
    if (auto v = get ("var")) if (auto n = parseSeed (*v)) s.varSeed = *n;
    if (auto v = get ("kick")) if (auto e = kickContextFromString (*v)) s.kickContext = *e;
    if (auto v = get ("swing")) if (auto n = parseNum<int> (*v); n && (*n == kAutoSwing || (*n >= kMinSwing && *n <= kMaxSwing))) s.swing = *n;
    if (auto v = get ("chrom")) if (*v == "0" || *v == "1") s.chromatic = *v == "1";
    if (auto v = get ("cx")) if (auto n = parseNum<int> (*v); n && *n >= 0 && *n <= 100) s.complexity = *n;
    return sanitise (s);
}

std::map<std::string, std::string> locksToMap (const LockSet& l)
{
    auto b = [] (bool x) { return std::string (x ? "1" : "0"); };
    return { { "root", b (l.root) }, { "scale", b (l.scale) }, { "family", b (l.family) }, { "feel", b (l.feel) },
             { "bars", b (l.bars) }, { "octave", b (l.octave) }, { "style", b (l.styleSeed) }, { "var", b (l.varSeed) } };
}

LockSet locksFromMap (const std::map<std::string, std::string>& kv, const LockSet& d)
{
    LockSet l = d;
    auto rd = [&] (const char* k, bool& dst) {
        auto it = kv.find (k);
        if (it != kv.end() && (it->second == "0" || it->second == "1")) dst = it->second == "1";
    };
    rd ("root", l.root); rd ("scale", l.scale); rd ("family", l.family); rd ("feel", l.feel);
    rd ("bars", l.bars); rd ("octave", l.octave); rd ("style", l.styleSeed); rd ("var", l.varSeed);
    return l;
}

std::string mapToString (const std::map<std::string, std::string>& kv)
{
    std::string out;
    for (auto& [k, v] : kv) out += k + '=' + v + ';';
    return out;
}

std::map<std::string, std::string> mapFromString (std::string_view s)
{
    std::map<std::string, std::string> kv;
    for (auto tok : split (s, ';'))
    {
        const auto eq = tok.find ('=');
        if (eq == std::string_view::npos || eq == 0) continue;
        kv[std::string (tok.substr (0, eq))] = std::string (tok.substr (eq + 1));
    }
    return kv;
}

std::string patternToString (const Pattern& p)
{
    auto kv = settingsToMap (p.settings);
    kv["v"] = std::to_string (p.algorithmVersion);
    kv["pbars"] = std::to_string (p.bars);
    kv["rfeel"] = std::string (toString (p.resolvedFeel));
    kv["arch"] = std::to_string (p.archetypeId);
    kv["ans"] = std::to_string (p.answerArchetypeId);
    kv["swingp"] = std::to_string (p.swingPercent);
    std::string plan = p.phrasePlan;
    for (auto& c : plan) if (c == ';' || c == '=' || c == '|') c = ' ';
    kv["plan"] = plan;
    return mapToString (kv) + '|' + notesToString (p.notes);
}

std::optional<Pattern> patternFromString (std::string_view s)
{
    const auto bar = s.find ('|');
    if (bar == std::string_view::npos) return std::nullopt;
    const auto kv = mapFromString (s.substr (0, bar));
    auto notes = notesFromString (s.substr (bar + 1));
    if (! notes) return std::nullopt;
    Pattern p;
    p.settings = settingsFromMap (kv);
    auto num = [&] (const char* k, int def) {
        auto it = kv.find (k);
        if (it == kv.end()) return def;
        auto n = parseNum<int> (it->second);
        return n ? *n : def;
    };
    p.algorithmVersion = (uint32_t) std::max (1, num ("v", (int) kAlgorithmVersion));
    p.bars = num ("pbars", p.settings.bars);
    if (auto it = kv.find ("rfeel"); it != kv.end())
        if (auto f = feelFromString (it->second)) p.resolvedFeel = *f;
    p.archetypeId = num ("arch", -1);
    p.answerArchetypeId = num ("ans", -1);
    p.swingPercent = num ("swingp", 50);
    if (auto it = kv.find ("plan"); it != kv.end()) p.phrasePlan = it->second;
    p.notes = std::move (*notes);
    if (! validatePattern (p).empty()) return std::nullopt;
    p.hash = computePatternHash (p);
    return p;
}
} // namespace gfb::serial

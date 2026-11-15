#include "core/TasteProfile.h"

#include <algorithm>
#include <charconv>
#include <sstream>

#include "core/PatternMetrics.h"
#include "core/RhythmGrammar.h"

namespace gfb::taste
{
namespace
{
constexpr int kUnit = 100;   // each rated pattern contributes 100 "units" per feature family

struct Features
{
    std::array<int, 16> steps {};
    std::array<int, 25> intervals {};
    std::array<int, 6> dur {};
    int densityBucket = 0;
};

Features extract (const Pattern& p)
{
    Features f;
    std::array<int, 16> s {};
    std::array<int, 25> iv {};
    std::array<int, 6> du {};
    int ns = 0, ni = 0;
    for (size_t i = 0; i < p.notes.size(); ++i)
    {
        const auto& n = p.notes[i];
        s[(size_t) (tickToStep (n.startTick) % kStepsPerBar)]++;
        const int64_t d = n.durationTicks;
        du[d <= 120 ? 0 : d <= 240 ? 1 : d <= 360 ? 2 : d <= 480 ? 3 : d <= 960 ? 4 : 5]++;
        ++ns;
        if (i > 0)
        {
            iv[(size_t) (std::clamp ((int) n.midiNote - (int) p.notes[i - 1].midiNote, -12, 12) + 12)]++;
            ++ni;
        }
    }
    // normalise so every pattern carries the same total weight (integer arithmetic)
    for (size_t i = 0; i < 16; ++i) f.steps[i] = ns ? s[i] * kUnit / ns : 0;
    for (size_t i = 0; i < 25; ++i) f.intervals[i] = ni ? iv[i] * kUnit / ni : 0;
    for (size_t i = 0; i < 6; ++i) f.dur[i] = ns ? du[i] * kUnit / ns : 0;
    const int perBar = p.bars > 0 ? (int) p.notes.size() / p.bars : 0;
    f.densityBucket = perBar <= 3 ? 0 : perBar <= 6 ? 1 : perBar <= 9 ? 2 : 3;
    return f;
}

template <size_t N>
void addTo (std::array<int, N>& dst, const std::array<int, N>& src)
{
    for (size_t i = 0; i < N; ++i) dst[i] += src[i];
}

template <size_t N>
int total (const std::array<int, N>& a)
{
    int t = 0;
    for (int x : a) t += x;
    return t;
}

template <size_t N>
void writeArr (std::ostringstream& o, const char* key, const std::array<int, N>& a)
{
    o << key << '=';
    for (size_t i = 0; i < N; ++i) o << (i ? "," : "") << a[i];
    o << ';';
}

template <size_t N>
bool readArr (const std::string& v, std::array<int, N>& a)
{
    size_t pos = 0;
    for (size_t i = 0; i < N; ++i)
    {
        const size_t e = v.find (',', pos);
        const std::string tok = v.substr (pos, e == std::string::npos ? std::string::npos : e - pos);
        int x = 0;
        auto [ptr, ec] = std::from_chars (tok.data(), tok.data() + tok.size(), x);
        if (ec != std::errc() || ptr != tok.data() + tok.size() || x < 0) return false;
        a[i] = x;
        if (e == std::string::npos) return i + 1 == N;
        pos = e + 1;
    }
    return true;
}
} // namespace

std::optional<std::string> cellFromPattern (const Pattern& p)
{
    std::string g (kStepsPerBar, '.');
    int onsets = 0;
    for (size_t i = 0; i < p.notes.size(); ++i)
    {
        const auto& n = p.notes[i];
        const int step = tickToStep (n.startTick);
        if (step >= kStepsPerBar) break;
        const int len = std::max (1, (int) ((n.durationTicks + kTicksPerSixteenth / 2) / kTicksPerSixteenth));
        g[(size_t) step] = step == 0 ? 'A' : (n.accent ? 'X' : (n.velocity < 70 ? 'g' : 'x'));
        ++onsets;
        for (int k = 1; k < len && step + k < kStepsPerBar; ++k)
        {
            if (g[(size_t) (step + k)] != '.') break;
            g[(size_t) (step + k)] = '-';
        }
        if (n.slideToNext)
        {
            int last = step;
            while (last + 1 < kStepsPerBar && g[(size_t) (last + 1)] == '-') ++last;
            if (last > step && last + 1 < kStepsPerBar && i + 1 < p.notes.size() && tickToStep (p.notes[i + 1].startTick) == last + 1)
                g[(size_t) last] = '~';
        }
    }
    // later onsets may have overwritten holds: re-validate through the grammar parser
    const auto cell = rhythm::parseGrid (g);
    if (onsets < 2 || rhythm::onsetCount (cell) < 2) return std::nullopt;
    return rhythm::toGrid (cell);
}

void addFeedback (TasteProfile& t, const Pattern& p, bool liked)
{
    if (p.notes.empty()) return;
    const Features f = extract (p);
    if (liked)
    {
        ++t.likes;
        addTo (t.likeSteps, f.steps);
        addTo (t.likeIntervals, f.intervals);
        addTo (t.likeDur, f.dur);
        t.likeDensity[(size_t) f.densityBucket] += kUnit;
        t.archetypes[p.archetypeId].first++;
        if (auto g = cellFromPattern (p))
        {
            auto it = std::find_if (t.cells.begin(), t.cells.end(), [&] (const LearnedCell& c) { return c.grid == *g && c.family == p.settings.family; });
            if (it != t.cells.end()) it->weight = std::min (it->weight + 1, 9);
            else
            {
                t.cells.push_back ({ *g, p.resolvedFeel, p.settings.family, 1 });
                if ((int) t.cells.size() > TasteProfile::kMaxCells) t.cells.erase (t.cells.begin()); // oldest first
            }
        }
    }
    else
    {
        ++t.dislikes;
        addTo (t.dislikeSteps, f.steps);
        addTo (t.dislikeIntervals, f.intervals);
        addTo (t.dislikeDur, f.dur);
        t.dislikeDensity[(size_t) f.densityBucket] += kUnit;
        t.archetypes[p.archetypeId].second++;
        // a disliked cell that had been learned loses weight / is forgotten
        if (auto g = cellFromPattern (p))
            for (auto it = t.cells.begin(); it != t.cells.end(); ++it)
                if (it->grid == *g && it->family == p.settings.family)
                {
                    if (--it->weight <= 0) t.cells.erase (it);
                    break;
                }
    }
    ++t.revision;
}

double confidence (const TasteProfile& t) noexcept
{
    return std::min (1.0, (double) (t.likes + t.dislikes) / 8.0);
}

int archetypePermille (const TasteProfile& t, int id) noexcept
{
    auto it = t.archetypes.find (id);
    if (it == t.archetypes.end()) return 1000;
    const int l = it->second.first, d = it->second.second;
    return std::clamp ((2 + l) * 1000 / (2 + d), 150, 4000);
}

/** 1 - L1/2 between the pattern's distribution and an accumulated (unnormalised) one. */
template <size_t N>
double similarity (const std::array<int, N>& x, const std::array<int, N>& acc)
{
    const int tx = total (x), ta = total (acc);
    if (tx <= 0 || ta <= 0) return 0.5;
    double l1 = 0;
    for (size_t i = 0; i < N; ++i) l1 += std::abs ((double) x[i] / tx - (double) acc[i] / ta);
    return 1.0 - 0.5 * l1;
}

double score (const Pattern& p, const TasteProfile& t)
{
    if (t.empty() || p.notes.empty()) return 0.5;
    const Features f = extract (p);
    std::array<int, 4> dens {};
    dens[(size_t) f.densityBucket] = kUnit;
    // weighted similarity to the liked centroid and to the disliked centroid
    auto sim = [&] (const auto& steps, const auto& ivs, const auto& durs, const auto& dn) {
        return (3.0 * similarity (f.steps, steps) + 1.0 * similarity (f.intervals, ivs) + 1.5 * similarity (f.dur, durs)
                + 1.0 * similarity (dens, dn)) / 6.5;
    };
    const double like = t.likes > 0 ? sim (t.likeSteps, t.likeIntervals, t.likeDur, t.likeDensity) : 0.5;
    const double dislike = t.dislikes > 0 ? sim (t.dislikeSteps, t.dislikeIntervals, t.dislikeDur, t.dislikeDensity) : 0.5;
    return std::clamp (0.5 + 1.5 * (like - dislike), 0.0, 1.0);
}

std::string toString (const TasteProfile& t)
{
    std::ostringstream o;
    o << "v=1;rev=" << t.revision << ";likes=" << t.likes << ";dislikes=" << t.dislikes << ';';
    writeArr (o, "ls", t.likeSteps); writeArr (o, "ds", t.dislikeSteps);
    writeArr (o, "li", t.likeIntervals); writeArr (o, "di", t.dislikeIntervals);
    writeArr (o, "ld", t.likeDur); writeArr (o, "dd", t.dislikeDur);
    writeArr (o, "ln", t.likeDensity); writeArr (o, "dn", t.dislikeDensity);
    o << "arch=";
    bool first = true;
    for (const auto& [id, c] : t.archetypes) { o << (first ? "" : ",") << id << ':' << c.first << ':' << c.second; first = false; }
    o << ";cells=";
    first = true;
    for (const auto& c : t.cells)
    {
        o << (first ? "" : ",") << c.grid << ':' << (int) c.feel << ':' << (int) c.family << ':' << c.weight;
        first = false;
    }
    o << ';';
    return o.str();
}

std::optional<TasteProfile> fromString (const std::string& s)
{
    std::map<std::string, std::string> kv;
    size_t pos = 0;
    while (pos < s.size())
    {
        const size_t e = s.find (';', pos);
        const std::string tok = s.substr (pos, e == std::string::npos ? std::string::npos : e - pos);
        const size_t eq = tok.find ('=');
        if (eq != std::string::npos) kv[tok.substr (0, eq)] = tok.substr (eq + 1);
        if (e == std::string::npos) break;
        pos = e + 1;
    }
    if (kv["v"] != "1") return std::nullopt;
    TasteProfile t;
    auto num = [&] (const char* k, auto& dst) {
        const auto& v = kv[k];
        auto [p, ec] = std::from_chars (v.data(), v.data() + v.size(), dst);
        return ec == std::errc() && p == v.data() + v.size();
    };
    if (! num ("rev", t.revision) || ! num ("likes", t.likes) || ! num ("dislikes", t.dislikes)) return std::nullopt;
    if (! readArr (kv["ls"], t.likeSteps) || ! readArr (kv["ds"], t.dislikeSteps) || ! readArr (kv["li"], t.likeIntervals)
        || ! readArr (kv["di"], t.dislikeIntervals) || ! readArr (kv["ld"], t.likeDur) || ! readArr (kv["dd"], t.dislikeDur)
        || ! readArr (kv["ln"], t.likeDensity) || ! readArr (kv["dn"], t.dislikeDensity))
        return std::nullopt;
    std::istringstream arch (kv["arch"]);
    std::string item;
    while (std::getline (arch, item, ','))
    {
        int id, l, d;
        if (std::sscanf (item.c_str(), "%d:%d:%d", &id, &l, &d) == 3 && l >= 0 && d >= 0) t.archetypes[id] = { l, d };
    }
    std::istringstream cells (kv["cells"]);
    while (std::getline (cells, item, ','))
    {
        const auto parts = [&] {
            std::vector<std::string> v;
            std::istringstream is (item);
            std::string x;
            while (std::getline (is, x, ':')) v.push_back (x);
            return v;
        }();
        if (parts.size() != 4 || parts[0].size() != (size_t) kStepsPerBar) continue;
        const int feel = std::atoi (parts[1].c_str()), fam = std::atoi (parts[2].c_str()), w = std::atoi (parts[3].c_str());
        if (feel < 1 || feel >= kNumFeels || fam < 0 || fam > 1 || w < 1) continue;
        const auto cell = rhythm::parseGrid (parts[0]);
        if (rhythm::onsetCount (cell) < 2) continue;
        t.cells.push_back ({ rhythm::toGrid (cell), (Feel) feel, (Family) fam, std::min (w, 9) });
        if ((int) t.cells.size() >= TasteProfile::kMaxCells) break;
    }
    return t;
}
} // namespace gfb::taste

#include "core/PatternEvaluator.h"

#include <algorithm>
#include <sstream>

#include "core/ReferencePriors.h"

namespace gfb
{
namespace
{
double inBand (double v, double lo, double hi, double falloff) noexcept
{
    if (v >= lo && v <= hi) return 1.0;
    const double d = v < lo ? lo - v : v - hi;
    return std::max (0.0, 1.0 - d / falloff);
}

struct Weights
{
    double density, sync, offbeat, kick, motif, range, intervals, anchor, ending, contrast, accent, repetition, chain, rest, groove, ivPrior, legato;
};

/** Mean user-dataset likelihood of the onset positions, relative to the most
    likely step (0..1). Integer tables + plain arithmetic (deterministic). */
double groovePrior (const Pattern& p) noexcept
{
    const auto& t = priors::kUserSteps;
    const int mx = *std::max_element (t.begin(), t.end());
    if (p.notes.empty() || mx <= 0) return 0.0;
    double s = 0;
    for (const auto& n : p.notes) s += t[(size_t) (tickToStep (n.startTick) % kStepsPerBar)];
    return s / ((double) p.notes.size() * mx);
}

/** Mean user-dataset probability of the melodic intervals, relative to the most common non-repeat interval. */
double intervalPrior (const Pattern& p) noexcept
{
    const auto& t = priors::kUserIntervals;
    int mx = 0;
    for (size_t i = 0; i < t.size(); ++i) if (i != 12) mx = std::max (mx, t[i]);
    if (p.notes.size() < 2 || mx <= 0) return 1.0;
    double s = 0;
    for (size_t i = 1; i < p.notes.size(); ++i)
    {
        const int iv = std::clamp ((int) p.notes[i].midiNote - (int) p.notes[i - 1].midiNote, -12, 12);
        s += std::min (1.0, (double) t[(size_t) (iv + 12)] / mx);
    }
    return s / (double) (p.notes.size() - 1);
}

Weights weightsFor (Feel f, Family fam) noexcept
{
    Weights w { 3, 2, 1.5, 1.5, 2, 1.5, 2, 2, 1.5, 1.5, 1, 1.5, 2, 1, 3, 2, 2 };
    switch (f)
    {
        case Feel::Sparse:     w.rest = 0.3; w.density = 3.5; w.contrast = 1; w.groove = 2; break;
        case Feel::Busy:       w.chain = 3; w.density = 3; w.intervals = 2.5; w.legato = 0.5; break;
        case Feel::Rolling:    w.offbeat = 2.5; w.chain = 2.5; break;
        case Feel::Syncopated: w.sync = 3.5; w.kick = 2; break;
        case Feel::Stabby:     w.contrast = 2.5; w.chain = 3; w.legato = 0; w.groove = 2; break;
        default: break;
    }
    if (fam == Family::Sub) { w.range *= 1.5; w.anchor *= 1.3; w.intervals *= 1.2; }
    else { w.accent *= 2; w.intervals *= 1.3; }
    return w;
}
} // namespace

int maxChainForFeel (Feel f) noexcept
{
    switch (f)
    {
        case Feel::Sparse: return 2;
        case Feel::Stabby: return 2;
        case Feel::Balanced: return 3;
        case Feel::Syncopated: return 3;
        case Feel::Rolling: return 4;
        case Feel::Busy: return 6;
        case Feel::Random: return 4;
    }
    return 4;
}

std::string ScoreResult::describe() const
{
    std::ostringstream o;
    o.precision (3);
    o << (rejected ? "REJECTED" : "score=") ;
    if (! rejected) o << total;
    for (auto& r : reasons) o << " [" << r << "]";
    for (auto& c : components) o << ' ' << c.name << '=' << c.value;
    return o.str();
}

ScoreResult evaluatePattern (const Pattern& p, const EvalContext& ctx)
{
    ScoreResult r;
    const auto v = validatePattern (p);
    if (! v.empty()) { r.rejected = true; r.reasons.push_back ("invalid: " + v); return r; }
    r.metrics = computeMetrics (p);
    const auto& m = r.metrics;
    const auto& fp = feelProfile (ctx.feel);

    if (m.noteCount == 0) r.reasons.push_back ("silent");
    if (m.outOfScale > 0) r.reasons.push_back ("out-of-scale notes");
    if (m.emptyBars > 0) r.reasons.push_back ("empty bar");
    if (m.maxChain >= 12) r.reasons.push_back ("machine-gun 16ths");
    if (! r.reasons.empty()) { r.rejected = true; return r; }

    const auto w = weightsFor (ctx.feel, ctx.family);
    auto add = [&] (const char* name, double val, double weight) { r.components.push_back ({ name, std::clamp (val, 0.0, 1.0), weight }); };

    // 1 density: in-range and close to the feel target
    const double span = std::max (1, fp.maxOnsets - fp.minOnsets);
    double dens = inBand (m.onsetsPerBar, fp.minOnsets, fp.maxOnsets, span * 0.5);
    dens *= 1.0 - 0.5 * std::min (1.0, std::abs (m.onsetsPerBar - fp.targetOnsets) / span);
    add ("density", dens, w.density);
    // 2 syncopation & 3 offbeat placement
    add ("syncopation", inBand (m.syncopation * 1000, fp.syncLo, fp.syncHi, 400), w.sync);
    add ("offbeat", inBand (m.offbeatRatio * 1000, fp.offbeatLo, fp.offbeatHi, 400), w.offbeat);
    // 4 implied kick interaction (a prior, never a hard ban)
    double kick = 1.0;
    if (ctx.kick == KickContext::FourOnFloor) kick = 1.0 - std::max (0.0, m.kickCollision - 0.25) * 2.0; // user data: ~18 %
    else if (ctx.kick == KickContext::Broken) kick = inBand (m.kickCollision, 0.1, 0.45, 0.4);
    add ("kick", kick, w.kick);
    // 5 motif recurrence (structural repetition, but not a frozen loop)
    if (ctx.scoreMotif && p.bars > 1)
    {
        // user dataset: strongly repetitive phrases (A A A A' / A B A B: ~44 % distinct bars, similarity ~0.88)
        double motif = inBand (m.barSimilarity, 0.6, 1.0, 0.4);
        if (p.bars >= 4)
            motif *= inBand ((double) m.distinctBars / p.bars, 0.25, 0.6, 0.5);
        add ("motif", motif, w.motif);
    }
    // 6 register span
    const double maxSpan = ctx.family == Family::Sub ? 12 : 15;
    add ("range", inBand (m.pitchRange, 0, maxSpan, 8), w.range);
    // 7 interval character
    double iv;
    if (ctx.family == Family::Sub)
        iv = 1.0 - m.leapRatio * 2.5 - std::max (0.0, m.stepRatio - 0.5);
    else
        iv = 0.6 + 0.5 * std::min (1.0, m.stepRatio * 2.0) - m.leapRatio * 2.0;
    if (m.distinctPitches < 2 && m.noteCount > 3) iv -= ctx.family == Family::Acid ? 0.5 : 0.2;
    if (m.distinctPitches > 7) iv -= 0.3;
    add ("intervals", iv, w.intervals);
    // 8 root anchoring
    add ("anchor", (m.firstIsRoot ? 0.5 : 0.1) + 0.5 * std::min (1.0, m.rootRatio * 2.5), w.anchor);
    // 9 ending
    add ("ending", m.endsResolved ? 1.0 : 0.3, w.ending);
    // 10 duration contrast
    double contrast;
    if (ctx.feel == Feel::Stabby) contrast = m.shortRatio;
    else if (ctx.feel == Feel::Busy) contrast = 0.6 + 0.4 * std::min (1.0, (m.longRatio + 0.1) * 3);
    else contrast = (m.shortRatio > 0.05 || m.longRatio > 0.05) ? (m.longRatio > 0.05 ? 1.0 : 0.7) : 0.5;
    add ("contrast", contrast, w.contrast);
    // 11 accent dynamics
    add ("accent", inBand (m.accentRatio, 0.15, 0.6, 0.3), w.accent);
    // 12 repetition control
    const int maxRun = ctx.family == Family::Acid ? 4 : 6;
    double rep = m.maxSamePitchRun <= maxRun ? 1.0 : std::max (0.0, 1.0 - 0.2 * (m.maxSamePitchRun - maxRun));
    if (m.repeatRatio > 0.8 && m.noteCount > 4) rep *= 0.6;
    add ("repetition", rep, w.repetition);
    // 13 anti machine-gun chain bound
    add ("chain", inBand (m.maxChain, 0, maxChainForFeel (ctx.feel), 3), w.chain);
    // 14 rests
    add ("rest", inBand (m.longestRestSteps, 0, fp.maxRestSteps, 6), w.rest);

    // 15-17: priors learned from the user's own dataset
    add ("groove_prior", ctx.kick == KickContext::Open ? 0.5 + 0.5 * groovePrior (p) : groovePrior (p), w.groove);
    add ("interval_prior", intervalPrior (p), w.ivPrior);
    add ("legato", inBand (m.soundingRatio, 0.55, 0.95, 0.35), w.legato);
    // 18: the user's own feedback (weight grows with the number of ratings)
    if (ctx.taste != nullptr && ! ctx.taste->empty())
        add ("taste", taste::score (p, *ctx.taste), 10.0 * taste::confidence (*ctx.taste));

    double num = 0, den = 0;
    for (auto& c : r.components) { num += c.value * c.weight; den += c.weight; }
    r.total = den > 0 ? num / den : 0;
    return r;
}
} // namespace gfb

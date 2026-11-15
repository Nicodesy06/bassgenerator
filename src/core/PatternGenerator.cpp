#include "core/PatternGenerator.h"

#include <algorithm>
#include <array>

#include "core/PhraseComposer.h"
#include "core/ScaleTheory.h"

namespace gfb
{
namespace
{
Feel resolveFeel (Feel f, Rng& rng)
{
    if (f != Feel::Random) return f;
    // profile-weighted blend, not uniform soup: Balanced/Rolling/Syncopated favoured
    const std::array<Feel, 6> feels { Feel::Sparse, Feel::Balanced, Feel::Busy, Feel::Rolling, Feel::Syncopated, Feel::Stabby };
    const std::array<int, 6> w { 10, 30, 10, 20, 20, 10 };
    return feels[(size_t) rng.pickWeighted (w.data(), 6)];
}

uint8_t familyBit (Family f) noexcept { return f == Family::Sub ? kFamSub : kFamAcid; }
uint8_t kickBit (KickContext k) noexcept { return k == KickContext::FourOnFloor ? kFitFour : (k == KickContext::Broken ? kFitBroken : kFitOpen); }

std::vector<const RhythmArchetype*> archetypePool (Feel feel, Family fam, std::vector<int>& weights, KickContext kick,
                                                   const TasteProfile* taste, const std::vector<RhythmArchetype>& learned,
                                                   const std::vector<int>& learnedWeights)
{
    std::vector<const RhythmArchetype*> pool;
    weights.clear();
    for (const auto& a : rhythmCorpus())
    {
        if ((a.families & familyBit (fam)) == 0) continue;
        int w = 0;
        if (a.feel == feel) w = 4;
        else if (a.alsoFeel == feel) w = 2;
        if (w == 0) continue;
        if (a.families != kFamBoth) w += 2;              // family-specific grooves preferred
        if (a.kickFit & kickBit (kick)) w *= 2;          // context prior
        if (taste != nullptr) w = std::max (1, w * taste::archetypePermille (*taste, a.id) / 1000);
        pool.push_back (&a);
        weights.push_back (w);
    }
    // cells learned from the user's LIKEs (same feel + family)
    for (size_t i = 0; i < learned.size(); ++i)
        if (learned[i].feel == feel && (learned[i].families & familyBit (fam)))
        {
            pool.push_back (&learned[i]);
            weights.push_back (6 + 3 * learnedWeights[i]); // liked cells: strong preference
        }
    return pool;
}

/** Bring a cell's onset count inside the feel bounds with bounded edits. */
void enforceDensity (RhythmCell& cell, Rng& rng, const FeelProfile& fp, Family fam)
{
    for (int guard = 0; guard < 24 && rhythm::onsetCount (cell) < fp.minOnsets; ++guard)
    {
        // relax the bound temporarily: AddOnset checks the max only
        FeelProfile loose = fp;
        loose.minOnsets = 0;
        if (! rhythm::mutate (cell, Mutation::AddOnset, rng, loose, fam))
            rhythm::mutate (cell, Mutation::SplitNote, rng, loose, fam);
    }
    for (int guard = 0; guard < 24 && rhythm::onsetCount (cell) > fp.maxOnsets; ++guard)
    {
        FeelProfile loose = fp;
        loose.maxOnsets = kStepsPerBar;
        if (! rhythm::mutate (cell, Mutation::RemoveOnset, rng, loose, fam))
            rhythm::mutate (cell, Mutation::MergeNotes, rng, loose, fam);
    }
}

void applyRandomMutations (RhythmCell& cell, int count, Rng& rng, const FeelProfile& fp, Family fam)
{
    int applied = 0;
    for (int attempt = 0; attempt < count * 6 && applied < count; ++attempt)
        if (rhythm::mutate (cell, rhythm::pickMutation (rng, fp, fam), rng, fp, fam)) ++applied;
}

struct Core
{
    RhythmCell cell;
    std::vector<NoteSlot> slots;
    std::vector<int> pitches;
};

BarRealisation toBar (const std::string& label, const Core& c) { return { label, c.cell, c.slots, c.pitches }; }

Core varyCore (const Core& base, Rng& rng, const FeelProfile& fp, Family fam, const MotifVocabulary& vocab,
               const PitchContext& ctx, int rhythmOps, int pitchOps, int rhythmPermille)
{
    Core out = base;
    if (rhythmOps > 0 && rng.chancePermille (rhythmPermille))
    {
        applyRandomMutations (out.cell, rhythmOps, rng, fp, fam);
        out.slots = rhythm::slots (out.cell);
        out.pitches = phrase::carryPitches (base.slots, base.pitches, out.slots, vocab, rng, ctx);
    }
    if (pitchOps > 0) pitch::varyPitches (out.slots, out.pitches, vocab, rng, ctx, pitchOps);
    return out;
}

const FillCell& pickFill (Rng& rng, Family fam, bool wantLong)
{
    std::vector<const FillCell*> c;
    std::vector<int> w;
    for (const auto& f : fillCorpus())
    {
        if ((f.families & familyBit (fam)) == 0) continue;
        const bool isLong = f.grid.size() == 8;
        c.push_back (&f);
        w.push_back (isLong == wantLong ? 4 : 1);
    }
    return *c[(size_t) rng.pickWeighted (w.data(), (int) w.size())];
}

Core makeFill (const Core& base, Rng& rng, const FillCell& fill, const PitchContext& ctx, const FeelProfile& fp, Family fam)
{
    Core out = base;
    rhythm::applyFill (out.cell, fill.grid);
    // a fill may push density out of range for sparse feels: trim gently
    if (rhythm::onsetCount (out.cell) > fp.maxOnsets + 1) enforceDensity (out.cell, rng, fp, fam);
    out.slots = rhythm::slots (out.cell);
    // pitches: keep base pitches for the untouched part, shape the fill part
    std::vector<int> pitches (out.slots.size(), 0);
    for (size_t i = 0; i < out.slots.size(); ++i)
        for (size_t j = 0; j < base.slots.size(); ++j)
            if (base.slots[j].step == out.slots[i].step) pitches[i] = base.pitches[j];
    const int from = kStepsPerBar - (int) fill.grid.size();
    out.pitches = pitch::fillPitches (out.slots, from, fill.shape, pitches, ctx);
    for (size_t i = 1; i < out.slots.size(); ++i)
        if (out.slots[i].role == Role::Ghost && out.slots[i].step < from) out.pitches[i] = out.pitches[i - 1];
    if (out.slots.empty()) return base;
    return out;
}
} // namespace

int effectiveSwing (const GeneratorSettings& s, Feel resolvedFeel, int archetypeMaxSwing) noexcept
{
    if (s.swing != kAutoSwing) return std::clamp (s.swing, kMinSwing, kMaxSwing);
    int sw = feelProfile (resolvedFeel).defaultSwing;
    if (s.family == Family::Acid) sw = std::max (50, sw - 2); // acid lines sit straighter
    return std::min (sw, archetypeMaxSwing);
}

Pattern generatePattern (const GeneratorSettings& in, const GenerationOptions& opts, GenerationReport* report)
{
    const GeneratorSettings s = sanitise (in);
    GenerationReport localReport;
    GenerationReport& rep = report ? *report : localReport;
    rep = {};

    // ------------------------------------------------------------- STYLE layer
    Rng styleRng (deriveSeed (s.styleSeed, "style-core"));
    const Feel feel = resolveFeel (s.feel, styleRng);
    const FeelProfile fp = withComplexity (feelProfile (feel), s.complexity);
    rep.resolvedFeel = feel;

    // personal corpus from the taste profile (string_views point into opts.taste, alive for this call)
    std::vector<RhythmArchetype> learned;
    std::vector<int> learnedWeights;
    if (opts.taste != nullptr)
    {
        int idx = 0;
        for (const auto& c : opts.taste->cells)
        {
            const auto& fp0 = feelProfile (c.feel);
            const int n = rhythm::onsetCount (rhythm::parseGrid (c.grid));
            if (n >= fp0.minOnsets && n <= fp0.maxOnsets)
                learned.push_back ({ 9000 + idx, "Learned", c.grid, c.feel, Feel::Random, familyBit (c.family), kFitAll,
                                     ContourHint::Stepwise, 58, "cell learned from a LIKE" }),
                learnedWeights.push_back (c.weight);
            ++idx;
        }
    }
    std::vector<int> poolW;
    auto pool = archetypePool (feel, s.family, poolW, s.kickContext, opts.taste, learned, learnedWeights);
    if (pool.empty()) // cannot happen with the shipped corpus (tested), defensive only
    {
        for (const auto& a : rhythmCorpus()) { pool.push_back (&a); poolW.push_back (1); }
    }
    if (s.complexity != kNeutralComplexity)
    {
        // Complexity: prefer archetypes whose onset count is near the adjusted target
        const int span = std::max (1, fp.maxOnsets - fp.minOnsets);
        for (size_t i = 0; i < pool.size(); ++i)
        {
            const int n = rhythm::onsetCount (rhythm::parseGrid (pool[i]->grid));
            const int dist = std::abs (n - fp.targetOnsets);
            poolW[i] *= std::max (1, 6 - dist * 6 / span);
        }
    }
    const RhythmArchetype& arch = *pool[(size_t) styleRng.pickWeighted (poolW.data(), (int) poolW.size())];

    // answer archetype: prefer moderately similar grooves from the same pool
    const RhythmCell archCell = rhythm::parseGrid (arch.grid);
    std::vector<int> ansW;
    for (size_t i = 0; i < pool.size(); ++i)
    {
        if (pool[i] == &arch) { ansW.push_back (0); continue; }
        const double sim = rhythm::onsetSimilarity (archCell, rhythm::parseGrid (pool[i]->grid));
        ansW.push_back (poolW[i] * (sim >= 0.2 && sim <= 0.75 ? 3 : 1));
    }
    const bool hasAnswerArch = pool.size() > 1;
    const RhythmArchetype& ansArch = hasAnswerArch ? *pool[(size_t) styleRng.pickWeighted (ansW.data(), (int) ansW.size())] : arch;

    PitchContext ctx { s.root, s.scale, s.family, arch.contour };
    if (s.family == Family::Acid && ctx.contour == ContourHint::Pedal && styleRng.chancePermille (500)) ctx.contour = ContourHint::Riff;
    const MotifVocabulary vocab = pitch::chooseVocabulary (styleRng, ctx);

    const int tmplIdx = phrase::pickTemplateIndex (s.bars, styleRng);
    const auto plan = phrase::phraseTemplate (s.bars, tmplIdx);
    const std::array<int, 3> ansWeights = s.family == Family::Sub ? std::array<int, 3> { 2, 3, 1 } : std::array<int, 3> { 3, 2, 2 };
    const AnswerType ansType = (AnswerType) styleRng.pickWeighted (ansWeights.data(), 3);
    const std::array<int, 4> transSteps { 3, -2, 4, -1 };
    const std::array<int, 4> transW { 3, 2, 2, 1 };
    const int transpose = transSteps[(size_t) styleRng.pickWeighted (transW.data(), 4)];
    const int swing = effectiveSwing (s, feel, arch.maxSwing);

    ArticulationParams ap { feel, s.family, s.root, s.scale, s.octave, swing };
    EvalContext coreEval { feel, s.family, s.kickContext, false, opts.taste, &fp };
    EvalContext fullEval { feel, s.family, s.kickContext, true, opts.taste, &fp };

    auto finishMeta = [&] (Pattern& p) {
        p.settings = s;
        p.resolvedFeel = feel;
        p.archetypeId = arch.id;
        p.answerArchetypeId = ansArch.id;
        std::string planStr;
        for (auto& l : plan) planStr += (planStr.empty() ? "" : " ") + l;
        p.phrasePlan = planStr;
        p.swingPercent = swing;
        p.tasteRevision = opts.taste != nullptr ? opts.taste->revision : 0;
    };

    // core motif A: best of N style candidates
    Core core;
    double bestCore = -1;
    const int nStyle = std::max (1, opts.styleCandidates);
    for (int i = 0; i < nStyle; ++i)
    {
        Rng r (deriveSeed (s.styleSeed, "style-candidate", (uint64_t) i));
        Core c;
        c.cell = archCell;
        const int muts = i == 0 ? 0 : r.range (0, fp.maxStyleMutations);
        applyRandomMutations (c.cell, muts, r, fp, s.family);
        enforceDensity (c.cell, r, fp, s.family);
        c.slots = rhythm::slots (c.cell);
        c.pitches = pitch::assignPitches (c.slots, vocab, r, ctx, BarFunction::Statement, 0);
        Pattern one = phrase::renderBars ({ toBar ("A", c) }, ap);
        finishMeta (one);
        one.bars = 1;
        const auto sc = evaluatePattern (one, coreEval);
        const double v = sc.rejected ? -1.0 : sc.total;
        if (opts.collectLog) rep.styleLog.push_back ({ i, v, sc.rejected, sc.describe() });
        if (v > bestCore) { bestCore = v; core = c; rep.chosenStyle = i; }
    }
    if (rep.chosenStyle < 0) // every candidate rejected: use the plain archetype
    {
        Rng r (deriveSeed (s.styleSeed, "style-fallback"));
        core.cell = archCell;
        enforceDensity (core.cell, r, fp, s.family);
        core.slots = rhythm::slots (core.cell);
        core.pitches = pitch::assignPitches (core.slots, vocab, r, ctx, BarFunction::Statement, 0);
        rep.fallbackUsed = true;
    }

    // answer B (phrase design => STYLE)
    Core answer;
    {
        Rng r (deriveSeed (s.styleSeed, "answer"));
        switch (ansType)
        {
            case AnswerType::Transposed:
                answer = core;
                for (size_t i = 0; i < answer.pitches.size(); ++i)
                {
                    int p = pitch::transposeDiatonic (answer.pitches[i], transpose, s.scale);
                    while (p > pitch::maxSemis (s.family)) p -= 12;
                    while (p < pitch::minSemis (s.family)) p += 12;
                    answer.pitches[i] = p;
                }
                break;
            case AnswerType::Inverted:
                answer = core;
                for (size_t i = 0; i < answer.pitches.size(); ++i)
                {
                    if (answer.slots[i].role == Role::Anchor || answer.slots[i].role == Role::Octave) continue;
                    const int st = phrase::diatonicStepsFromRoot (answer.pitches[i], s.scale);
                    int p = pitch::transposeDiatonic (0, -st, s.scale);
                    while (p > pitch::maxSemis (s.family)) p -= 12;
                    while (p < pitch::minSemis (s.family)) p += 12;
                    answer.pitches[i] = p;
                }
                for (size_t i = 1; i < answer.slots.size(); ++i)
                    if (answer.slots[i].role == Role::Ghost) answer.pitches[i] = answer.pitches[i - 1];
                break;
            case AnswerType::Contrast:
                answer.cell = rhythm::parseGrid (ansArch.grid);
                applyRandomMutations (answer.cell, r.range (0, 1), r, fp, s.family);
                enforceDensity (answer.cell, r, fp, s.family);
                answer.slots = rhythm::slots (answer.cell);
                answer.pitches = pitch::assignPitches (answer.slots, vocab, r, ctx, BarFunction::Answer,
                                                       core.pitches.empty() ? 0 : core.pitches.back());
                break;
        }
    }

    rep.archetypeId = arch.id;
    rep.answerArchetypeId = ansArch.id;
    rep.coreGrid = rhythm::toGrid (core.cell);
    for (const auto& e : vocab.entries) rep.vocabulary.push_back (std::to_string (e.semis));
    rep.answerType = ansType == AnswerType::Transposed ? "transposed" : (ansType == AnswerType::Inverted ? "inverted" : "contrast");

    // --------------------------------------------------------------- VAR layer
    const uint64_t varBase = s.varSeed ^ mix64 (s.styleSeed);
    Pattern best;
    double bestScore = -1;
    const int nVar = std::max (1, opts.varCandidates);
    for (int j = 0; j < nVar; ++j)
    {
        Rng v (deriveSeed (varBase, "var-candidate", (uint64_t) j));
        std::vector<BarRealisation> bars;
        Core prevBase = core;
        for (size_t b = 0; b < plan.size(); ++b)
        {
            const auto& label = plan[b];
            Core c;
            if (label == "A") c = core;
            else if (label == "A*")
            {
                const int mode = v.range (0, 9);
                if (mode < 4) c = varyCore (core, v, fp, s.family, vocab, ctx, 0, 1, 0);
                else if (mode < 8) c = varyCore (core, v, fp, s.family, vocab, ctx, 1, 0, 1000);
                else c = makeFill (core, v, pickFill (v, s.family, false), ctx, fp, s.family);
            }
            else if (label == "A'") c = varyCore (core, v, fp, s.family, vocab, ctx, 1, v.range (0, 1), 600);
            else if (label == "A''")
            {
                // Explicit sequencing: function-argument evaluation order is unspecified
                // (GCC/MSVC right-to-left, Clang left-to-right). This order matches the
                // algorithmVersion 1 reference output.
                const int pitchOps = v.range (1, 2);
                const int rhythmOps = v.range (1, 2);
                c = varyCore (core, v, fp, s.family, vocab, ctx, rhythmOps, pitchOps, 800);
            }
            else if (label == "B") c = answer;
            else if (label == "B'") c = varyCore (answer, v, fp, s.family, vocab, ctx, 1, 1, 600);
            else if (label == "F")
            {
                const FillCell& fill = pickFill (v, s.family, false); // drawn first (reference order)
                const bool fromCore = v.chancePermille (700);
                c = makeFill (fromCore ? core : prevBase, v, fill, ctx, fp, s.family);
            }
            else if (label == "F2") c = makeFill (core, v, pickFill (v, s.family, true), ctx, fp, s.family);
            else c = core;
            if (b + 1 == plan.size()) pitch::resolveEnding (c.slots, c.pitches, ctx, v);
            if (label != "F" && label != "F2") prevBase = c;
            bars.push_back (toBar (label, c));
        }
        Pattern cand = phrase::renderBars (bars, ap);
        finishMeta (cand);
        const auto sc = evaluatePattern (cand, fullEval);
        const double val = sc.rejected ? -1.0 : sc.total;
        if (sc.rejected) rep.rejectedCandidates++;
        if (opts.collectLog) rep.varLog.push_back ({ j, val, sc.rejected, sc.describe() });
        if (val > bestScore) { bestScore = val; best = std::move (cand); rep.chosenVar = j; }
    }

    if (rep.chosenVar < 0) // every variation rejected: repeat the core motif
    {
        std::vector<BarRealisation> bars;
        for (int b = 0; b < s.bars; ++b) bars.push_back (toBar ("A", core));
        best = phrase::renderBars (bars, ap);
        finishMeta (best);
        rep.fallbackUsed = true;
    }
    rep.chosenScore = bestScore;

    // --------------------------------------------------- expression (own stream)
    Rng ex (deriveSeed (varBase, "expression"));
    for (auto& n : best.notes)
    {
        const int jitter = n.accent ? ex.range (0, 4) : ex.range (-3, 3);
        n.velocity = (uint8_t) std::clamp ((int) n.velocity + jitter, 1, 127);
    }
    // ------------------------------- optional chromatic approach notes (own stream)
    if (s.chromatic && best.notes.size() > 2)
    {
        Rng ch (deriveSeed (varBase, "chromatic"));
        int lastBar = -1;
        size_t lockedTarget = 0; // the note an approach resolves to is never altered itself
        for (size_t i = 1; i + 1 < best.notes.size(); ++i)
        {
            if (i == lockedTarget) continue;
            auto& n = best.notes[i];
            const auto& next = best.notes[i + 1];
            const int step = tickToStep (n.startTick);
            const int bar = step / kStepsPerBar;
            if (bar == lastBar || step % 4 == 0 || n.accent) continue; // weak positions only, max one per bar
            const int d = (int) next.midiNote - (int) n.midiNote;
            if (d == 0 || std::abs (d) > 5) continue;
            const int approach = (int) next.midiNote + (d > 0 ? -1 : 1); // a semitone before the target
            if (approach < 0 || approach > 127 || theory::inScale (s.root, s.scale, approach % 12)) continue;
            if (approach == (int) best.notes[i - 1].midiNote) continue;
            if (! ch.chancePermille (300)) continue;
            n.midiNote = (uint8_t) approach;
            lastBar = bar;
            lockedTarget = i + 1;
        }
    }
    std::string err = validatePattern (best);
    if (! err.empty()) normalisePattern (best);
    best.hash = computePatternHash (best);
    rep.phrasePlan = best.phrasePlan;
    return best;
}
} // namespace gfb

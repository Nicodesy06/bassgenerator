#include "core/RhythmCorpus.h"

#include <array>

namespace gfb
{
namespace
{
using F = Feel;
using C = ContourHint;
constexpr F None = F::Random;

// clang-format off
constexpr std::array<RhythmArchetype, 57> kCorpus {{
    // ----------------------------------------------------------------- SPARSE
    { 101, "Offbeat Pair",      "..X--.....X-....", F::Sparse, None,       kFamSub,  kFitFour | kFitOpen, C::Pedal,        58, "two long offbeat notes leaving every kick free" },
    { 102, "Late Ghost",        "..X-......x-.g..", F::Sparse, None,       kFamSub,  kFitFour,            C::RootFifth,    58, "offbeat statement with a ghost answer before the bar line" },
    { 103, "Delayed Answer",    "A--.......X-..x.", F::Sparse, F::Stabby,  kFamBoth, kFitBroken | kFitOpen, C::CallResponse, 56, "downbeat hold, silence, then a two-note answer" },
    { 104, "Push Pair",         "...X-.......X-..", F::Sparse, None,       kFamSub,  kFitAll,             C::OctaveBounce, 54, "anticipated beat-2 push and a beat-4 statement" },
    { 105, "Three Points",      "..X-...x-.....X.", F::Sparse, F::Syncopated, kFamBoth, kFitFour,         C::Riff,         58, "three offbeat points spanning the bar" },
    { 106, "Late Entry",        "......X--..x-...", F::Sparse, None,       kFamSub,  kFitFour | kFitOpen, C::Pedal,        56, "enters after beat 2, breathing intro bass" },
    { 107, "Anchor Echo",       "A-.....g..X--...", F::Sparse, None,       kFamSub,  kFitBroken | kFitOpen, C::RootFifth,  56, "root on one, ghost echo, long offbeat" },
    { 108, "Slide Pair",        "..X~x.......X-..", F::Sparse, None,       kFamAcid, kFitAll,             C::Stepwise,     56, "acid slide pair then a single statement" },
    { 109, "Slide Hook",        "X-.......x~X....", F::Sparse, F::Stabby,  kFamAcid, kFitBroken | kFitOpen, C::Riff,       56, "downbeat, gap, sliding hook into beat 3 off" },
    { 110, "Octave Lift",       "..o-......X-..g.", F::Sparse, None,       kFamBoth, kFitFour,            C::OctaveBounce, 58, "octave offbeat lifts against a low offbeat answer" },
    // --------------------------------------------------------------- BALANCED
    { 201, "Offbeat Four",      "..X-..x-..X-..x-", F::Balanced, F::Sparse,  kFamBoth, kFitFour,          C::RootFifth,    60, "classic house offbeat bass, one note per off-8th" },
    { 202, "Tresillo Lean",     "A-.x..X-.x...xX.", F::Balanced, F::Syncopated, kFamBoth, kFitBroken | kFitOpen, C::Riff, 58, "downbeat anchor with 3-3 leaning answers" },
    { 203, "Double Offbeat",    "..X-.xX-..X-.xx-", F::Balanced, None,      kFamSub,  kFitFour,           C::RootFifth,    60, "offbeats with 16th pickups into beats 2 and 4" },
    { 204, "Pickup Anchor",     "A-.xX-..A-..X-x.", F::Balanced, None,      kFamBoth, kFitBroken,         C::OctaveBounce, 56, "anchors on 1 and 3 with 16th pickups" },
    { 205, "Offbeat Doubles",   "..Xx..X-..Xx..X.", F::Balanced, F::Rolling, kFamBoth, kFitFour,          C::Riff,         60, "off-8th plus trailing 16th in two places" },
    { 206, "Broken Walk",       "X-..x-.xX-..x-..", F::Balanced, None,      kFamSub,  kFitBroken | kFitOpen, C::Stepwise,  56, "broken-beat friendly walking line" },
    { 207, "Octave Stab Four",  "..X-x-..X-..o-..", F::Balanced, None,      kFamSub,  kFitBroken,         C::OctaveBounce, 56, "offbeat entry, beat answers, octave on four" },
    { 208, "Rubber Cell",       "A-..x..xA-..x.x.", F::Balanced, None,      kFamBoth, kFitBroken | kFitOpen, C::CallResponse, 58, "anchor every half bar with rubbery answers" },
    { 209, "Lean In",           ".xX-..x...xX-.x.", F::Balanced, F::Syncopated, kFamBoth, kFitFour,       C::Riff,         60, "16th lean-ins into off-8ths" },
    { 210, "Acid Swing Cell",   "A~x...X-..x~X-..", F::Balanced, None,      kFamAcid, kFitAll,            C::Stepwise,     58, "acid cell with two slides and space" },
    // ------------------------------------------------------------------- BUSY
    { 301, "Acid Chatter",      "Ax.xX.xxA.xxX.x.", F::Busy, None,          kFamAcid, kFitAll,            C::Stepwise,     56, "dense 16th acid line with anchor resets" },
    { 302, "Machine Pulse",     "X.xxX.x.X.xxX.xx", F::Busy, None,          kFamAcid, kFitBroken | kFitOpen, C::Riff,      54, "driving pulse with doubled pickups" },
    { 303, "Busy Walk",         "A-xxX-.xA-xx.xX.", F::Busy, F::Rolling,    kFamBoth, kFitBroken,         C::Stepwise,     56, "walking busy line, holds on anchors" },
    { 304, "Offbeat Engine",    "..XxX-xx..XxX-x.", F::Busy, None,          kFamBoth, kFitFour,           C::RootFifth,    58, "busy but leaves beat 1 and 3 for the kick" },
    { 305, "Acid Gallop",       "Xx.xX-xgXx.x.xX-", F::Busy, None,          kFamAcid, kFitBroken | kFitOpen, C::Riff,      56, "galloping 16ths with ghost breath" },
    { 306, "Ghost Motor",       "A-xg.xX-xgX-.xx.", F::Busy, None,          kFamSub,  kFitBroken | kFitOpen, C::RootFifth, 58, "ghost-laden sub motor" },
    { 307, "Slide Machine",     "X~xx.xX~x.xX~xx.", F::Busy, None,          kFamAcid, kFitAll,            C::Stepwise,     54, "three slide groups, high movement" },
    { 308, "Split Anchors",     "A.x.xX.xA.x.xXx.", F::Busy, None,          kFamBoth, kFitBroken,         C::OctaveBounce, 56, "anchored halves with syncopated accents" },
    { 309, "Three Three Two",   "x-xx-xX-xx-xX-x.", F::Busy, F::Syncopated, kFamSub,  kFitAll,            C::Riff,         56, "3+3+2 groupings in 16ths" },
    // ---------------------------------------------------------------- ROLLING
    { 401, "Rolling Gallop",    ".xXx.xXx.xXx.xXx", F::Rolling, None,       kFamSub,  kFitFour,           C::RootFifth,    60, "rolling offbeat gallop, kick holes on beats" },
    { 402, "Twin Offbeats",     "..Xx..Xx..Xx..Xx", F::Rolling, F::Balanced, kFamBoth, kFitFour,          C::OctaveBounce, 62, "paired off-8th + 16th, bouncing" },
    { 403, "Anchor Roll",       "A.xxA.x.A.xxA-.x", F::Rolling, F::Busy,    kFamBoth, kFitBroken | kFitOpen, C::Pedal,     58, "rolls hanging from on-beat anchors" },
    { 404, "Ghost Bounce",      ".gX-.gX-.gX-.gXx", F::Rolling, None,       kFamSub,  kFitFour,           C::RootFifth,    62, "ghost pickups into held offbeats" },
    { 405, "Rubber Roll",       "x.Xxx.Xx.xXx.xX.", F::Rolling, None,       kFamBoth, kFitBroken,         C::Riff,         60, "elastic rolling cell" },
    { 406, "Bounce Skip",       "..XxX..xX.xX..Xx", F::Rolling, None,       kFamBoth, kFitFour | kFitOpen, C::CallResponse, 60, "bouncing line that skips a beat" },
    { 407, "Acid Roll",         ".xX~x.X-.xX~x.X-", F::Rolling, None,       kFamAcid, kFitFour,           C::Stepwise,     58, "rolling acid with mirrored slides" },
    { 408, "Hold Roll",         "A-.xX.xx.xX-.xX.", F::Rolling, None,       kFamBoth, kFitBroken | kFitOpen, C::OctaveBounce, 58, "long anchor then a roll chain" },
    { 409, "Ghost Train",       "..Xg.xXg..Xg.xXg", F::Rolling, None,       kFamSub,  kFitFour,           C::Pedal,        62, "offbeats trailed by ghosts" },
    // ------------------------------------------------------------- SYNCOPATED
    { 501, "Three Over Four",   "X--x--X--x--X-..", F::Syncopated, None,    kFamBoth, kFitBroken | kFitOpen, C::RootFifth, 54, "3-3-3-3-4 dotted-8th grouping" },
    { 502, "Offbeat Dotted",    "..X--X--X-..x-x.", F::Syncopated, None,    kFamSub,  kFitFour,           C::CallResponse, 56, "dotted groups starting off the beat" },
    { 503, "Anchor Tresillo",   "A--X--x-.xX-..X-", F::Syncopated, None,    kFamBoth, kFitBroken,         C::Riff,         56, "tresillo opening and offbeat answer" },
    { 504, "Anticipations",     "...X-..X-..X-.x.", F::Syncopated, F::Sparse, kFamSub, kFitFour,          C::Pedal,        56, "every beat anticipated by a 16th" },
    { 505, "Late Dotted",       ".X-.X-.X-.xX-.x.", F::Syncopated, None,    kFamBoth, kFitAll,            C::Stepwise,     56, "dotted chain starting on the e" },
    { 506, "Push Grid",         "..X-.X-x-..X-..x", F::Syncopated, None,    kFamSub,  kFitFour,           C::OctaveBounce, 56, "pushed 3-step chain with a bar-end pickup" },
    { 507, "Acid Displace",     "X~x.x~X-..x~X-..", F::Syncopated, None,    kFamAcid, kFitBroken | kFitOpen, C::Riff,      56, "displaced acid slides" },
    { 508, "Acid Hiccup",       "..Xx-.X..x-.X-x.", F::Syncopated, None,    kFamAcid, kFitFour,           C::Stepwise,     56, "hiccup doubles around offbeats" },
    { 509, "Anchor Push",       "A-.X-..X..X-.Xx.", F::Syncopated, None,    kFamBoth, kFitBroken,         C::CallResponse, 56, "anchor then pushes against the grid" },
    { 510, "Sync Offbeat Lock", "..X-..X..X-..xX-", F::Syncopated, F::Balanced, kFamBoth, kFitFour,      C::RootFifth,    58, "offbeat lock with a late-bar push" },
    // ----------------------------------------------------------------- STABBY
    { 601, "Stab Grid",         "X..x.X....X..x..", F::Stabby, None,        kFamBoth, kFitBroken | kFitOpen, C::Riff,      54, "clipped 3-3-4-3 stabs" },
    { 602, "Offbeat Stabs",     "..X...X.x...X...", F::Stabby, F::Sparse,   kFamSub,  kFitFour,           C::RootFifth,    56, "short offbeat punches" },
    { 603, "Twin Stab",         "X.X...x.X.X...x.", F::Stabby, None,        kFamBoth, kFitBroken,         C::OctaveBounce, 54, "paired 8th stabs" },
    { 604, "Late Stabs",        "...X..X....X..X.", F::Stabby, None,        kFamSub,  kFitFour,           C::Pedal,        56, "late 16th stabs with big gaps" },
    { 605, "Punch Hole",        "A.....X.X.....X.", F::Stabby, F::Sparse,   kFamBoth, kFitBroken | kFitOpen, C::CallResponse, 54, "downbeat punch, hole, answer" },
    { 606, "Skip Stab",         ".X..X..X.X..X...", F::Stabby, None,        kFamAcid, kFitAll,            C::Stepwise,     54, "skipping acid stabs" },
    { 607, "Clave Stab",        "X..X..X.X..X..X.", F::Stabby, F::Syncopated, kFamBoth, kFitBroken | kFitOpen, C::Riff,  54, "3-3-2 clave-like stabs" },
    { 608, "Acid Pecks",        "..X.x...X.x...xX", F::Stabby, None,        kFamAcid, kFitFour,           C::Stepwise,     56, "pecking acid stabs with an end flick" },
    { 609, "Front Stabs",       "X.x.....X.x.X...", F::Stabby, None,        kFamSub,  kFitBroken,         C::RootFifth,    54, "front-loaded half-bar stabs" },
}};

constexpr std::array<FillCell, 10> kFills {{
    { 1, "Run Up 3",        "x.xx",     FillShape::RunUp,      kFamBoth },
    { 2, "Run Four",        "XxXx",     FillShape::RunDown,    kFamAcid },
    { 3, "Octave Kick",     "X-.o",     FillShape::OctaveJump, kFamBoth },
    { 4, "Drop Out",        "....",     FillShape::Break,      kFamBoth },
    { 5, "Half Run",        "x.x.X.xx", FillShape::RunUp,      kFamBoth },
    { 6, "Dotted Descent",  "X-x-x-x-", FillShape::RunDown,    kFamSub  },
    { 7, "Late Climb",      "..x.xxXx", FillShape::RunUp,      kFamAcid },
    { 8, "Approach",        "..X-..x.", FillShape::Approach,   kFamSub  },
    { 9, "Slide Turn",      "X~x.x~X.", FillShape::RunDown,    kFamAcid },
    { 10, "Pedal Stutter",  "x.gxx.X-", FillShape::Pedal,      kFamSub  },
}};
// clang-format on
} // namespace

std::span<const RhythmArchetype> rhythmCorpus() noexcept { return kCorpus; }
std::span<const FillCell> fillCorpus() noexcept { return kFills; }

const RhythmArchetype* findArchetype (int id) noexcept
{
    for (const auto& a : kCorpus)
        if (a.id == id) return &a;
    return nullptr;
}
} // namespace gfb

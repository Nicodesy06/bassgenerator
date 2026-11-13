#pragma once
// Bounded history of exact generated pattern snapshots (per plugin instance,
// also stored in the project state).
// Policy:
//  - capacity 64, oldest entries dropped first
//  - a push whose note hash AND settings equal the newest entry is ignored
//    (re-selecting the same result does not flood the list); identical
//    patterns further back are kept (they are separate moments of the session)
//  - navigating (back/forward/select) only moves the cursor; pushing a new
//    pattern appends at the end and moves the cursor there (nothing is lost)
//  - recalling an entry restores the exact notes AND its generator settings
//    (root/scale/family/feel/bars/octave/seeds); synth settings are untouched.

#include <deque>
#include <optional>

#include "core/PatternTypes.h"

namespace gfb
{
class PatternHistory
{
public:
    static constexpr int kCapacity = 64;

    /** Returns true if the pattern was added. */
    bool push (const Pattern& p);
    int size() const noexcept { return (int) entries.size(); }
    bool empty() const noexcept { return entries.empty(); }
    int cursor() const noexcept { return cursorIndex; }
    const Pattern& at (int index) const { return entries.at ((size_t) index); }

    std::optional<Pattern> back();     // older
    std::optional<Pattern> forward();  // newer
    std::optional<Pattern> select (int index);
    bool canGoBack() const noexcept { return cursorIndex > 0; }
    bool canGoForward() const noexcept { return cursorIndex + 1 < size(); }
    void clear() noexcept { entries.clear(); cursorIndex = -1; }

    /** Restore from serialized state (keeps at most kCapacity newest). */
    void assign (std::deque<Pattern> list, int cursor);
    const std::deque<Pattern>& all() const noexcept { return entries; }

private:
    std::deque<Pattern> entries;
    int cursorIndex = -1;
};
} // namespace gfb

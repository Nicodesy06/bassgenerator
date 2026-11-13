#include "core/PatternHistory.h"

#include <algorithm>

namespace gfb
{
bool PatternHistory::push (const Pattern& p)
{
    if (! entries.empty())
    {
        const auto& last = entries.back();
        if (last.hash == p.hash && last.notes == p.notes && last.settings == p.settings)
        {
            cursorIndex = size() - 1;
            return false;
        }
    }
    entries.push_back (p);
    while ((int) entries.size() > kCapacity) entries.pop_front();
    cursorIndex = size() - 1;
    return true;
}

std::optional<Pattern> PatternHistory::back()
{
    if (! canGoBack()) return std::nullopt;
    return entries[(size_t) --cursorIndex];
}

std::optional<Pattern> PatternHistory::forward()
{
    if (! canGoForward()) return std::nullopt;
    return entries[(size_t) ++cursorIndex];
}

std::optional<Pattern> PatternHistory::select (int index)
{
    if (index < 0 || index >= size()) return std::nullopt;
    cursorIndex = index;
    return entries[(size_t) index];
}

void PatternHistory::assign (std::deque<Pattern> list, int cur)
{
    while ((int) list.size() > kCapacity) list.pop_front();
    entries = std::move (list);
    cursorIndex = entries.empty() ? -1 : std::clamp (cur, 0, size() - 1);
}
} // namespace gfb

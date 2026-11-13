#pragma once
// Minimal dependency-free test harness. Usage:
//   TEST_CASE (Suite, name) { CHECK (x == 1); REQUIRE (ptr != nullptr); }
// Run: gfb_tests [suite-substring]
#include <cmath>
#include <cstdio>
#include <functional>
#include <sstream>
#include <stdexcept>
#include <string>
#include <vector>

namespace gfbtest
{
struct Case { std::string suite, name; std::function<void()> fn; };
inline std::vector<Case>& registry() { static std::vector<Case> r; return r; }
/** Optional sub-process entry point: `gfb_juce_tests --worker ...` (multi-process tests). */
inline std::function<int (int, char**)>& workerHook() { static std::function<int (int, char**)> h; return h; }
inline int& failures() { static int f = 0; return f; }
inline int& checks() { static int c = 0; return c; }
struct Registrar { Registrar (const char* s, const char* n, std::function<void()> f) { registry().push_back ({ s, n, std::move (f) }); } };
struct RequireFailed : std::runtime_error { using std::runtime_error::runtime_error; };

inline bool check (bool ok, const char* expr, const char* file, int line, const std::string& extra = {})
{
    ++checks();
    if (! ok)
    {
        ++failures();
        std::printf ("  FAIL %s:%d: %s %s\n", file, line, expr, extra.c_str());
    }
    return ok;
}

template <typename A, typename B>
std::string describe (const A& a, const B& b)
{
    std::ostringstream o;
    o << "(lhs=" << a << ", rhs=" << b << ")";
    return o.str();
}
} // namespace gfbtest

#define GFB_CAT2(a, b) a##b
#define GFB_CAT(a, b) GFB_CAT2 (a, b)
#define TEST_CASE(suite, name)                                                                         \
    static void GFB_CAT (test_, GFB_CAT (suite, name)) ();                                             \
    static gfbtest::Registrar GFB_CAT (reg_, GFB_CAT (suite, name)) (#suite, #name, &GFB_CAT (test_, GFB_CAT (suite, name))); \
    static void GFB_CAT (test_, GFB_CAT (suite, name)) ()

#define CHECK(cond) gfbtest::check ((cond), #cond, __FILE__, __LINE__)
#define CHECK_MSG(cond, msg) gfbtest::check ((cond), #cond, __FILE__, __LINE__, (msg))
#define CHECK_EQ(a, b) [&] { const auto& gfb_a_ = (a); const auto& gfb_b_ = (b); return gfbtest::check (gfb_a_ == gfb_b_, #a " == " #b, __FILE__, __LINE__, gfbtest::describe (gfb_a_, gfb_b_)); }()
#define CHECK_NEAR(a, b, eps) [&] { const double gfb_a_ = (double) (a), gfb_b_ = (double) (b); return gfbtest::check (std::abs (gfb_a_ - gfb_b_) <= (eps), #a " ~= " #b, __FILE__, __LINE__, gfbtest::describe (gfb_a_, gfb_b_)); }()
#define REQUIRE(cond) do { if (! gfbtest::check ((cond), #cond, __FILE__, __LINE__)) throw gfbtest::RequireFailed (#cond); } while (0)

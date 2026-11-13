#include <chrono>
#include <cstring>

#include "TestFramework.h"

int main (int argc, char** argv)
{
    const char* filter = argc > 1 ? argv[1] : "";
    int ran = 0, failedCases = 0;
    const auto t0 = std::chrono::steady_clock::now();
    for (auto& c : gfbtest::registry())
    {
        const std::string full = c.suite + "." + c.name;
        if (*filter && full.find (filter) == std::string::npos) continue;
        const int before = gfbtest::failures();
        const auto s = std::chrono::steady_clock::now();
        try { c.fn(); }
        catch (const gfbtest::RequireFailed&) {}
        catch (const std::exception& e) { ++gfbtest::failures(); std::printf ("  EXCEPTION %s\n", e.what()); }
        const double ms = std::chrono::duration<double, std::milli> (std::chrono::steady_clock::now() - s).count();
        const bool ok = gfbtest::failures() == before;
        failedCases += ok ? 0 : 1;
        std::printf ("[%s] %s (%.1f ms)\n", ok ? " OK " : "FAIL", full.c_str(), ms);
        ++ran;
    }
    const double total = std::chrono::duration<double> (std::chrono::steady_clock::now() - t0).count();
    std::printf ("\n%d test cases, %d failed, %d checks, %d check failures, %.2f s\n", ran, failedCases, gfbtest::checks(), gfbtest::failures(), total);
    if (ran == 0) { std::printf ("no tests matched filter '%s'\n", filter); return 2; }
    return gfbtest::failures() == 0 ? 0 : 1;
}

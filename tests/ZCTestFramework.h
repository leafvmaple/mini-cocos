#pragma once

// Tiny zero-dependency test harness. Each ZC_TEST registers itself via a static
// initializer; runAll() executes them and reports per-test check failures.
// Deliberately uses std:: (host-only test infra) and stays independent of the
// engine's mstd switcher so it works under both STL backends.

#include <cstdio>
#include <vector>

namespace zctest {

struct TestCase {
    const char* name;
    void (*fn)(int&);
};

inline std::vector<TestCase>& registry() {
    static std::vector<TestCase> tests;
    return tests;
}

struct Registrar {
    Registrar(const char* name, void (*fn)(int&)) { registry().push_back({name, fn}); }
};

inline int runAll() {
    int totalFailures = 0;
    for (const TestCase& tc : registry()) {
        int localFailures = 0;
        tc.fn(localFailures);
        if (localFailures) {
            std::printf("[FAIL] %s (%d check(s) failed)\n", tc.name, localFailures);
            totalFailures += localFailures;
        } else {
            std::printf("[ ok ] %s\n", tc.name);
        }
    }
    std::printf("\n%s: %zu test(s), %d check failure(s)\n", totalFailures ? "FAILED" : "PASSED",
                registry().size(), totalFailures);
    return totalFailures ? 1 : 0;
}

} // namespace zctest

#define ZC_TEST(name)                                                                              \
    static void name(int& _zc_fail);                                                               \
    static ::zctest::Registrar _zc_reg_##name(#name, &name);                                       \
    static void name(int& _zc_fail)

#define ZC_CHECK(cond)                                                                             \
    do {                                                                                           \
        if (!(cond)) {                                                                             \
            ++_zc_fail;                                                                            \
            std::printf("    check failed: %s (%s:%d)\n", #cond, __FILE__, __LINE__);              \
        }                                                                                          \
    } while (0)

#define ZC_CHECK_EQ(a, b) ZC_CHECK((a) == (b))

#define ZC_CHECK_NEAR(a, b, eps)                                                                   \
    do {                                                                                           \
        const double _zc_d = static_cast<double>(a) - static_cast<double>(b);                      \
        if (_zc_d > (eps) || _zc_d < -(eps)) {                                                     \
            ++_zc_fail;                                                                            \
            std::printf("    check failed: |%s - %s| > %s (%s:%d)\n", #a, #b, #eps, __FILE__,       \
                        __LINE__);                                                                 \
        }                                                                                          \
    } while (0)

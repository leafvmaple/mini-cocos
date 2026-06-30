#include "ZCTestFramework.h"

#include "base/ZCEasing.h"

using namespace zocos::easing;

ZC_TEST(ease_curves_pass_through_endpoints) {
    float (*curves[])(float) = {sineIn, sineOut, sineInOut, cubicIn, cubicOut, cubicInOut};
    for (auto* f : curves) {
        ZC_CHECK_NEAR(f(0.f), 0.f, 1e-5);
        ZC_CHECK_NEAR(f(1.f), 1.f, 1e-5);
    }
}

ZC_TEST(ease_inout_symmetric_midpoint) {
    ZC_CHECK_NEAR(sineInOut(0.5f), 0.5f, 1e-4);
    ZC_CHECK_NEAR(cubicInOut(0.5f), 0.5f, 1e-4);
}

ZC_TEST(ease_in_is_slow_start_out_is_slow_end) {
    // EaseIn lags the midpoint (slow start), EaseOut leads it (fast start).
    ZC_CHECK(cubicIn(0.5f) < 0.5f);
    ZC_CHECK(cubicOut(0.5f) > 0.5f);
    ZC_CHECK(sineIn(0.5f) < 0.5f);
    ZC_CHECK(sineOut(0.5f) > 0.5f);
}

ZC_TEST(ease_curves_monotonic_nondecreasing) {
    float (*curves[])(float) = {sineIn, sineOut, sineInOut, cubicIn, cubicOut, cubicInOut};
    for (auto* f : curves) {
        float prev = f(0.f);
        for (int i = 1; i <= 32; ++i) {
            const float t = static_cast<float>(i) / 32.f;
            const float v = f(t);
            ZC_CHECK(v >= prev - 1e-4f);
            prev = v;
        }
    }
}

ZC_TEST(ease_rate_curves_endpoints_and_known_values) {
    for (float rate : {1.f, 2.f, 3.f}) {
        ZC_CHECK_NEAR(rateIn(0.f, rate), 0.f, 1e-5);
        ZC_CHECK_NEAR(rateIn(1.f, rate), 1.f, 1e-5);
        ZC_CHECK_NEAR(rateOut(0.f, rate), 0.f, 1e-5);
        ZC_CHECK_NEAR(rateOut(1.f, rate), 1.f, 1e-5);
        ZC_CHECK_NEAR(rateInOut(0.f, rate), 0.f, 1e-5);
        ZC_CHECK_NEAR(rateInOut(1.f, rate), 1.f, 1e-5);
        ZC_CHECK_NEAR(rateInOut(0.5f, rate), 0.5f, 1e-4);
    }
    // rate 1 is linear; rate 2 squares.
    ZC_CHECK_NEAR(rateIn(0.5f, 1.f), 0.5f, 1e-4);
    ZC_CHECK_NEAR(rateIn(0.5f, 2.f), 0.25f, 1e-4);
    ZC_CHECK(rateIn(0.5f, 2.f) < 0.5f);  // slow start
    ZC_CHECK(rateOut(0.5f, 2.f) > 0.5f); // fast start
}

#pragma once

// Pure easing curves used by ActionEase. Each maps normalized time t in [0, 1]
// to an eased value with f(0) = 0 and f(1) = 1. The sine and polynomial families
// need only sin/cos; the rate-parameterized family uses mstd::pow, which the
// freestanding sys:: STL now provides (zstl sys::pow).

#include "base/ZCStd.h"

namespace zocos {
namespace easing {

inline constexpr float kPi = 3.14159265358979323846f;
inline constexpr float kHalfPi = kPi * 0.5f;

inline float sineIn(float t) { return 1.f - mstd::cos(t * kHalfPi); }
inline float sineOut(float t) { return mstd::sin(t * kHalfPi); }
inline float sineInOut(float t) { return -0.5f * (mstd::cos(kPi * t) - 1.f); }

// Rate-parameterized power curves (cocos2d-x EaseIn/EaseOut/EaseInOut). A rate
// of 1 is linear; larger rates steepen the ease.
inline float rateIn(float t, float rate) { return mstd::pow(t, rate); }
inline float rateOut(float t, float rate) { return mstd::pow(t, 1.f / rate); }
inline float rateInOut(float t, float rate) {
    t *= 2.f;
    if (t < 1.f) {
        return 0.5f * mstd::pow(t, rate);
    }
    return 1.f - 0.5f * mstd::pow(2.f - t, rate);
}

inline float cubicIn(float t) { return t * t * t; }
inline float cubicOut(float t) {
    const float u = 1.f - t;
    return 1.f - u * u * u;
}
inline float cubicInOut(float t) {
    if (t < 0.5f) {
        return 4.f * t * t * t;
    }
    const float f = 2.f * t - 2.f;
    return 0.5f * f * f * f + 1.f;
}

} // namespace easing
} // namespace zocos

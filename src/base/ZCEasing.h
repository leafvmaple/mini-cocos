#pragma once

// Pure easing curves used by ActionEase. Each maps normalized time t in [0, 1]
// to an eased value with f(0) = 0 and f(1) = 1. Kept free of std::pow so the
// freestanding sys:: STL (zstl, which exposes sin/cos but not pow) builds too;
// that is why the families here are sine- and polynomial-based.

#include "base/ZCStd.h"

namespace zocos {
namespace easing {

inline constexpr float kPi = 3.14159265358979323846f;
inline constexpr float kHalfPi = kPi * 0.5f;

inline float sineIn(float t) { return 1.f - mstd::cos(t * kHalfPi); }
inline float sineOut(float t) { return mstd::sin(t * kHalfPi); }
inline float sineInOut(float t) { return -0.5f * (mstd::cos(kPi * t) - 1.f); }

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

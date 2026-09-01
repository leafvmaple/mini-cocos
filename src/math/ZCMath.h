#pragma once

// Named ZCMath.h so "Math.h" does not shadow system <math.h> on case-insensitive filesystems.

#include "base/ZCStd.h"

namespace zocos {

struct Vec2 {
    float x = 0.f;
    float y = 0.f;
};

struct Size {
    float width = 0.f;
    float height = 0.f;
};

struct Rect {
    float x = 0.f;
    float y = 0.f;
    float width = 0.f;
    float height = 0.f;
};

// Column-major 4x4, compatible with glUniformMatrix4fv(..., GL_FALSE, ...).
struct Mat4 {
    float m[16]{};

    static Mat4 identity() {
        Mat4 r{};
        r.m[0] = r.m[5] = r.m[10] = r.m[15] = 1.f;
        return r;
    }

    static Mat4 translate(float tx, float ty, float tz = 0.f) {
        Mat4 r = identity();
        r.m[12] = tx;
        r.m[13] = ty;
        r.m[14] = tz;
        return r;
    }

    static Mat4 scale(float sx, float sy, float sz = 1.f) {
        Mat4 r = identity();
        r.m[0] = sx;
        r.m[5] = sy;
        r.m[10] = sz;
        return r;
    }

    static Mat4 rotateZ(float degrees) {
        const float rad = degrees * 3.14159265f / 180.f;
        const float c = mstd::cos(rad);
        const float s = mstd::sin(rad);
        Mat4 r = identity();
        r.m[0] = c;
        r.m[1] = s;
        r.m[4] = -s;
        r.m[5] = c;
        return r;
    }

    static Mat4 ortho(float left, float right, float bottom, float top, float nearZ, float farZ) {
        Mat4 r = identity();
        r.m[0] = 2.f / (right - left);
        r.m[5] = 2.f / (top - bottom);
        r.m[10] = -2.f / (farZ - nearZ);
        r.m[12] = -(right + left) / (right - left);
        r.m[13] = -(top + bottom) / (top - bottom);
        r.m[14] = -(farZ + nearZ) / (farZ - nearZ);
        return r;
    }

    Mat4 operator*(const Mat4& b) const {
        Mat4 o{};
        for (int c = 0; c < 4; ++c) {
            for (int r = 0; r < 4; ++r) {
                o.m[c * 4 + r] = m[0 * 4 + r] * b.m[c * 4 + 0] + m[1 * 4 + r] * b.m[c * 4 + 1] +
                                 m[2 * 4 + r] * b.m[c * 4 + 2] + m[3 * 4 + r] * b.m[c * 4 + 3];
            }
        }
        return o;
    }

    // Transform a 2D point (z = 0, w = 1). This assumes an affine matrix,
    // so no perspective divide is performed.
    Vec2 transformPoint(const Vec2& point) const {
        return {m[0] * point.x + m[4] * point.y + m[12], m[1] * point.x + m[5] * point.y + m[13]};
    }

    const float* data() const { return m; }
};

inline Mat4 zcNodeLocalMatrix(const Vec2& position, const Vec2& scale, float rotationDegrees,
                              const Vec2& anchorPoint, const Size& contentSize) {
    const float ax = anchorPoint.x * contentSize.width;
    const float ay = anchorPoint.y * contentSize.height;
    return Mat4::translate(position.x, position.y, 0.f) * Mat4::rotateZ(rotationDegrees) *
           Mat4::scale(scale.x, scale.y, 1.f) * Mat4::translate(-ax, -ay, 0.f);
}

} // namespace zocos

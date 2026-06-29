#pragma once

#include "math/ZCMath.h"

#include "base/ZCStd.h"

namespace zocos {

using RenderSortKey = mstd::uint64_t;

struct TextureHandle {
    mstd::uint32_t value = 0;

    bool isValid() const { return value != 0; }
};

struct Color4B {
    mstd::uint8_t r = 255;
    mstd::uint8_t g = 255;
    mstd::uint8_t b = 255;
    mstd::uint8_t a = 255;
};

struct QuadVertex {
    Vec2 position{};
    Vec2 uv{};
    Color4B color{};
};

// The renderer speaks a single batchable primitive: a list of triangles (6
// vertices per quad) sharing one texture and opacity, drawn in the space given
// by `world`. Sprites and Labels both lower to this. When flush() merges
// adjacent commands it pre-transforms vertices into world space and leaves
// `world` identity, so a whole batch issues as one draw.
struct RenderCommand {
    RenderSortKey sortKey = 0;
    mstd::uint32_t submissionIndex = 0;
    Mat4 world = Mat4::identity();
    TextureHandle texture{};
    mstd::vector<QuadVertex> vertices;
    float opacity = 1.f;
};

inline RenderSortKey makeRenderSortKey(mstd::uint16_t pass, mstd::uint16_t layer,
                                       mstd::uint32_t material) {
    return (static_cast<RenderSortKey>(pass) << 48) | (static_cast<RenderSortKey>(layer) << 32) |
           static_cast<RenderSortKey>(material);
}

} // namespace zocos
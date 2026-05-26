#pragma once

#include "math/ZCMath.h"

#include "base/ZCStd.h"

namespace zocos {

using RenderSortKey = mstd::uint64_t;

struct TextureHandle {
    mstd::uint32_t value = 0;

    bool isValid() const { return value != 0; }
};

enum class RenderCommandType : mstd::uint8_t {
    DrawSprite = 0,
    DrawQuads = 1,
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

struct DrawSpriteCommand {
    Mat4 world = Mat4::identity();
    Size contentSize{};
    TextureHandle texture{};
    Rect uvRect{0.f, 0.f, 1.f, 1.f};
    float opacity = 1.f;
};

struct DrawQuadsCommand {
    Mat4 world = Mat4::identity();
    TextureHandle texture{};
    mstd::vector<QuadVertex> vertices;
    float opacity = 1.f;
};

struct RenderCommand {
    RenderCommandType type = RenderCommandType::DrawSprite;
    RenderSortKey sortKey = 0;
    mstd::uint32_t submissionIndex = 0;
    DrawSpriteCommand sprite{};
    DrawQuadsCommand quads{};
};

inline RenderSortKey makeRenderSortKey(mstd::uint16_t pass, mstd::uint16_t layer,
                                       mstd::uint32_t material) {
    return (static_cast<RenderSortKey>(pass) << 48) | (static_cast<RenderSortKey>(layer) << 32) |
           static_cast<RenderSortKey>(material);
}

} // namespace zocos
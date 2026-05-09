#pragma once

#include "math/ZCMath.h"

#include <cstdint>

namespace zocos {

using RenderSortKey = std::uint64_t;

struct TextureHandle {
    std::uint32_t value = 0;

    bool isValid() const { return value != 0; }
};

enum class RenderCommandType : std::uint8_t {
    DrawSprite = 0,
};

struct DrawSpriteCommand {
    Mat4 world = Mat4::identity();
    Size contentSize{};
    TextureHandle texture{};
    float opacity = 1.f;
};

struct RenderCommand {
    RenderCommandType type = RenderCommandType::DrawSprite;
    RenderSortKey sortKey = 0;
    std::uint32_t submissionIndex = 0;
    DrawSpriteCommand sprite{};
};

inline RenderSortKey makeRenderSortKey(std::uint16_t pass, std::uint16_t layer,
                                        std::uint32_t material) {
    return (static_cast<RenderSortKey>(pass) << 48) |
        (static_cast<RenderSortKey>(layer) << 32) | static_cast<RenderSortKey>(material);
}

} // namespace zocos
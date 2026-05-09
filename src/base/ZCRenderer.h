#pragma once

#include "base/ZCRenderCommand.h"

#include <vector>

namespace zocos {

class RenderDevice;

class Renderer {
public:
    void beginFrame(const Mat4& projection);
    void addDrawSprite(const Mat4& world, const Size& contentSize, TextureHandle texture,
                       float opacity, RenderSortKey sortKey = 0);
    void addDrawSprite(const Mat4& world, const Size& contentSize, TextureHandle texture,
                       const Rect& uvRect = Rect{0.f, 0.f, 1.f, 1.f}, float opacity = 1.f,
                       RenderSortKey sortKey = 0);
    void flush(RenderDevice& device, int framebufferWidth, int framebufferHeight);
    void endFrame();

private:
    Mat4 _projection = Mat4::identity();
    std::vector<RenderCommand> _commands;
    std::uint32_t _submissionCounter = 0;
};

} // namespace zocos
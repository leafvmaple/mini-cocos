#include "base/ZCRenderer.h"

#include "base/ZCRenderDevice.h"

#include <algorithm>

namespace zocos {

void Renderer::beginFrame(const Mat4& projection) {
    _projection = projection;
    _commands.clear();
    _submissionCounter = 0;
}

void Renderer::addDrawSprite(const Mat4& world, const Size& contentSize, TextureHandle texture,
                             float opacity, RenderSortKey sortKey) {
    if (!texture.isValid()) {
        return;
    }

    RenderCommand cmd;
    cmd.type = RenderCommandType::DrawSprite;
    cmd.sortKey = sortKey;
    cmd.submissionIndex = _submissionCounter++;
    cmd.sprite.world = world;
    cmd.sprite.contentSize = contentSize;
    cmd.sprite.texture = texture;
    cmd.sprite.opacity = opacity;
    _commands.push_back(cmd);
}

void Renderer::flush(RenderDevice& device, int framebufferWidth, int framebufferHeight) {
    std::stable_sort(_commands.begin(), _commands.end(), [](const RenderCommand& a, const RenderCommand& b) {
        if (a.sortKey != b.sortKey) {
            return a.sortKey < b.sortKey;
        }
        return a.submissionIndex < b.submissionIndex;
    });

    device.beginFrame(_projection, framebufferWidth, framebufferHeight);
    for (const auto& command : _commands) {
        device.submit(command);
    }
    device.endFrame();
}

void Renderer::endFrame() {
    _commands.clear();
    _submissionCounter = 0;
}

} // namespace zocos
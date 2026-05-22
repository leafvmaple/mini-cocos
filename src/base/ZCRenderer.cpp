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
    addDrawSprite(world, contentSize, texture, Rect{0.f, 0.f, 1.f, 1.f}, opacity, sortKey);
}

void Renderer::addDrawSprite(const Mat4& world, const Size& contentSize, TextureHandle texture,
                             const Rect& uvRect, float opacity, RenderSortKey sortKey) {
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
    cmd.sprite.uvRect = uvRect;
    cmd.sprite.opacity = opacity;
    _commands.push_back(cmd);
}

void Renderer::addDrawQuads(const Mat4& world, TextureHandle texture,
                            const std::vector<QuadVertex>& vertices, float opacity,
                            RenderSortKey sortKey) {
    if (!texture.isValid() || vertices.empty()) {
        return;
    }

    RenderCommand cmd;
    cmd.type = RenderCommandType::DrawQuads;
    cmd.sortKey = sortKey;
    cmd.submissionIndex = _submissionCounter++;
    cmd.quads.world = world;
    cmd.quads.texture = texture;
    cmd.quads.vertices = vertices;
    cmd.quads.opacity = opacity;
    _commands.push_back(std::move(cmd));
}

void Renderer::flush(RenderDevice& device, int framebufferWidth, int framebufferHeight) {
    std::stable_sort(_commands.begin(), _commands.end(),
                     [](const RenderCommand& a, const RenderCommand& b) {
                         if (a.sortKey != b.sortKey) {
                             return a.sortKey < b.sortKey;
                         }
                         return a.submissionIndex < b.submissionIndex;
                     });

    // Cross-node batching: consecutive DrawQuads commands with the same
    // texture and opacity can be merged into a single draw by pre-transforming
    // each source quad's vertices into world space and emitting them with an
    // identity world matrix. Labels sharing one FontAtlas page collapse into
    // one draw call this way.
    auto transformVerts = [](const Mat4& world, const std::vector<QuadVertex>& src,
                             std::vector<QuadVertex>& dst) {
        dst.reserve(dst.size() + src.size());
        const float m0 = world.m[0];
        const float m1 = world.m[1];
        const float m4 = world.m[4];
        const float m5 = world.m[5];
        const float m12 = world.m[12];
        const float m13 = world.m[13];
        for (const auto& v : src) {
            QuadVertex out;
            out.position.x = m0 * v.position.x + m4 * v.position.y + m12;
            out.position.y = m1 * v.position.x + m5 * v.position.y + m13;
            out.uv = v.uv;
            dst.push_back(out);
        }
    };

    std::vector<RenderCommand> merged;
    merged.reserve(_commands.size());
    for (auto& command : _commands) {
        if (command.type == RenderCommandType::DrawQuads && !merged.empty() &&
            merged.back().type == RenderCommandType::DrawQuads &&
            merged.back().quads.texture.value == command.quads.texture.value &&
            merged.back().quads.opacity == command.quads.opacity) {
            transformVerts(command.quads.world, command.quads.vertices,
                           merged.back().quads.vertices);
            continue;
        }
        if (command.type == RenderCommandType::DrawQuads) {
            RenderCommand out;
            out.type = RenderCommandType::DrawQuads;
            out.sortKey = command.sortKey;
            out.submissionIndex = command.submissionIndex;
            out.quads.world = Mat4::identity();
            out.quads.texture = command.quads.texture;
            out.quads.opacity = command.quads.opacity;
            transformVerts(command.quads.world, command.quads.vertices, out.quads.vertices);
            merged.push_back(std::move(out));
            continue;
        }
        merged.push_back(std::move(command));
    }

    device.beginFrame(_projection, framebufferWidth, framebufferHeight);
    for (const auto& command : merged) {
        device.submit(command);
    }
    device.endFrame();
}

void Renderer::endFrame() {
    _commands.clear();
    _submissionCounter = 0;
}

} // namespace zocos
#include "base/ZCRenderer.h"

#include "base/ZCRenderDevice.h"

#include "base/ZCStd.h"

namespace zocos {

void Renderer::beginFrame(const Mat4& projection) {
    _projection = projection;
    _commands.clear();
    _submissionCounter = 0;
    _globalOpacity = 1.f;
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

    const float w = contentSize.width;
    const float h = contentSize.height;
    const float u0 = uvRect.x;
    const float v0 = uvRect.y;
    const float u1 = uvRect.x + uvRect.width;
    const float v1 = uvRect.y + uvRect.height;

    // A sprite is just a one-quad batch. Fold opacity into per-vertex alpha and
    // emit it through the same DrawQuads path as Labels so consecutive
    // same-texture sprites collapse into a single draw call (see flush()).
    const float clampedOpacity = mstd::clamp(opacity * _globalOpacity, 0.f, 1.f);
    const Color4B color{255, 255, 255, static_cast<mstd::uint8_t>(clampedOpacity * 255.f + 0.5f)};
    const QuadVertex verts[] = {
        {{0.f, 0.f}, {u0, v0}, color}, {{w, 0.f}, {u1, v0}, color}, {{w, h}, {u1, v1}, color},
        {{0.f, 0.f}, {u0, v0}, color}, {{w, h}, {u1, v1}, color},   {{0.f, h}, {u0, v1}, color},
    };

    RenderCommand cmd;
    cmd.sortKey = sortKey;
    cmd.submissionIndex = _submissionCounter++;
    cmd.world = world;
    cmd.texture = texture;
    cmd.opacity = 1.f;
    cmd.vertices.reserve(6);
    for (const QuadVertex& v : verts) {
        cmd.vertices.push_back(v);
    }
    _commands.push_back(mstd::move(cmd));
}

void Renderer::addDrawQuads(const Mat4& world, TextureHandle texture,
                            const mstd::vector<QuadVertex>& vertices, float opacity,
                            RenderSortKey sortKey) {
    if (!texture.isValid() || vertices.empty()) {
        return;
    }

    RenderCommand cmd;
    cmd.sortKey = sortKey;
    cmd.submissionIndex = _submissionCounter++;
    cmd.world = world;
    cmd.texture = texture;
    cmd.vertices = vertices;
    cmd.opacity = mstd::clamp(opacity * _globalOpacity, 0.f, 1.f);
    _commands.push_back(mstd::move(cmd));
}

void Renderer::flush(RenderDevice& device, int framebufferWidth, int framebufferHeight) {
    mstd::stable_sort(_commands.begin(), _commands.end(),
                      [](const RenderCommand& a, const RenderCommand& b) {
                          if (a.sortKey != b.sortKey) {
                              return a.sortKey < b.sortKey;
                          }
                          return a.submissionIndex < b.submissionIndex;
                      });

    // Cross-node batching: consecutive commands with the same texture and
    // opacity merge into a single draw by pre-transforming each source quad's
    // vertices into world space and emitting them with an identity world
    // matrix. Sprites and Labels sharing a texture collapse into one draw this
    // way.
    auto transformVerts = [](const Mat4& world, const mstd::vector<QuadVertex>& src,
                             mstd::vector<QuadVertex>& dst) {
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
            out.color = v.color;
            dst.push_back(out);
        }
    };

    mstd::vector<RenderCommand> merged;
    merged.reserve(_commands.size());
    for (auto& command : _commands) {
        if (!merged.empty() && merged.back().texture.value == command.texture.value &&
            merged.back().opacity == command.opacity) {
            transformVerts(command.world, command.vertices, merged.back().vertices);
            continue;
        }

        RenderCommand out;
        out.sortKey = command.sortKey;
        out.submissionIndex = command.submissionIndex;
        out.world = Mat4::identity();
        out.texture = command.texture;
        out.opacity = command.opacity;
        transformVerts(command.world, command.vertices, out.vertices);
        merged.push_back(mstd::move(out));
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
    _globalOpacity = 1.f;
}

} // namespace zocos

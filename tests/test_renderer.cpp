#include "ZCTestFramework.h"

#include "base/ZCRenderDevice.h"
#include "base/ZCRenderer.h"
#include "base/ZCStd.h"

using namespace zocos;

namespace {
// Records what the Renderer hands the device after sorting and batching.
struct FakeDevice : public RenderDevice {
    int beginFrames = 0;
    int endFrames = 0;
    mstd::vector<mstd::size_t> drawVertexCounts;
    mstd::vector<float> drawOpacities;
    mstd::vector<mstd::uint8_t> firstVertexAlphas;

    void beginFrame(const Mat4&, int, int) override { ++beginFrames; }
    void submit(const RenderCommand& command) override {
        drawVertexCounts.push_back(command.vertices.size());
        drawOpacities.push_back(command.opacity);
        firstVertexAlphas.push_back(command.vertices.empty() ? 0 : command.vertices[0].color.a);
    }
    void endFrame() override { ++endFrames; }

    TextureHandle createTexture(const TextureCreateInfo&) override { return {}; }
    void destroyTexture(TextureHandle) override {}
    void updateTextureRegion(TextureHandle, int, int, int, int, const TextureUploadData&) override {
    }
};

mstd::vector<QuadVertex> unitQuad() {
    mstd::vector<QuadVertex> quad;
    const float pts[6][2] = {{0, 0}, {1, 0}, {1, 1}, {0, 0}, {1, 1}, {0, 1}};
    for (const auto& p : pts) {
        QuadVertex v;
        v.position = {p[0], p[1]};
        quad.push_back(v);
    }
    return quad;
}
} // namespace

ZC_TEST(renderer_merges_same_texture_quads) {
    Renderer renderer;
    FakeDevice device;
    TextureHandle tex;
    tex.value = 7;
    const auto quad = unitQuad();

    renderer.beginFrame(Mat4::identity());
    renderer.addDrawQuads(Mat4::identity(), tex, quad, 1.f, 0);
    renderer.addDrawQuads(Mat4::identity(), tex, quad, 1.f, 0);
    renderer.addDrawQuads(Mat4::identity(), tex, quad, 1.f, 0);
    renderer.flush(device, 800, 600);
    renderer.endFrame();

    ZC_CHECK_EQ(device.beginFrames, 1);
    ZC_CHECK_EQ(device.endFrames, 1);
    ZC_CHECK_EQ(device.drawVertexCounts.size(), static_cast<mstd::size_t>(1)); // one merged draw
    ZC_CHECK_EQ(device.drawVertexCounts[0], static_cast<mstd::size_t>(18));    // 3 quads * 6
}

ZC_TEST(renderer_splits_different_textures) {
    Renderer renderer;
    FakeDevice device;
    const auto quad = unitQuad();
    TextureHandle a;
    a.value = 1;
    TextureHandle b;
    b.value = 2;

    renderer.beginFrame(Mat4::identity());
    renderer.addDrawQuads(Mat4::identity(), a, quad, 1.f, makeRenderSortKey(0, 0, 1));
    renderer.addDrawQuads(Mat4::identity(), b, quad, 1.f, makeRenderSortKey(0, 0, 2));
    renderer.flush(device, 800, 600);
    renderer.endFrame();

    ZC_CHECK_EQ(device.drawVertexCounts.size(), static_cast<mstd::size_t>(2));
}

ZC_TEST(renderer_batches_sprites_sharing_a_texture) {
    Renderer renderer;
    FakeDevice device;
    TextureHandle tex;
    tex.value = 5;

    renderer.beginFrame(Mat4::identity());
    renderer.addDrawSprite(Mat4::identity(), Size{10.f, 10.f}, tex, 1.f, 0);
    renderer.addDrawSprite(Mat4::translate(20.f, 0.f), Size{10.f, 10.f}, tex, 1.f, 0);
    renderer.flush(device, 800, 600);
    renderer.endFrame();

    ZC_CHECK_EQ(device.drawVertexCounts.size(), static_cast<mstd::size_t>(1)); // 2 sprites, 1 draw
    ZC_CHECK_EQ(device.drawVertexCounts[0], static_cast<mstd::size_t>(12));    // 2 * 6
}

ZC_TEST(renderer_skips_invalid_texture) {
    Renderer renderer;
    FakeDevice device;
    TextureHandle invalid; // value == 0
    const auto quad = unitQuad();

    renderer.beginFrame(Mat4::identity());
    renderer.addDrawQuads(Mat4::identity(), invalid, quad, 1.f, 0);
    renderer.addDrawSprite(Mat4::identity(), Size{10.f, 10.f}, invalid, 1.f, 0);
    renderer.flush(device, 800, 600);
    renderer.endFrame();

    ZC_CHECK_EQ(device.drawVertexCounts.size(), static_cast<mstd::size_t>(0));
}

ZC_TEST(renderer_global_opacity_applies_to_quads_and_sprites) {
    Renderer renderer;
    FakeDevice device;
    TextureHandle tex;
    tex.value = 9;

    renderer.beginFrame(Mat4::identity());
    renderer.setGlobalOpacity(0.5f);
    renderer.addDrawQuads(Mat4::identity(), tex, unitQuad(), 0.8f, 0);
    renderer.addDrawSprite(Mat4::identity(), Size{10.f, 10.f}, tex, 0.5f, 1);
    renderer.flush(device, 800, 600);
    renderer.endFrame();

    ZC_CHECK_EQ(device.drawOpacities.size(), static_cast<mstd::size_t>(2));
    ZC_CHECK_NEAR(device.drawOpacities[0], 0.4f, 1e-5);
    ZC_CHECK_EQ(device.firstVertexAlphas[1], static_cast<mstd::uint8_t>(64));
    ZC_CHECK_NEAR(renderer.getGlobalOpacity(), 1.f, 1e-5);
}

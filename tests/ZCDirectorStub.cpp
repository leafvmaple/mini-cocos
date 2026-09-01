#include "base/ZCDirector.h"

#include "base/ZCFontAtlasCache.h"
#include "base/ZCFontCache.h"
#include "base/ZCPlatformFactory.h"
#include "base/ZCTextureCache.h"

namespace zocos {

namespace {
class HeadlessView final : public View {
public:
    bool init(int width, int height, const char*) override {
        _width = width;
        _height = height;
        return true;
    }

    void shutdown() override {}
    bool shouldClose() const override { return false; }
    void pollEvents() override {}
    void swapBuffers() override {}
    int getFramebufferWidth() const override { return _width; }
    int getFramebufferHeight() const override { return _height; }
    bool getMousePosition(float&, float&) const override { return false; }
    double getTimeSeconds() const override {
        _time += 0.05;
        return _time;
    }
    void setDelegate(ViewDelegate* delegate) override { _delegate = delegate; }

private:
    ViewDelegate* _delegate = nullptr;
    int _width = 0;
    int _height = 0;
    mutable double _time = 0.0;
};

class HeadlessRenderDevice final : public RenderDevice {
public:
    void beginFrame(const Mat4&, int, int) override {}
    void submit(const RenderCommand&) override {}
    void endFrame() override {}
    TextureHandle createTexture(const TextureCreateInfo&) override { return {}; }
    void destroyTexture(TextureHandle) override {}
    void updateTextureRegion(TextureHandle, int, int, int, int, const TextureUploadData&) override {
    }
};
} // namespace

// Headless platform and cache implementations let the tests exercise the real
// Director scene-stack code without creating a window or render device.
mstd::unique_ptr<View> createDefaultView() { return mstd::make_unique<HeadlessView>(); }

mstd::unique_ptr<RenderDevice> createDefaultRenderDevice(View&) {
    return mstd::make_unique<HeadlessRenderDevice>();
}

FontAtlasCache::FontAtlasCache(Director& director) : _director(director) {}
FontAtlasCache::~FontAtlasCache() = default;
void FontAtlasCache::removeAllFontAtlas() {}

void FontCache::removeAllFonts() {}

void TextureCache::removeAllTextures(Director&) {}

} // namespace zocos

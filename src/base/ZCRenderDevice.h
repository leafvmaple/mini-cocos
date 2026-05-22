#pragma once

#include "base/ZCRenderCommand.h"

#include <cstdint>

namespace zocos {

enum class TextureFormat : std::uint8_t {
    RGBA8Unorm = 0,
    A8Unorm = 1,
};

enum class TextureDataOrigin : std::uint8_t {
    BottomLeft = 0,
    TopLeft = 1,
};

struct TextureUploadData {
    const unsigned char* pixels = nullptr;
    int rowPitchBytes = 0;
    TextureDataOrigin origin = TextureDataOrigin::TopLeft;
};

struct TextureCreateInfo {
    int width = 0;
    int height = 0;
    TextureFormat format = TextureFormat::RGBA8Unorm;
    TextureUploadData initialData{};
};

class RenderDevice {
public:
    virtual ~RenderDevice() = default;

    virtual void beginFrame(const Mat4& projection, int framebufferWidth,
                            int framebufferHeight) = 0;
    virtual void submit(const RenderCommand& command) = 0;
    virtual void endFrame() = 0;

    virtual TextureHandle createTexture(const TextureCreateInfo& createInfo) = 0;
    virtual void destroyTexture(TextureHandle texture) = 0;

    // Update a sub-rectangle of an existing texture without reallocating it.
    // (x, y) are in the same TopLeft pixel coordinate space the texture was
    // created with. `data.pixels` must point at the top-left pixel of the
    // source sub-rectangle; `data.rowPitchBytes` is the source stride between
    // consecutive rows (may be larger than width*bpp). On backends that
    // store textures Y-flipped (both GL and Vulkan in this engine), the
    // backend internally translates the y coordinate and reorders rows so
    // callers never need to think about origin differences.
    virtual void updateTextureRegion(TextureHandle texture, int x, int y, int width, int height,
                                     const TextureUploadData& data) = 0;
};

} // namespace zocos
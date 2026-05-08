#pragma once

#include "base/ZCRenderCommand.h"

#include <cstdint>

namespace zocos {

enum class TextureFormat : std::uint8_t {
    RGBA8Unorm = 0,
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

    virtual void beginFrame(const Mat4& projection, int framebufferWidth, int framebufferHeight) = 0;
    virtual void submit(const RenderCommand& command) = 0;
    virtual void endFrame() = 0;

    virtual TextureHandle createTexture(const TextureCreateInfo& createInfo) = 0;
    virtual void destroyTexture(TextureHandle texture) = 0;
};

} // namespace zocos
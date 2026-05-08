#pragma once

#include "base/ZCRenderCommand.h"

namespace zocos {

class RenderDevice {
public:
    virtual ~RenderDevice() = default;

    virtual void beginFrame(const Mat4& projection, int framebufferWidth, int framebufferHeight) = 0;
    virtual void submit(const RenderCommand& command) = 0;
    virtual void endFrame() = 0;

    virtual TextureHandle createTextureRGBA8(int width, int height, const unsigned char* pixels) = 0;
    virtual void destroyTexture(TextureHandle texture) = 0;
};

} // namespace zocos
#pragma once

#include "base/ZCRenderDevice.h"
#include "platform/opengl_loader.h"

#include <cstdint>
#include <unordered_map>

namespace zocos {

class RenderDeviceGL : public RenderDevice {
public:
    RenderDeviceGL() = default;
    ~RenderDeviceGL() override;

    void beginFrame(const Mat4& projection, int framebufferWidth, int framebufferHeight) override;
    void submit(const RenderCommand& command) override;
    void endFrame() override;

    TextureHandle createTexture(const TextureCreateInfo& createInfo) override;
    void destroyTexture(TextureHandle texture) override;

private:
    bool ensureSpritePipeline();
    void ensureSpriteGeometry();
    void drawSprite(const DrawSpriteCommand& command);
    GLuint getTextureId(TextureHandle texture) const;

    Mat4 _projection = Mat4::identity();

    GLuint _spriteProgram = 0;
    GLint _spriteLocMvp = -1;
    GLint _spriteLocTex = -1;
    GLuint _spriteVao = 0;
    GLuint _spriteVbo = 0;

    std::unordered_map<std::uint32_t, GLuint> _textures;
    std::uint32_t _nextTextureHandle = 1;
};

} // namespace zocos
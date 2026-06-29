#pragma once

#include "base/ZCRenderDevice.h"
#include "platform/opengl/ZCOpenGLLoader.h"

#include "base/ZCStd.h"

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
    void updateTextureRegion(TextureHandle texture, int x, int y, int width, int height,
                             const TextureUploadData& data) override;

private:
    struct GLTextureRecord {
        GLuint id = 0;
        int width = 0;
        int height = 0;
        int bytesPerPixel = 4;
        GLenum uploadFormat = 0;
    };

    struct GLPendingDraw {
        GLuint textureId = 0;
        GLint firstVertex = 0;
        GLsizei vertexCount = 0;
        Mat4 mvp = Mat4::identity();
    };

    bool ensureSpritePipeline();
    void ensureSpriteGeometry();
    void drawQuads(const RenderCommand& command);
    GLint appendVertices(const QuadVertex* vertices, mstd::size_t vertexCount, float opacity);
    void flushDrawCommands();
    GLuint getTextureId(TextureHandle texture) const;

    Mat4 _projection = Mat4::identity();

    GLuint _spriteProgram = 0;
    GLint _spriteLocMvp = -1;
    GLint _spriteLocTex = -1;
    GLuint _spriteVao = 0;
    GLuint _spriteVbo = 0;

    // Like the Vulkan backend: accumulate every batch's vertices into one
    // buffer for the frame and record per-batch draws, so the whole frame
    // uploads once instead of re-specifying the VBO per draw.
    mstd::vector<QuadVertex> _pendingVertices;
    mstd::vector<GLPendingDraw> _pendingDraws;

    mstd::unordered_map<mstd::uint32_t, GLTextureRecord> _textures;
    mstd::uint32_t _nextTextureHandle = 1;
};

} // namespace zocos
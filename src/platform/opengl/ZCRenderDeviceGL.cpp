#include "platform/opengl/ZCRenderDeviceGL.h"

#include "base/ZCStd.h"

namespace zocos {

namespace {

const char* kSpriteVs = R"(#version 330 core
layout (location = 0) in vec2 aPos;
layout (location = 1) in vec2 aUv;
layout (location = 2) in vec4 aColor;
uniform mat4 uMvp;
out vec2 vUv;
out vec4 vColor;
void main() {
    vUv = aUv;
    vColor = aColor;
    gl_Position = uMvp * vec4(aPos, 0.0, 1.0);
}
)";

const char* kSpriteFs = R"(#version 330 core
in vec2 vUv;
in vec4 vColor;
uniform sampler2D uTex;
out vec4 FragColor;
void main() {
    FragColor = vColor * texture(uTex, vUv);
}
)";

GLuint compileShader(GLenum type, const char* src) {
    const GLuint shader = glCreateShader(type);
    glShaderSource(shader, 1, &src, nullptr);
    glCompileShader(shader);

    GLint ok = 0;
    glGetShaderiv(shader, GL_COMPILE_STATUS, &ok);
    if (!ok) {
        char buf[512];
        glGetShaderInfoLog(shader, sizeof(buf), nullptr, buf);
        mstd::fprintf(stderr, "Shader compile error: %s\n", buf);
        glDeleteShader(shader);
        return 0;
    }
    return shader;
}

GLuint linkProgram(GLuint vs, GLuint fs) {
    const GLuint program = glCreateProgram();
    glAttachShader(program, vs);
    glAttachShader(program, fs);
    glLinkProgram(program);

    GLint ok = 0;
    glGetProgramiv(program, GL_LINK_STATUS, &ok);
    if (!ok) {
        char buf[512];
        glGetProgramInfoLog(program, sizeof(buf), nullptr, buf);
        mstd::fprintf(stderr, "Program link error: %s\n", buf);
        glDeleteProgram(program);
        return 0;
    }
    return program;
}

} // namespace

RenderDeviceGL::~RenderDeviceGL() {
    for (const auto& it : _textures) {
        const GLuint texId = it.second;
        if (texId) {
            glDeleteTextures(1, &texId);
        }
    }
    _textures.clear();

    if (_spriteVbo) {
        glDeleteBuffers(1, &_spriteVbo);
        _spriteVbo = 0;
    }
    if (_spriteVao) {
        glDeleteVertexArrays(1, &_spriteVao);
        _spriteVao = 0;
    }
    if (_spriteProgram) {
        glDeleteProgram(_spriteProgram);
        _spriteProgram = 0;
    }
}

void RenderDeviceGL::beginFrame(const Mat4& projection, int framebufferWidth,
                                int framebufferHeight) {
    _projection = projection;

    glViewport(0, 0, framebufferWidth, framebufferHeight);
    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
    glClearColor(0.12f, 0.12f, 0.15f, 1.f);
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
}

void RenderDeviceGL::submit(const RenderCommand& command) {
    switch (command.type) {
    case RenderCommandType::DrawSprite:
        drawSprite(command.sprite);
        break;
    case RenderCommandType::DrawQuads:
        drawQuads(command.quads);
        break;
    }
}

void RenderDeviceGL::endFrame() { glBindVertexArray(0); }

TextureHandle RenderDeviceGL::createTexture(const TextureCreateInfo& createInfo) {
    if (createInfo.width <= 0 || createInfo.height <= 0 || !createInfo.initialData.pixels) {
        return {};
    }

    int bytesPerPixel = 4;
    GLint glInternalFormat = GL_RGBA;
    GLenum glUploadFormat = GL_RGBA;
    switch (createInfo.format) {
    case TextureFormat::RGBA8Unorm:
        bytesPerPixel = 4;
        glInternalFormat = GL_RGBA;
        glUploadFormat = GL_RGBA;
        break;
    case TextureFormat::A8Unorm:
        bytesPerPixel = 1;
        glInternalFormat = GL_R8;
        glUploadFormat = GL_RED;
        break;
    default:
        return {};
    }

    const int tightRowPitch = createInfo.width * bytesPerPixel;
    const int srcRowPitch = createInfo.initialData.rowPitchBytes > 0
                                ? createInfo.initialData.rowPitchBytes
                                : tightRowPitch;
    if (srcRowPitch < tightRowPitch) {
        return {};
    }

    const unsigned char* uploadPixels = createInfo.initialData.pixels;
    mstd::vector<unsigned char> staging;
    const bool shouldFlipY = createInfo.initialData.origin == TextureDataOrigin::TopLeft;
    const bool shouldPackRows = srcRowPitch != tightRowPitch;
    if (shouldFlipY || shouldPackRows) {
        staging.resize(static_cast<size_t>(tightRowPitch * createInfo.height));
        for (int dstY = 0; dstY < createInfo.height; ++dstY) {
            int srcY = dstY;
            if (shouldFlipY) {
                srcY = (createInfo.height - 1) - dstY;
            }

            const auto* src =
                createInfo.initialData.pixels + static_cast<size_t>(srcY) * srcRowPitch;
            auto* dst = staging.data() + static_cast<size_t>(dstY) * tightRowPitch;
            mstd::memcpy(dst, src, static_cast<mstd::size_t>(tightRowPitch));
        }
        uploadPixels = staging.data();
    }

    GLuint tex = 0;
    glGenTextures(1, &tex);
    glBindTexture(GL_TEXTURE_2D, tex);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
    // A8 atlas: swizzle so sampling yields (1, 1, 1, r). Lets the shader use
    // a single `color * texture(...)` formula for both RGBA and A8 atlases.
    if (createInfo.format == TextureFormat::A8Unorm) {
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_SWIZZLE_R, GL_ONE);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_SWIZZLE_G, GL_ONE);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_SWIZZLE_B, GL_ONE);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_SWIZZLE_A, GL_RED);
    }
    glPixelStorei(GL_UNPACK_ALIGNMENT, 1);
    glTexImage2D(GL_TEXTURE_2D, 0, glInternalFormat, createInfo.width, createInfo.height, 0,
                 glUploadFormat, GL_UNSIGNED_BYTE, uploadPixels);
    glPixelStorei(GL_UNPACK_ALIGNMENT, 4);

    TextureHandle handle;
    handle.value = _nextTextureHandle++;
    GLTextureRecord rec;
    rec.id = tex;
    rec.width = createInfo.width;
    rec.height = createInfo.height;
    rec.bytesPerPixel = bytesPerPixel;
    rec.uploadFormat = glUploadFormat;
    _textures[handle.value] = rec;
    return handle;
}

void RenderDeviceGL::destroyTexture(TextureHandle texture) {
    if (!texture.isValid()) {
        return;
    }
    const auto it = _textures.find(texture.value);
    if (it == _textures.end()) {
        return;
    }
    const GLuint texId = it->second.id;
    if (texId) {
        glDeleteTextures(1, &texId);
    }
    _textures.erase(it);
}

void RenderDeviceGL::updateTextureRegion(TextureHandle texture, int x, int y, int width, int height,
                                         const TextureUploadData& data) {
    if (!texture.isValid() || width <= 0 || height <= 0 || !data.pixels) {
        return;
    }
    const auto it = _textures.find(texture.value);
    if (it == _textures.end()) {
        return;
    }
    const GLTextureRecord& rec = it->second;
    if (x < 0 || y < 0 || x + width > rec.width || y + height > rec.height) {
        return;
    }

    // Textures created with TopLeft origin are flipped row-wise so that GL's
    // bottom-left UV space matches the source layout (see createTexture).
    // Apply the same transform here so callers can stay in TopLeft pixel
    // coords. For BottomLeft sources the rows already line up with GL.
    const int bytesPerPixel = rec.bytesPerPixel;
    const int tightRowPitch = width * bytesPerPixel;
    const int srcRowPitch = data.rowPitchBytes > 0 ? data.rowPitchBytes : tightRowPitch;
    const bool shouldFlipY = data.origin == TextureDataOrigin::TopLeft;
    const int glY = shouldFlipY ? (rec.height - y - height) : y;

    mstd::vector<unsigned char> packed(static_cast<mstd::size_t>(tightRowPitch * height));
    for (int row = 0; row < height; ++row) {
        const int srcRow = shouldFlipY ? (height - 1 - row) : row;
        const auto* src = data.pixels + static_cast<mstd::size_t>(srcRow) * srcRowPitch;
        auto* dst = packed.data() + static_cast<mstd::size_t>(row) * tightRowPitch;
        mstd::memcpy(dst, src, static_cast<mstd::size_t>(tightRowPitch));
    }

    glBindTexture(GL_TEXTURE_2D, rec.id);
    glPixelStorei(GL_UNPACK_ALIGNMENT, 1);
    glTexSubImage2D(GL_TEXTURE_2D, 0, x, glY, width, height, rec.uploadFormat, GL_UNSIGNED_BYTE,
                    packed.data());
    glPixelStorei(GL_UNPACK_ALIGNMENT, 4);
}

bool RenderDeviceGL::ensureSpritePipeline() {
    if (_spriteProgram) {
        return true;
    }

    const GLuint vs = compileShader(GL_VERTEX_SHADER, kSpriteVs);
    const GLuint fs = compileShader(GL_FRAGMENT_SHADER, kSpriteFs);
    if (!vs || !fs) {
        if (vs) {
            glDeleteShader(vs);
        }
        if (fs) {
            glDeleteShader(fs);
        }
        return false;
    }

    _spriteProgram = linkProgram(vs, fs);
    glDeleteShader(vs);
    glDeleteShader(fs);
    if (!_spriteProgram) {
        return false;
    }

    _spriteLocMvp = glGetUniformLocation(_spriteProgram, "uMvp");
    _spriteLocTex = glGetUniformLocation(_spriteProgram, "uTex");
    return true;
}

void RenderDeviceGL::ensureSpriteGeometry() {
    if (_spriteVao) {
        return;
    }

    glGenVertexArrays(1, &_spriteVao);
    glGenBuffers(1, &_spriteVbo);
}

void RenderDeviceGL::drawSprite(const DrawSpriteCommand& command) {
    const GLuint texId = getTextureId(command.texture);
    if (!texId) {
        return;
    }

    const float w = command.contentSize.width;
    const float h = command.contentSize.height;
    const float u0 = command.uvRect.x;
    const float v0 = command.uvRect.y;
    const float u1 = command.uvRect.x + command.uvRect.width;
    const float v1 = command.uvRect.y + command.uvRect.height;

    const float clampedOpacity =
        command.opacity < 0.f ? 0.f : (command.opacity > 1.f ? 1.f : command.opacity);
    const Color4B col{255, 255, 255, static_cast<mstd::uint8_t>(clampedOpacity * 255.f + 0.5f)};
    const QuadVertex verts[] = {
        {{0.f, 0.f}, {u0, v0}, col}, {{w, 0.f}, {u1, v0}, col}, {{w, h}, {u1, v1}, col},
        {{0.f, 0.f}, {u0, v0}, col}, {{w, h}, {u1, v1}, col},   {{0.f, h}, {u0, v1}, col},
    };

    drawVertices(texId, command.world, verts, 6);
}

void RenderDeviceGL::drawQuads(const DrawQuadsCommand& command) {
    if (command.vertices.empty()) {
        return;
    }

    const GLuint texId = getTextureId(command.texture);
    if (!texId) {
        return;
    }

    drawVertices(texId, command.world, command.vertices.data(), command.vertices.size());
}

void RenderDeviceGL::drawVertices(GLuint textureId, const Mat4& world, const QuadVertex* vertices,
                                  mstd::size_t vertexCount) {
    if (!vertices || vertexCount == 0) {
        return;
    }
    if (!ensureSpritePipeline()) {
        return;
    }
    ensureSpriteGeometry();

    glBindVertexArray(_spriteVao);
    glBindBuffer(GL_ARRAY_BUFFER, _spriteVbo);
    glBufferData(GL_ARRAY_BUFFER, static_cast<GLsizeiptr>(vertexCount * sizeof(QuadVertex)),
                 vertices, GL_STATIC_DRAW);
    glEnableVertexAttribArray(0);
    glVertexAttribPointer(0, 2, GL_FLOAT, GL_FALSE, sizeof(QuadVertex), reinterpret_cast<void*>(0));
    glEnableVertexAttribArray(1);
    glVertexAttribPointer(1, 2, GL_FLOAT, GL_FALSE, sizeof(QuadVertex),
                          reinterpret_cast<void*>(sizeof(Vec2)));
    glEnableVertexAttribArray(2);
    glVertexAttribPointer(2, 4, GL_UNSIGNED_BYTE, GL_TRUE, sizeof(QuadVertex),
                          reinterpret_cast<void*>(sizeof(Vec2) * 2));

    const Mat4 mvp = _projection * world;
    glUseProgram(_spriteProgram);
    glUniformMatrix4fv(_spriteLocMvp, 1, GL_FALSE, mvp.data());
    glActiveTexture(GL_TEXTURE0);
    glBindTexture(GL_TEXTURE_2D, textureId);
    glUniform1i(_spriteLocTex, 0);
    glDrawArrays(GL_TRIANGLES, 0, static_cast<GLsizei>(vertexCount));
}

GLuint RenderDeviceGL::getTextureId(TextureHandle texture) const {
    const auto it = _textures.find(texture.value);
    if (it == _textures.end()) {
        return 0;
    }
    return it->second.id;
}

} // namespace zocos
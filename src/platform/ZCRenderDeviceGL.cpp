#include "platform/ZCRenderDeviceGL.h"

#include <cstdio>
#include <cstring>
#include <vector>

namespace zocos {

namespace {

const char* kSpriteVs = R"(#version 330 core
layout (location = 0) in vec2 aPos;
layout (location = 1) in vec2 aUv;
uniform mat4 uMvp;
out vec2 vUv;
void main() {
    vUv = aUv;
    gl_Position = uMvp * vec4(aPos, 0.0, 1.0);
}
)";

const char* kSpriteFs = R"(#version 330 core
in vec2 vUv;
uniform sampler2D uTex;
uniform float uOpacity;
out vec4 FragColor;
void main() {
    vec4 texColor = texture(uTex, vUv);
    FragColor = vec4(texColor.rgb, texColor.a * uOpacity);
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
        std::fprintf(stderr, "Shader compile error: %s\n", buf);
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
        std::fprintf(stderr, "Program link error: %s\n", buf);
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

void RenderDeviceGL::beginFrame(const Mat4& projection, int framebufferWidth, int framebufferHeight) {
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
    }
}

void RenderDeviceGL::endFrame() {
    glBindVertexArray(0);
}

TextureHandle RenderDeviceGL::createTexture(const TextureCreateInfo& createInfo) {
    if (createInfo.width <= 0 || createInfo.height <= 0 || !createInfo.initialData.pixels) {
        return {};
    }
    if (createInfo.format != TextureFormat::RGBA8Unorm) {
        return {};
    }

    constexpr int kBytesPerPixel = 4;
    const int tightRowPitch = createInfo.width * kBytesPerPixel;
    const int srcRowPitch = createInfo.initialData.rowPitchBytes > 0 ?
        createInfo.initialData.rowPitchBytes : tightRowPitch;
    if (srcRowPitch < tightRowPitch) {
        return {};
    }

    const unsigned char* uploadPixels = createInfo.initialData.pixels;
    std::vector<unsigned char> staging;
    const bool shouldFlipY = createInfo.initialData.origin == TextureDataOrigin::TopLeft;
    const bool shouldPackRows = srcRowPitch != tightRowPitch;
    if (shouldFlipY || shouldPackRows) {
        staging.resize(static_cast<size_t>(tightRowPitch * createInfo.height));
        for (int dstY = 0; dstY < createInfo.height; ++dstY) {
            int srcY = dstY;
            if (shouldFlipY) {
                srcY = (createInfo.height - 1) - dstY;
            }

            const auto* src = createInfo.initialData.pixels + static_cast<size_t>(srcY) * srcRowPitch;
            auto* dst = staging.data() + static_cast<size_t>(dstY) * tightRowPitch;
            std::memcpy(dst, src, static_cast<size_t>(tightRowPitch));
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
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, createInfo.width, createInfo.height, 0, GL_RGBA,
                 GL_UNSIGNED_BYTE, uploadPixels);

    TextureHandle handle;
    handle.value = _nextTextureHandle++;
    _textures[handle.value] = tex;
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
    const GLuint texId = it->second;
    if (texId) {
        glDeleteTextures(1, &texId);
    }
    _textures.erase(it);
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
    _spriteLocOpacity = glGetUniformLocation(_spriteProgram, "uOpacity");
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
    if (!ensureSpritePipeline()) {
        return;
    }
    ensureSpriteGeometry();

    const float w = command.contentSize.width;
    const float h = command.contentSize.height;
    const float verts[] = {
        0.f, 0.f, 0.f, 0.f,
        w, 0.f, 1.f, 0.f,
        w, h, 1.f, 1.f,
        0.f, 0.f, 0.f, 0.f,
        w, h, 1.f, 1.f,
        0.f, h, 0.f, 1.f,
    };

    glBindVertexArray(_spriteVao);
    glBindBuffer(GL_ARRAY_BUFFER, _spriteVbo);
    glBufferData(GL_ARRAY_BUFFER, sizeof(verts), verts, GL_STATIC_DRAW);
    glEnableVertexAttribArray(0);
    glVertexAttribPointer(0, 2, GL_FLOAT, GL_FALSE, 4 * sizeof(float), reinterpret_cast<void*>(0));
    glEnableVertexAttribArray(1);
    glVertexAttribPointer(1, 2, GL_FLOAT, GL_FALSE, 4 * sizeof(float), reinterpret_cast<void*>(8));

    const Mat4 mvp = _projection * command.world;
    glUseProgram(_spriteProgram);
    glUniformMatrix4fv(_spriteLocMvp, 1, GL_FALSE, mvp.data());
    glActiveTexture(GL_TEXTURE0);
    glBindTexture(GL_TEXTURE_2D, texId);
    glUniform1i(_spriteLocTex, 0);
    glUniform1f(_spriteLocOpacity, command.opacity);
    glDrawArrays(GL_TRIANGLES, 0, 6);
}

GLuint RenderDeviceGL::getTextureId(TextureHandle texture) const {
    const auto it = _textures.find(texture.value);
    if (it == _textures.end()) {
        return 0;
    }
    return it->second;
}

} // namespace zocos
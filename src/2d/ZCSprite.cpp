#include "2d/ZCSprite.h"

#include "base/ZCDirector.h"
#include "base/ZCRenderDevice.h"
#include "base/ZCRenderer.h"

#define STB_IMAGE_IMPLEMENTATION
#include <stb_image.h>

#include <cstdio>
#include <new>
#include <vector>

namespace zocos {

namespace {

bool uploadTexture(Director& director, TextureHandle& inOutTexture, int width, int height,
                   const unsigned char* pixels) {
    auto* device = director.getRenderDevice();
    if (!device) {
        std::fprintf(stderr, "Render device is not ready.\n");
        return false;
    }

    if (inOutTexture.isValid()) {
        device->destroyTexture(inOutTexture);
        inOutTexture = {};
    }

    TextureCreateInfo createInfo;
    createInfo.width = width;
    createInfo.height = height;
    createInfo.format = TextureFormat::RGBA8Unorm;
    createInfo.initialData.pixels = pixels;
    createInfo.initialData.rowPitchBytes = width * 4;
    createInfo.initialData.origin = TextureDataOrigin::TopLeft;

    const TextureHandle newTexture = device->createTexture(createInfo);
    if (!newTexture.isValid()) {
        std::fprintf(stderr, "Failed to create GPU texture.\n");
        return false;
    }

    inOutTexture = newTexture;
    return true;
}

} // namespace

Sprite::Sprite(Director& director) : _director(director) {
}

Sprite* Sprite::create(Director& director) {
    auto* sprite = new (std::nothrow) Sprite(director);
    if (sprite && sprite->init()) {
        sprite->autorelease();
        return sprite;
    }
    delete sprite;
    return nullptr;
}

Sprite* Sprite::createWithFile(Director& director, const char* path) {
    auto* sprite = create(director);
    if (!sprite) {
        return nullptr;
    }
    if (!sprite->initWithFile(path)) {
        return nullptr;
    }
    return sprite;
}

bool Sprite::init() {
    if (!Node::init()) {
        return false;
    }
    setContentSize({128.f, 128.f});
    return true;
}

Sprite::~Sprite() {
    auto* device = _director.getRenderDevice();
    if (device && _texture.isValid()) {
        device->destroyTexture(_texture);
        _texture = {};
    }
}

bool Sprite::initWithFile(const char* path) {
    int w = 0;
    int h = 0;
    int ch = 0;
    unsigned char* data = stbi_load(path, &w, &h, &ch, 4);
    if (!data) {
        std::fprintf(stderr, "stbi_load failed: %s\n", path);
        return false;
    }

    const bool uploaded = uploadTexture(_director, _texture, w, h, data);
    stbi_image_free(data);
    if (!uploaded) {
        return false;
    }

    setContentSize({static_cast<float>(w), static_cast<float>(h)});
    _ready = true;
    return true;
}

void Sprite::initWithCheckerboard() {
    constexpr int N = 64;
    std::vector<unsigned char> px(static_cast<size_t>(N * N * 4));
    for (int y = 0; y < N; ++y) {
        for (int x = 0; x < N; ++x) {
            const bool c = ((x / 8) + (y / 8)) % 2 == 0;
            const unsigned char v = c ? 240 : 60;
            const size_t i = static_cast<size_t>((y * N + x) * 4);
            px[i + 0] = v;
            px[i + 1] = static_cast<unsigned char>(255 - v);
            px[i + 2] = 160;
            px[i + 3] = 255;
        }
    }

    _ready = uploadTexture(_director, _texture, N, N, px.data());
    if (_ready) {
        setContentSize({static_cast<float>(N), static_cast<float>(N)});
    }
}

void Sprite::draw(Renderer& renderer, const Mat4& world) {
    if (!_ready || !_texture.isValid()) {
        return;
    }

    const RenderSortKey sortKey = makeRenderSortKey(0, 0, _texture.value);
    renderer.addDrawSprite(world, _contentSize, _texture, getOpacity(), sortKey);
}

} // namespace zocos

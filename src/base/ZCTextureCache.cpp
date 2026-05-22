#include "base/ZCTextureCache.h"

#include "base/ZCDirector.h"
#include "platform/ZCFileUtils.h"
#include "base/ZCRenderDevice.h"

#define STB_IMAGE_IMPLEMENTATION
#include <stb_image.h>

#include <cstdio>
#include "base/ZCStd.h"

namespace zocos {

bool TextureCache::acquireFromFile(Director& director, const char* path, TextureHandle& outTexture,
                                   Size& outPixelSize) {
    if (!path || path[0] == '\0') {
        return false;
    }

    const mstd::string key = mstd::string("file://") + path;
    const auto it = _entriesByKey.find(key);
    if (it != _entriesByKey.end()) {
        it->second.refCount += 1;
        outTexture = it->second.texture;
        outPixelSize = it->second.pixelSize;
        return true;
    }

    int width = 0;
    int height = 0;
    int channels = 0;
    mstd::vector<unsigned char> encodedBytes;
    if (!FileUtils::getInstance().getDataFromFile(path, encodedBytes) || encodedBytes.empty()) {
        std::fprintf(stderr, "readBinaryFile failed: %s\n", path);
        return false;
    }

    if (encodedBytes.size() > static_cast<mstd::size_t>(mstd::numeric_limits<int>::max())) {
        std::fprintf(stderr, "Image file is too large: %s\n", path);
        return false;
    }

    unsigned char* data = stbi_load_from_memory(
        encodedBytes.data(), static_cast<int>(encodedBytes.size()), &width, &height, &channels, 4);
    if (!data) {
        std::fprintf(stderr, "stbi_load_from_memory failed: %s\n", path);
        return false;
    }

    const bool ok = uploadFromPixels(director, key, width, height, data, width * 4,
                                     TextureDataOrigin::TopLeft, outTexture, outPixelSize);
    stbi_image_free(data);
    return ok;
}

bool TextureCache::acquireCheckerboard(Director& director, int size, TextureHandle& outTexture,
                                       Size& outPixelSize) {
    const int dimension = size > 0 ? size : 64;
    const mstd::string key = "builtin://checkerboard/" + mstd::to_string(dimension);

    const auto it = _entriesByKey.find(key);
    if (it != _entriesByKey.end()) {
        it->second.refCount += 1;
        outTexture = it->second.texture;
        outPixelSize = it->second.pixelSize;
        return true;
    }

    mstd::vector<unsigned char> pixels(static_cast<mstd::size_t>(dimension * dimension * 4));
    for (int y = 0; y < dimension; ++y) {
        for (int x = 0; x < dimension; ++x) {
            const bool checker = ((x / 8) + (y / 8)) % 2 == 0;
            const unsigned char base = checker ? 220 : 70;
            const unsigned char r = static_cast<unsigned char>((base + x * 3) % 256);
            const unsigned char g = static_cast<unsigned char>((base + y * 5) % 256);
            const unsigned char b = static_cast<unsigned char>(checker ? 180 : 110);

            const mstd::size_t index = static_cast<mstd::size_t>((y * dimension + x) * 4);
            pixels[index + 0] = r;
            pixels[index + 1] = g;
            pixels[index + 2] = b;
            pixels[index + 3] = 255;
        }
    }

    return uploadFromPixels(director, key, dimension, dimension, pixels.data(), dimension * 4,
                            TextureDataOrigin::TopLeft, outTexture, outPixelSize);
}

void TextureCache::release(Director& director, TextureHandle texture) {
    if (!texture.isValid()) {
        return;
    }

    Entry* entry = findEntryByTexture(texture);
    if (!entry) {
        return;
    }

    entry->refCount -= 1;
    if (entry->refCount > 0) {
        return;
    }

    auto keyIt = _keyByTexture.find(texture.value);
    if (keyIt == _keyByTexture.end()) {
        return;
    }

    if (auto* device = director.getRenderDevice()) {
        device->destroyTexture(texture);
    }

    _entriesByKey.erase(keyIt->second);
    _keyByTexture.erase(keyIt);
}

void TextureCache::removeUnusedTextures(Director& director) {
    for (auto it = _entriesByKey.begin(); it != _entriesByKey.end();) {
        if (it->second.refCount > 0) {
            ++it;
            continue;
        }

        const TextureHandle texture = it->second.texture;
        if (texture.isValid()) {
            if (auto* device = director.getRenderDevice()) {
                device->destroyTexture(texture);
            }
            _keyByTexture.erase(texture.value);
        }

        it = _entriesByKey.erase(it);
    }
}

void TextureCache::removeAllTextures(Director& director) {
    if (auto* device = director.getRenderDevice()) {
        for (const auto& kv : _entriesByKey) {
            const TextureHandle texture = kv.second.texture;
            if (texture.isValid()) {
                device->destroyTexture(texture);
            }
        }
    }

    _entriesByKey.clear();
    _keyByTexture.clear();
}

bool TextureCache::uploadFromPixels(Director& director, const mstd::string& key, int width, int height,
                                    const unsigned char* pixels, int rowPitchBytes,
                                    TextureDataOrigin origin, TextureHandle& outTexture,
                                    Size& outPixelSize) {
    auto* device = director.getRenderDevice();
    if (!device) {
        std::fprintf(stderr, "Render device is not ready.\n");
        return false;
    }

    TextureCreateInfo createInfo;
    createInfo.width = width;
    createInfo.height = height;
    createInfo.format = TextureFormat::RGBA8Unorm;
    createInfo.initialData.pixels = pixels;
    createInfo.initialData.rowPitchBytes = rowPitchBytes;
    createInfo.initialData.origin = origin;

    const TextureHandle texture = device->createTexture(createInfo);
    if (!texture.isValid()) {
        std::fprintf(stderr, "Failed to create GPU texture for key: %s\n", key.c_str());
        return false;
    }

    Entry entry;
    entry.texture = texture;
    entry.pixelSize = {static_cast<float>(width), static_cast<float>(height)};
    entry.refCount = 1;

    _entriesByKey[key] = entry;
    _keyByTexture[texture.value] = key;

    outTexture = texture;
    outPixelSize = entry.pixelSize;
    return true;
}

TextureCache::Entry* TextureCache::findEntryByTexture(TextureHandle texture) {
    const auto keyIt = _keyByTexture.find(texture.value);
    if (keyIt == _keyByTexture.end()) {
        return nullptr;
    }

    const auto entryIt = _entriesByKey.find(keyIt->second);
    if (entryIt == _entriesByKey.end()) {
        _keyByTexture.erase(keyIt);
        return nullptr;
    }

    return &entryIt->second;
}

} // namespace zocos

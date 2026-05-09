#pragma once

#include "base/ZCRenderDevice.h"
#include "math/ZCMath.h"

#include <cstdint>
#include <string>
#include <unordered_map>

namespace zocos {

class Director;

class TextureCache {
public:
    bool acquireFromFile(Director& director, const char* path, TextureHandle& outTexture,
                         Size& outPixelSize);
    bool acquireCheckerboard(Director& director, int size, TextureHandle& outTexture,
                             Size& outPixelSize);

    void release(Director& director, TextureHandle texture);
    void removeUnusedTextures(Director& director);
    void removeAllTextures(Director& director);

    std::size_t getCachedTextureCount() const { return _entriesByKey.size(); }

private:
    struct Entry {
        TextureHandle texture{};
        Size pixelSize{};
        int refCount = 0;
    };

    bool uploadFromPixels(Director& director, const std::string& key, int width, int height,
                          const unsigned char* pixels, int rowPitchBytes,
                          TextureDataOrigin origin, TextureHandle& outTexture,
                          Size& outPixelSize);

    Entry* findEntryByTexture(TextureHandle texture);

    std::unordered_map<std::string, Entry> _entriesByKey;
    std::unordered_map<std::uint32_t, std::string> _keyByTexture;
};

} // namespace zocos

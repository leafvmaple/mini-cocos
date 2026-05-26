#pragma once

#include "base/ZCRenderDevice.h"
#include "math/ZCMath.h"

#include "base/ZCStd.h"

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

    mstd::size_t getCachedTextureCount() const { return _entriesByKey.size(); }

private:
    struct Entry {
        TextureHandle texture{};
        Size pixelSize{};
        int refCount = 0;
    };

    bool uploadFromPixels(Director& director, const mstd::string& key, int width, int height,
                          const unsigned char* pixels, int rowPitchBytes,
                          TextureDataOrigin origin, TextureHandle& outTexture,
                          Size& outPixelSize);

    Entry* findEntryByTexture(TextureHandle texture);

    mstd::unordered_map<mstd::string, Entry> _entriesByKey;
    mstd::unordered_map<mstd::uint32_t, mstd::string> _keyByTexture;
};

} // namespace zocos

#include "base/ZCFontAtlasCache.h"

#include "base/ZCFontAtlas.h"

#include <cstdio>

namespace zocos {

FontAtlasCache::FontAtlasCache(Director& director) : _director(director) {}

FontAtlasCache::~FontAtlasCache() { removeAllFontAtlas(); }

mstd::string FontAtlasCache::makeKey(const mstd::string& fontPath, float fontSize) {
    char buf[32];
    std::snprintf(buf, sizeof(buf), "|%.2f", fontSize);
    return fontPath + buf;
}

FontAtlas* FontAtlasCache::getFontAtlasTTF(const mstd::string& fontPath, float fontSize) {
    const mstd::string key = makeKey(fontPath, fontSize);
    auto it = _atlases.find(key);
    if (it != _atlases.end()) {
        return it->second;
    }
    auto* atlas = FontAtlas::create(_director, fontPath, fontSize);
    if (!atlas) {
        return nullptr;
    }
    atlas->retain();
    _atlases.emplace(key, atlas);
    return atlas;
}

void FontAtlasCache::removeAllFontAtlas() {
    for (auto& kv : _atlases) {
        kv.second->release();
    }
    _atlases.clear();
}

} // namespace zocos

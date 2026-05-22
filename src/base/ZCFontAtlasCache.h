#pragma once

#include <string>
#include <unordered_map>

namespace zocos {

class Director;
class FontAtlas;

// Cocos2d-x style FontAtlasCache: a process-wide store of FontAtlas instances
// keyed by (fontPath, fontSize). The cache owns one retain on every entry so
// that closing a Label does not destroy the atlas — the next Label that
// requests the same TTF + size immediately re-uses the warmed glyph cache.
//
// Lifetime contract:
//   * `getFontAtlasTTF` returns the cached pointer without bumping the
//     refcount for the caller. The Label takes its own retain inside
//     `setFontAtlas`, so the standard call chain
//         label->setFontAtlas(cache.getFontAtlasTTF(...))
//     ends with refcount = cache(1) + label(1).
//   * `removeAllFontAtlas` releases the cache's retain on every entry. It
//     is invoked from Director::shutdown so any FontAtlas still referenced
//     by Labels survives until those Labels release it.
class FontAtlasCache {
public:
    explicit FontAtlasCache(Director& director);
    ~FontAtlasCache();

    FontAtlasCache(const FontAtlasCache&) = delete;
    FontAtlasCache& operator=(const FontAtlasCache&) = delete;

    FontAtlas* getFontAtlasTTF(const std::string& fontPath, float fontSize);

    void removeAllFontAtlas();

private:
    static std::string makeKey(const std::string& fontPath, float fontSize);

    Director& _director;
    std::unordered_map<std::string, FontAtlas*> _atlases;
};

} // namespace zocos

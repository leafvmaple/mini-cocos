#include "base/ZCFontCache.h"

#include "base/ZCStd.h"

namespace zocos {

bool FontCache::acquireFromFile(const mstd::string& path, Font*& outFont) {
    const mstd::string resolvedPath = Font::resolveFontPath(path);
    if (resolvedPath.empty()) {
        return false;
    }

    auto it = _entriesByKey.find(resolvedPath);
    if (it != _entriesByKey.end()) {
        it->second.refCount += 1;
        outFont = &it->second.font;
        return true;
    }

    Entry entry;
    if (!entry.font.loadFromFile(resolvedPath)) {
        return false;
    }
    entry.refCount = 1;

    auto [insertedIt, inserted] = _entriesByKey.emplace(resolvedPath, mstd::move(entry));
    if (!inserted) {
        return false;
    }

    Font* cachedFont = &insertedIt->second.font;
    _keyByFont[cachedFont] = path;
    outFont = cachedFont;
    return true;
}

void FontCache::release(Font* font) {
    if (!font) {
        return;
    }

    const auto keyIt = _keyByFont.find(font);
    if (keyIt == _keyByFont.end()) {
        return;
    }

    const auto entryIt = _entriesByKey.find(keyIt->second);
    if (entryIt == _entriesByKey.end()) {
        _keyByFont.erase(keyIt);
        return;
    }

    entryIt->second.refCount -= 1;
    if (entryIt->second.refCount > 0) {
        return;
    }

    _entriesByKey.erase(entryIt);
    _keyByFont.erase(keyIt);
}

void FontCache::removeUnusedFonts() {
    for (auto it = _entriesByKey.begin(); it != _entriesByKey.end();) {
        if (it->second.refCount > 0) {
            ++it;
            continue;
        }

        _keyByFont.erase(&it->second.font);
        it = _entriesByKey.erase(it);
    }
}

void FontCache::removeAllFonts() {
    _entriesByKey.clear();
    _keyByFont.clear();
}

} // namespace zocos

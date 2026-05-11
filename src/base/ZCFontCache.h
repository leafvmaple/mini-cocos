#pragma once

#include "base/ZCFont.h"

#include <cstddef>
#include <string>
#include <unordered_map>

namespace zocos {

class FontCache {
public:
    bool acquireFromFile(const std::string& path, Font*& outFont);
    void release(Font* font);

    void removeUnusedFonts();
    void removeAllFonts();

    std::size_t getCachedFontCount() const { return _entriesByKey.size(); }

private:
    struct Entry {
        Font font;
        int refCount = 0;
    };

    std::unordered_map<std::string, Entry> _entriesByKey;
    std::unordered_map<const Font*, std::string> _keyByFont;
};

} // namespace zocos

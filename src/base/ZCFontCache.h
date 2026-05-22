#pragma once

#include "base/ZCFont.h"

#include <cstddef>
#include "base/ZCStd.h"

namespace zocos {

class FontCache {
public:
    bool acquireFromFile(const mstd::string& path, Font*& outFont);
    void release(Font* font);

    void removeUnusedFonts();
    void removeAllFonts();

    mstd::size_t getCachedFontCount() const { return _entriesByKey.size(); }

private:
    struct Entry {
        Font font;
        int refCount = 0;
    };

    mstd::unordered_map<mstd::string, Entry> _entriesByKey;
    mstd::unordered_map<const Font*, mstd::string> _keyByFont;
};

} // namespace zocos

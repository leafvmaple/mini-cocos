#include "base/ZCFont.h"

#include <stb_truetype.h>

#include <fstream>
#include <iterator>
#include <utility>

namespace zocos {

bool Font::loadFromFile(const std::string& path) {
    if (path.empty()) {
        return false;
    }

    std::ifstream file(path, std::ios::binary);
    if (!file) {
        return false;
    }

    std::vector<unsigned char> bytes((std::istreambuf_iterator<char>(file)),
                                     std::istreambuf_iterator<char>());
    if (bytes.empty()) {
        return false;
    }

    const int offset = stbtt_GetFontOffsetForIndex(bytes.data(), 0);
    if (offset < 0) {
        return false;
    }

    stbtt_fontinfo fontInfo{};
    if (stbtt_InitFont(&fontInfo, bytes.data(), offset) == 0) {
        return false;
    }

    _path = path;
    _data = std::move(bytes);
    _fontOffset = offset;
    return true;
}

bool Font::isValid() const {
    return !_data.empty() && _fontOffset >= 0;
}

const unsigned char* Font::getData() const {
    return _data.empty() ? nullptr : _data.data();
}

} // namespace zocos

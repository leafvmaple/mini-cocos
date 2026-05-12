#include "base/ZCFont.h"
#include "platform/ZCFileUtils.h"

#include <stb_truetype.h>

#include <utility>

namespace zocos {

bool Font::loadFromFile(const std::string& path) {
    std::vector<unsigned char> bytes;
    if (!FileUtils::getInstance().getDataFromFile(path, bytes) || bytes.empty()) {
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

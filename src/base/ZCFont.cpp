#include "base/ZCFont.h"
#include "platform/ZCFileUtils.h"

#include <stb_truetype.h>

#include "base/ZCStd.h"

namespace zocos {

const mstd::vector<mstd::string>& Font::getDefaultFontCandidates() {
    static const mstd::vector<mstd::string> kDefaultCandidates = {
        "fonts/NotoSansSC-Regular.otf",
        "fonts/NotoSans-Regular.ttf",
        "fonts/NotoSerif-Regular.ttf",
    };
    return kDefaultCandidates;
}

mstd::string Font::resolveFontPath(const mstd::string& preferredPath) {
    auto& fileUtils = FileUtils::getInstance();

    if (!preferredPath.empty()) {
        const mstd::string fullPath = fileUtils.fullPathForFilename(preferredPath);
        if (!fullPath.empty()) {
            return fullPath;
        }
    }

    for (const mstd::string& candidate : getDefaultFontCandidates()) {
        const mstd::string fullPath = fileUtils.fullPathForFilename(candidate);
        if (!fullPath.empty()) {
            return fullPath;
        }
    }

    return {};
}

bool Font::loadFromFile(const mstd::string& path) {
    const mstd::string resolvedPath = resolveFontPath(path);
    if (resolvedPath.empty()) {
        return false;
    }

    mstd::vector<unsigned char> bytes;
    if (!FileUtils::getInstance().getDataFromFile(resolvedPath, bytes) || bytes.empty()) {
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

    _path = resolvedPath;
    _data = mstd::move(bytes);
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

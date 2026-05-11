#include "base/ZCFontAtlas.h"

#include "base/ZCDirector.h"
#include "base/ZCFont.h"
#include "base/ZCFontCache.h"

#include <stb_truetype.h>

#include <algorithm>
#include <cmath>
#include <new>

namespace zocos {

FontAtlas::FontAtlas(Director& director) : _director(director) {
}

FontAtlas* FontAtlas::create(Director& director, const std::string& fontPath, float fontSize) {
    auto* atlas = new (std::nothrow) FontAtlas(director);
    if (atlas && atlas->init(fontPath, fontSize)) {
        atlas->autorelease();
        return atlas;
    }
    delete atlas;
    return nullptr;
}

FontAtlas::~FontAtlas() {
    releaseFont();
}

bool FontAtlas::init(const std::string& fontPath, float fontSize) {
    if (fontPath.empty()) {
        return false;
    }

    Font* font = nullptr;
    if (!_director.getFontCache().acquireFromFile(fontPath, font)) {
        return false;
    }

    stbtt_fontinfo fontInfo{};
    if (!font || !font->isValid() || !font->getData()
        || stbtt_InitFont(&fontInfo, font->getData(), font->getFontOffset()) == 0) {
        _director.getFontCache().release(font);
        return false;
    }

    releaseFont();

    _font = font;
    _fontPath = fontPath;
    _fontSize = std::max(1.f, fontSize);
    _scale = stbtt_ScaleForPixelHeight(&fontInfo, _fontSize);

    stbtt_GetFontVMetrics(&fontInfo, &_ascent, &_descent, &_lineGap);
    _baseline = static_cast<int>(std::ceil(static_cast<float>(_ascent) * _scale));
    _lineHeight = std::max(
        1, static_cast<int>(std::ceil(static_cast<float>(_ascent - _descent + _lineGap) * _scale)));

    return true;
}

bool FontAtlas::isValid() const {
    return _font != nullptr && _font->isValid() && _font->getData() != nullptr;
}

void FontAtlas::releaseFont() {
    if (!_font) {
        return;
    }

    _director.getFontCache().release(_font);
    _font = nullptr;
}

} // namespace zocos

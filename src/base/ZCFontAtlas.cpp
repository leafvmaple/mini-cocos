#include "base/ZCFontAtlas.h"

#include "base/ZCDirector.h"
#include "base/ZCFont.h"
#include "base/ZCFontCache.h"
#include "base/ZCRenderDevice.h"

#define STB_TRUETYPE_IMPLEMENTATION
#include <stb_truetype.h>

#include <algorithm>
#include <cmath>
#include <new>

namespace zocos {

namespace {

constexpr int kFallbackCodepoint = '?';
constexpr int kAtlasPadding = 1;
constexpr int kAtlasWidth = 1024;
constexpr int kAtlasHeight = 1024;

bool initFontInfo(Font* font, stbtt_fontinfo& outFontInfo) {
    return font && font->isValid() && font->getData() &&
           stbtt_InitFont(&outFontInfo, font->getData(), font->getFontOffset()) != 0;
}

} // namespace

FontAtlas::FontAtlas(Director& director) : _director(director) {}

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
    releaseAtlasTexture();
    releaseFont();
}

bool FontAtlas::init(const std::string& fontPath, float fontSize) {
    Font* font = nullptr;
    if (!_director.getFontCache().acquireFromFile(fontPath, font)) {
        return false;
    }

    stbtt_fontinfo fontInfo{};
    if (!initFontInfo(font, fontInfo)) {
        _director.getFontCache().release(font);
        return false;
    }

    releaseAtlasTexture();
    releaseFont();

    _font = font;
    _fontPath = _font->getPath();
    _fontSize = std::max(1.f, fontSize);
    _scale = stbtt_ScaleForPixelHeight(&fontInfo, _fontSize);

    stbtt_GetFontVMetrics(&fontInfo, &_ascent, &_descent, &_lineGap);
    _baseline = static_cast<int>(std::ceil(static_cast<float>(_ascent) * _scale));
    _lineHeight = std::max(
        1, static_cast<int>(std::ceil(static_cast<float>(_ascent - _descent + _lineGap) * _scale)));

    _glyphs.clear();
    _atlasWidth = kAtlasWidth;
    _atlasHeight = kAtlasHeight;
    _packCursorX = 0;
    _packCursorY = 0;
    _packRowHeight = 0;
    _atlasPixels.assign(static_cast<std::size_t>(_atlasWidth * _atlasHeight * 4), 0);
    _atlasDirty = true;

    FontGlyph fallbackGlyph;
    if (!getGlyph(kFallbackCodepoint, fallbackGlyph)) {
        releaseAtlasTexture();
        releaseFont();
        return false;
    }

    for (int cp = 32; cp <= 126; ++cp) {
        FontGlyph glyph;
        getGlyph(cp, glyph);
    }

    if (!commitAtlasTexture()) {
        releaseAtlasTexture();
        releaseFont();
        return false;
    }

    return true;
}

bool FontAtlas::isValid() const {
    return _font != nullptr && _font->isValid() && _font->getData() != nullptr &&
           _atlasTexture.isValid();
}

bool FontAtlas::getGlyph(int codepoint, FontGlyph& outGlyph) {
    const int normalizedCodepoint =
        (codepoint >= 0 && codepoint <= 0x10FFFF) ? codepoint : kFallbackCodepoint;

    const auto cached = _glyphs.find(normalizedCodepoint);
    if (cached != _glyphs.end()) {
        outGlyph = cached->second;
        return true;
    }

    FontGlyph glyph;
    if (!addGlyphToAtlas(normalizedCodepoint, glyph)) {
        if (normalizedCodepoint == kFallbackCodepoint) {
            return false;
        }
        return getGlyph(kFallbackCodepoint, outGlyph);
    }

    _glyphs.emplace(normalizedCodepoint, glyph);
    outGlyph = glyph;
    return true;
}

int FontAtlas::getKerning(int lhsCodepoint, int rhsCodepoint) const {
    if (lhsCodepoint < 0 || rhsCodepoint < 0 || lhsCodepoint > 0x10FFFF ||
        rhsCodepoint > 0x10FFFF) {
        return 0;
    }

    stbtt_fontinfo fontInfo{};
    if (!initFontInfo(_font, fontInfo)) {
        return 0;
    }

    return stbtt_GetCodepointKernAdvance(&fontInfo, lhsCodepoint, rhsCodepoint);
}

bool FontAtlas::commitAtlasTexture() {
    if (_atlasTexture.isValid() && !_atlasDirty) {
        return true;
    }
    return uploadAtlasTexture();
}

bool FontAtlas::addGlyphToAtlas(int codepoint, FontGlyph& outGlyph) {
    stbtt_fontinfo fontInfo{};
    if (!initFontInfo(_font, fontInfo)) {
        return false;
    }

    FontGlyph glyph;
    glyph.codepoint = codepoint;

    int advance = 0;
    int leftSideBearing = 0;
    stbtt_GetCodepointHMetrics(&fontInfo, codepoint, &advance, &leftSideBearing);
    (void)leftSideBearing;
    glyph.advance = advance;

    int x0 = 0;
    int y0 = 0;
    int x1 = 0;
    int y1 = 0;
    stbtt_GetCodepointBitmapBox(&fontInfo, codepoint, _scale, _scale, &x0, &y0, &x1, &y1);
    glyph.bearingX = x0;
    glyph.bearingY = y0;
    glyph.width = x1 - x0;
    glyph.height = y1 - y0;

    if (glyph.width > 0 && glyph.height > 0) {
        int atlasX = 0;
        int atlasY = 0;
        if (!allocGlyphRect(glyph.width, glyph.height, atlasX, atlasY)) {
            return false;
        }

        std::vector<unsigned char> alpha(static_cast<std::size_t>(glyph.width * glyph.height));
        stbtt_MakeCodepointBitmap(&fontInfo, alpha.data(), glyph.width, glyph.height, glyph.width,
                                  _scale, _scale, codepoint);

        for (int y = 0; y < glyph.height; ++y) {
            for (int x = 0; x < glyph.width; ++x) {
                const std::size_t srcIndex = static_cast<std::size_t>(y * glyph.width + x);
                const std::size_t dstIndex =
                    static_cast<std::size_t>(((atlasY + y) * _atlasWidth + (atlasX + x)) * 4);
                _atlasPixels[dstIndex + 0] = 255;
                _atlasPixels[dstIndex + 1] = 255;
                _atlasPixels[dstIndex + 2] = 255;
                _atlasPixels[dstIndex + 3] = alpha[srcIndex];
            }
        }

        glyph.uvRect = toUvRectTopLeft(atlasX, atlasY, glyph.width, glyph.height);
    }

    _atlasDirty = true;
    outGlyph = glyph;
    return true;
}

bool FontAtlas::allocGlyphRect(int glyphWidth, int glyphHeight, int& outPixelX, int& outPixelY) {
    if (glyphWidth <= 0 || glyphHeight <= 0) {
        return false;
    }

    const int paddedWidth = glyphWidth + kAtlasPadding * 2;
    const int paddedHeight = glyphHeight + kAtlasPadding * 2;
    if (paddedWidth > _atlasWidth || paddedHeight > _atlasHeight) {
        return false;
    }

    if (_packCursorX + paddedWidth > _atlasWidth) {
        _packCursorX = 0;
        _packCursorY += _packRowHeight;
        _packRowHeight = 0;
    }

    if (_packCursorY + paddedHeight > _atlasHeight) {
        return false;
    }

    outPixelX = _packCursorX + kAtlasPadding;
    outPixelY = _packCursorY + kAtlasPadding;

    _packCursorX += paddedWidth;
    _packRowHeight = std::max(_packRowHeight, paddedHeight);
    return true;
}

bool FontAtlas::uploadAtlasTexture() {
    if (_atlasPixels.empty() || _atlasWidth <= 0 || _atlasHeight <= 0) {
        return false;
    }

    auto* device = _director.getRenderDevice();
    if (!device) {
        return false;
    }

    TextureCreateInfo createInfo;
    createInfo.width = _atlasWidth;
    createInfo.height = _atlasHeight;
    createInfo.format = TextureFormat::RGBA8Unorm;
    createInfo.initialData.pixels = _atlasPixels.data();
    createInfo.initialData.rowPitchBytes = _atlasWidth * 4;
    createInfo.initialData.origin = TextureDataOrigin::TopLeft;

    const TextureHandle newTexture = device->createTexture(createInfo);
    if (!newTexture.isValid()) {
        return false;
    }

    if (_atlasTexture.isValid()) {
        device->destroyTexture(_atlasTexture);
    }

    _atlasTexture = newTexture;
    _atlasDirty = false;
    _atlasVersion += 1;
    return true;
}

Rect FontAtlas::toUvRectTopLeft(int x, int y, int width, int height) const {
    if (width <= 0 || height <= 0 || _atlasWidth <= 0 || _atlasHeight <= 0) {
        return Rect{0.f, 0.f, 0.f, 0.f};
    }

    const float invW = 1.f / static_cast<float>(_atlasWidth);
    const float invH = 1.f / static_cast<float>(_atlasHeight);

    Rect uv;
    uv.x = static_cast<float>(x) * invW;
    uv.y = static_cast<float>(_atlasHeight - (y + height)) * invH;
    uv.width = static_cast<float>(width) * invW;
    uv.height = static_cast<float>(height) * invH;
    return uv;
}

void FontAtlas::releaseAtlasTexture() {
    if (_atlasTexture.isValid()) {
        if (auto* device = _director.getRenderDevice()) {
            device->destroyTexture(_atlasTexture);
        }
        _atlasTexture = {};
    }

    _atlasVersion = 0;
    _atlasDirty = !_atlasPixels.empty();
}

void FontAtlas::releaseFont() {
    if (!_font) {
        return;
    }

    _director.getFontCache().release(_font);
    _font = nullptr;
}

} // namespace zocos

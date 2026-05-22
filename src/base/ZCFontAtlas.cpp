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

constexpr char32_t kFallbackCodepoint = U'?';
constexpr int kAtlasPadding = 1;
constexpr int kAtlasWidth = 1024;
constexpr int kAtlasHeight = 1024;

// ASCII glyphs cached eagerly so the very first frame already has data on
// the GPU and small Labels never trigger a re-upload during draw.
constexpr char32_t kAsciiPrewarmFirst = 0x20;
constexpr char32_t kAsciiPrewarmLast = 0x7E;

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
    releasePages();
    releaseFont();
}

bool FontAtlas::init(const std::string& fontPath, float fontSize) {
    Font* font = nullptr;
    if (!_director.getFontCache().acquireFromFile(fontPath, font)) {
        return false;
    }

    auto fontInfo = std::make_unique<stbtt_fontinfo>();
    if (!font->isValid() || !font->getData() ||
        stbtt_InitFont(fontInfo.get(), font->getData(), font->getFontOffset()) == 0) {
        _director.getFontCache().release(font);
        return false;
    }

    releasePages();
    releaseFont();

    _font = font;
    _fontInfo = std::move(fontInfo);
    _fontPath = _font->getPath();
    _fontSize = std::max(1.f, fontSize);
    _scale = stbtt_ScaleForPixelHeight(_fontInfo.get(), _fontSize);

    int ascent = 0;
    int descent = 0;
    int lineGap = 0;
    stbtt_GetFontVMetrics(_fontInfo.get(), &ascent, &descent, &lineGap);
    _fontAscender = static_cast<float>(ascent) * _scale;
    _fontDescender = static_cast<float>(descent) * _scale;
    _lineHeight = std::max(1.f, static_cast<float>(ascent - descent + lineGap) * _scale);

    _letterDefinitions.clear();
    _atlasWidth = kAtlasWidth;
    _atlasHeight = kAtlasHeight;

    if (addNewPage() < 0) {
        releaseFont();
        return false;
    }

    // Pre-warm ASCII so the first Label draw does not pay the rasterise cost.
    std::u32string ascii;
    ascii.reserve(kAsciiPrewarmLast - kAsciiPrewarmFirst + 1);
    for (char32_t cp = kAsciiPrewarmFirst; cp <= kAsciiPrewarmLast; ++cp) {
        ascii.push_back(cp);
    }
    if (!prepareLetterDefinitions(ascii)) {
        releasePages();
        releaseFont();
        return false;
    }

    return true;
}

bool FontAtlas::prepareLetterDefinitions(const std::u32string& utf32Text) {
    for (char32_t cp : utf32Text) {
        if (_letterDefinitions.find(cp) != _letterDefinitions.end()) {
            continue;
        }
        LetterDefinition def;
        if (!prepareLetterDefinition(cp, def)) {
            return false;
        }
        _letterDefinitions.emplace(cp, def);
    }
    if (!commitDirtyPages()) {
        return false;
    }
    return true;
}

bool FontAtlas::findLetterDefinitionForChar(char32_t utf32Char, LetterDefinition& outDef) {
    auto it = _letterDefinitions.find(utf32Char);
    if (it != _letterDefinitions.end()) {
        outDef = it->second;
        return outDef.validDefinition;
    }

    LetterDefinition def;
    if (!prepareLetterDefinition(utf32Char, def)) {
        if (utf32Char == kFallbackCodepoint) {
            return false;
        }
        commitDirtyPages();
        return findLetterDefinitionForChar(kFallbackCodepoint, outDef);
    }

    _letterDefinitions.emplace(utf32Char, def);
    if (!commitDirtyPages()) {
        return false;
    }
    outDef = def;
    return def.validDefinition;
}

float FontAtlas::getHorizontalKerningForChars(char32_t first, char32_t second) const {
    if (!_fontInfo) {
        return 0.f;
    }
    const int raw = stbtt_GetCodepointKernAdvance(_fontInfo.get(), static_cast<int>(first),
                                                  static_cast<int>(second));
    return static_cast<float>(raw) * _scale;
}

bool FontAtlas::prepareLetterDefinition(char32_t utf32Char, LetterDefinition& outDef) {
    if (!_fontInfo) {
        return false;
    }

    LetterDefinition def;
    def.utf32Char = utf32Char;

    int advance = 0;
    int leftSideBearing = 0;
    stbtt_GetCodepointHMetrics(_fontInfo.get(), static_cast<int>(utf32Char), &advance,
                               &leftSideBearing);
    def.xAdvance = advance;

    int x0 = 0;
    int y0 = 0;
    int x1 = 0;
    int y1 = 0;
    stbtt_GetCodepointBitmapBox(_fontInfo.get(), static_cast<int>(utf32Char), _scale, _scale, &x0,
                                &y0, &x1, &y1);
    const int bitmapW = x1 - x0;
    const int bitmapH = y1 - y0;
    def.width = static_cast<float>(bitmapW);
    def.height = static_cast<float>(bitmapH);
    def.offsetX = static_cast<float>(x0);
    def.offsetY = _fontAscender + static_cast<float>(y0);
    assert(bitmapW >= 0 && bitmapH >= 0);

    int pageIndex = 0;
    int atlasX = 0;
    int atlasY = 0;
    if (!allocGlyphRect(bitmapW, bitmapH, pageIndex, atlasX, atlasY)) {
        return false;
    }

    AtlasPage& page = _atlasPages[static_cast<std::size_t>(pageIndex)];
    std::vector<unsigned char> alpha(static_cast<std::size_t>(bitmapW * bitmapH));
    stbtt_MakeCodepointBitmap(_fontInfo.get(), alpha.data(), bitmapW, bitmapH, bitmapW, _scale,
                              _scale, static_cast<int>(utf32Char));

    for (int y = 0; y < bitmapH; ++y) {
        for (int x = 0; x < bitmapW; ++x) {
            const std::size_t srcIndex = static_cast<std::size_t>(y * bitmapW + x);
            const std::size_t dstIndex =
                static_cast<std::size_t>(((atlasY + y) * _atlasWidth + (atlasX + x)) * 4);
            page.pixels[dstIndex + 0] = 255;
            page.pixels[dstIndex + 1] = 255;
            page.pixels[dstIndex + 2] = 255;
            page.pixels[dstIndex + 3] = alpha[srcIndex];
        }
    }

    page.dirty = true;
    def.U = static_cast<float>(atlasX);
    def.V = static_cast<float>(atlasY);
    def.textureID = pageIndex;
    def.validDefinition = true;

    outDef = def;
    return true;
}

bool FontAtlas::allocGlyphRect(int glyphWidth, int glyphHeight, int& outPageIndex, int& outPixelX,
                               int& outPixelY) {
    const int paddedWidth = glyphWidth + kAtlasPadding * 2;
    const int paddedHeight = glyphHeight + kAtlasPadding * 2;
    assert(paddedWidth <= _atlasWidth && paddedHeight <= _atlasHeight);

    for (std::size_t i = 0; i < _atlasPages.size(); ++i) {
        AtlasPage& page = _atlasPages[i];
        int cursorX = page.packCursorX;
        int cursorY = page.packCursorY;
        int rowHeight = page.packRowHeight;
        if (cursorX + paddedWidth > _atlasWidth) {
            cursorX = 0;
            cursorY += rowHeight;
            rowHeight = 0;
        }
        if (cursorY + paddedHeight > _atlasHeight) {
            continue;
        }
        outPageIndex = static_cast<int>(i);
        outPixelX = cursorX + kAtlasPadding;
        outPixelY = cursorY + kAtlasPadding;
        page.packCursorX = cursorX + paddedWidth;
        page.packCursorY = cursorY;
        page.packRowHeight = std::max(rowHeight, paddedHeight);
        return true;
    }

    // No existing page has room: spin up a new page and place the glyph in
    // the top-left corner of it.
    const int newPage = addNewPage();
    if (newPage < 0) {
        return false;
    }
    AtlasPage& page = _atlasPages[static_cast<std::size_t>(newPage)];
    outPageIndex = newPage;
    outPixelX = kAtlasPadding;
    outPixelY = kAtlasPadding;
    page.packCursorX = paddedWidth;
    page.packCursorY = 0;
    page.packRowHeight = paddedHeight;
    return true;
}

int FontAtlas::addNewPage() {
    AtlasPage page;
    page.pixels.assign(static_cast<std::size_t>(_atlasWidth * _atlasHeight * 4), 0);
    page.dirty = true;
    _atlasPages.push_back(std::move(page));
    _atlasTextures.emplace_back();
    return static_cast<int>(_atlasPages.size()) - 1;
}

bool FontAtlas::commitDirtyPages() {
    bool anyUploaded = false;
    for (std::size_t i = 0; i < _atlasPages.size(); ++i) {
        AtlasPage& page = _atlasPages[i];
        if (!page.dirty && page.texture.isValid()) {
            continue;
        }
        if (!uploadPage(page)) {
            return false;
        }
        _atlasTextures[i] = page.texture;
        anyUploaded = true;
    }
    if (anyUploaded) {
        _atlasVersion += 1;
    }
    return true;
}

bool FontAtlas::uploadPage(AtlasPage& page) {
    auto* device = _director.getRenderDevice();
    if (!device) {
        return false;
    }

    TextureCreateInfo createInfo;
    createInfo.width = _atlasWidth;
    createInfo.height = _atlasHeight;
    createInfo.format = TextureFormat::RGBA8Unorm;
    createInfo.initialData.pixels = page.pixels.data();
    createInfo.initialData.rowPitchBytes = _atlasWidth * 4;
    createInfo.initialData.origin = TextureDataOrigin::TopLeft;

    const TextureHandle newTexture = device->createTexture(createInfo);
    if (!newTexture.isValid()) {
        return false;
    }
    if (page.texture.isValid()) {
        device->destroyTexture(page.texture);
    }
    page.texture = newTexture;
    page.dirty = false;
    return true;
}

void FontAtlas::releasePages() {
    auto* device = _director.getRenderDevice();
    for (auto& page : _atlasPages) {
        if (device && page.texture.isValid()) {
            device->destroyTexture(page.texture);
        }
    }
    _atlasPages.clear();
    _atlasTextures.clear();
    _atlasVersion = 0;
}

void FontAtlas::releaseFont() {
    _director.getFontCache().release(_font);
    _font = nullptr;
    _fontInfo.reset();
}

} // namespace zocos

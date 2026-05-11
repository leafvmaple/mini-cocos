#include "2d/ZCLabel.h"

#include "base/ZCDirector.h"
#include "base/ZCFontAtlas.h"
#include "base/ZCFont.h"
#include "base/ZCRenderDevice.h"
#include "base/ZCRenderer.h"

#define STB_TRUETYPE_IMPLEMENTATION
#include <stb_truetype.h>

#include <algorithm>
#include <cmath>
#include <cstdio>
#include <new>
#include <utility>
#include <vector>

namespace zocos {

namespace {

constexpr const char* kDefaultFontCandidates[] = {
    "fonts/NotoSansSC-Regular.otf",
    "fonts/NotoSans-Regular.ttf",
    "fonts/NotoSerif-Regular.ttf",
};

std::vector<int> decodeUtf8(const std::string& text) {
    std::vector<int> codepoints;
    codepoints.reserve(text.size());

    const auto pushReplacement = [&codepoints]() {
        codepoints.push_back('?');
    };

    std::size_t i = 0;
    while (i < text.size()) {
        const unsigned char c0 = static_cast<unsigned char>(text[i]);
        if (c0 < 0x80U) {
            codepoints.push_back(static_cast<int>(c0));
            ++i;
            continue;
        }

        if ((c0 & 0xE0U) == 0xC0U) {
            if (i + 1 >= text.size()) {
                pushReplacement();
                break;
            }
            const unsigned char c1 = static_cast<unsigned char>(text[i + 1]);
            if ((c1 & 0xC0U) != 0x80U) {
                pushReplacement();
                ++i;
                continue;
            }
            const int cp = ((static_cast<int>(c0 & 0x1FU) << 6) | static_cast<int>(c1 & 0x3FU));
            codepoints.push_back(cp >= 0x80 ? cp : '?');
            i += 2;
            continue;
        }

        if ((c0 & 0xF0U) == 0xE0U) {
            if (i + 2 >= text.size()) {
                pushReplacement();
                break;
            }
            const unsigned char c1 = static_cast<unsigned char>(text[i + 1]);
            const unsigned char c2 = static_cast<unsigned char>(text[i + 2]);
            if ((c1 & 0xC0U) != 0x80U || (c2 & 0xC0U) != 0x80U) {
                pushReplacement();
                ++i;
                continue;
            }
            const int cp = (static_cast<int>(c0 & 0x0FU) << 12)
                | (static_cast<int>(c1 & 0x3FU) << 6) | static_cast<int>(c2 & 0x3FU);
            codepoints.push_back(cp >= 0x800 ? cp : '?');
            i += 3;
            continue;
        }

        if ((c0 & 0xF8U) == 0xF0U) {
            if (i + 3 >= text.size()) {
                pushReplacement();
                break;
            }
            const unsigned char c1 = static_cast<unsigned char>(text[i + 1]);
            const unsigned char c2 = static_cast<unsigned char>(text[i + 2]);
            const unsigned char c3 = static_cast<unsigned char>(text[i + 3]);
            if ((c1 & 0xC0U) != 0x80U || (c2 & 0xC0U) != 0x80U || (c3 & 0xC0U) != 0x80U) {
                pushReplacement();
                ++i;
                continue;
            }
            const int cp = (static_cast<int>(c0 & 0x07U) << 18)
                | (static_cast<int>(c1 & 0x3FU) << 12)
                | (static_cast<int>(c2 & 0x3FU) << 6)
                | static_cast<int>(c3 & 0x3FU);
            codepoints.push_back((cp >= 0x10000 && cp <= 0x10FFFF) ? cp : '?');
            i += 4;
            continue;
        }

        pushReplacement();
        ++i;
    }

    return codepoints;
}

void destroyTexture(Director& director, TextureHandle& inOutTexture) {
    auto* device = director.getRenderDevice();
    if (!device || !inOutTexture.isValid()) {
        return;
    }
    device->destroyTexture(inOutTexture);
    inOutTexture = {};
}

} // namespace

Label::Label(Director& director) : _director(director) {
}

Label* Label::create(Director& director, const std::string& text, const std::string& fontPath,
                     float fontSize) {
    auto* label = new (std::nothrow) Label(director);
    if (label && label->init()) {
        label->setFontSize(fontSize);
        if (!fontPath.empty()) {
            label->setTTF(fontPath);
        } else {
            label->ensureFontAtlas();
        }
        label->setString(text);
        label->autorelease();
        return label;
    }
    delete label;
    return nullptr;
}

Label* Label::createWithTTF(Director& director, const std::string& text,
                            const std::string& fontPath, float fontSize) {
    return create(director, text, fontPath, fontSize);
}

Label::~Label() {
    if (_fontAtlas) {
        _fontAtlas->release();
        _fontAtlas = nullptr;
    }
    destroyTexture(_director, _texture);
}

bool Label::init() {
    if (!Node::init()) {
        return false;
    }
    setContentSize({1.f, _fontSize});
    return true;
}

void Label::setString(const std::string& text) {
    if (_text == text) {
        return;
    }
    _text = text;
    _dirty = true;
}

bool Label::setFontAtlas(FontAtlas* fontAtlas) {
    if (!fontAtlas || !fontAtlas->isValid()) {
        return false;
    }

    if (_fontAtlas == fontAtlas) {
        return true;
    }

    if (_fontAtlas) {
        _fontAtlas->release();
    }

    _fontAtlas = fontAtlas;
    _fontAtlas->retain();

    _fontPath = _fontAtlas->getFontPath();
    _fontSize = _fontAtlas->getFontSize();
    _dirty = true;
    return true;
}

bool Label::setTTF(const std::string& fontPath) {
    if (fontPath.empty()) {
        return ensureFontAtlas();
    }

    if (_fontAtlas && _fontPath == fontPath && std::fabs(_fontSize - _fontAtlas->getFontSize()) <= 1e-4f) {
        return true;
    }

    auto* atlas = FontAtlas::create(_director, fontPath, _fontSize);
    if (!atlas) {
        return false;
    }

    return setFontAtlas(atlas);
}

void Label::setFontSize(float fontSize) {
    const float clampedSize = std::max(1.f, fontSize);
    if (std::fabs(_fontSize - clampedSize) <= 1e-4f) {
        return;
    }

    _fontSize = clampedSize;
    if (!_fontPath.empty()) {
        auto* atlas = FontAtlas::create(_director, _fontPath, _fontSize);
        if (atlas) {
            setFontAtlas(atlas);
            return;
        }
    }

    _dirty = true;
}

bool Label::ensureFontAtlas() {
    if (_fontAtlas && _fontAtlas->isValid()) {
        return true;
    }

    if (!_fontPath.empty()) {
        auto* atlas = FontAtlas::create(_director, _fontPath, _fontSize);
        if (atlas) {
            return setFontAtlas(atlas);
        }
    }

    for (const char* candidate : kDefaultFontCandidates) {
        if (!candidate) {
            continue;
        }
        auto* atlas = FontAtlas::create(_director, candidate, _fontSize);
        if (atlas && setFontAtlas(atlas)) {
            return true;
        }
    }

    return false;
}

bool Label::rebuildTexture() {
    auto* device = _director.getRenderDevice();
    if (!device) {
        _ready = false;
        return false;
    }

    if (!ensureFontAtlas()) {
        _ready = false;
        return false;
    }

    if (!_fontAtlas || !_fontAtlas->isValid()) {
        _ready = false;
        return false;
    }

    Font* font = _fontAtlas->getFont();
    if (!font || !font->isValid() || !font->getData()) {
        _ready = false;
        return false;
    }

    stbtt_fontinfo fontInfo{};
    if (stbtt_InitFont(&fontInfo, font->getData(), font->getFontOffset()) == 0) {
        _ready = false;
        return false;
    }

    const std::string view = _text.empty() ? std::string(" ") : _text;
    const std::vector<int> cps = decodeUtf8(view);

    std::vector<std::vector<int>> lines;
    lines.emplace_back();
    for (int cp : cps) {
        if (cp == '\r') {
            continue;
        }
        if (cp == '\n') {
            lines.emplace_back();
            continue;
        }
        lines.back().push_back(cp);
    }

    const float scale = _fontAtlas->getScale();
    const int baseline = _fontAtlas->getBaseline();
    const int lineHeight = _fontAtlas->getLineHeight();

    float maxWidthF = 0.f;
    for (const auto& line : lines) {
        float lineWidthF = 0.f;
        for (std::size_t i = 0; i < line.size(); ++i) {
            int advance = 0;
            int leftSideBearing = 0;
            stbtt_GetCodepointHMetrics(&fontInfo, line[i], &advance, &leftSideBearing);
            (void)leftSideBearing;
            lineWidthF += static_cast<float>(advance) * scale;
            if (i + 1 < line.size()) {
                lineWidthF += static_cast<float>(
                    stbtt_GetCodepointKernAdvance(&fontInfo, line[i], line[i + 1]))
                    * scale;
            }
        }
        maxWidthF = std::max(maxWidthF, lineWidthF);
    }

    const int width = std::max(1, static_cast<int>(std::ceil(maxWidthF)));
    const int height = std::max(1, lineHeight * static_cast<int>(lines.size()));
    std::vector<unsigned char> pixels(static_cast<std::size_t>(width * height * 4), 0);

    for (std::size_t lineIndex = 0; lineIndex < lines.size(); ++lineIndex) {
        float penX = 0.f;
        const int baselineY = static_cast<int>(lineIndex) * lineHeight + baseline;
        const auto& line = lines[lineIndex];

        for (std::size_t i = 0; i < line.size(); ++i) {
            const int cp = line[i];

            int x0 = 0;
            int y0 = 0;
            int x1 = 0;
            int y1 = 0;
            stbtt_GetCodepointBitmapBox(&fontInfo, cp, scale, scale, &x0, &y0, &x1, &y1);
            const int glyphWidth = x1 - x0;
            const int glyphHeight = y1 - y0;

            if (glyphWidth > 0 && glyphHeight > 0) {
                std::vector<unsigned char> glyph(static_cast<std::size_t>(glyphWidth * glyphHeight));
                stbtt_MakeCodepointBitmap(&fontInfo, glyph.data(), glyphWidth, glyphHeight,
                                          glyphWidth, scale, scale, cp);

                const int dstXBase = static_cast<int>(std::floor(penX)) + x0;
                const int dstYBase = baselineY + y0;
                for (int gy = 0; gy < glyphHeight; ++gy) {
                    for (int gx = 0; gx < glyphWidth; ++gx) {
                        const unsigned char alpha = glyph[static_cast<std::size_t>(gy * glyphWidth + gx)];
                        if (alpha == 0) {
                            continue;
                        }

                        const int dstX = dstXBase + gx;
                        const int dstY = dstYBase + gy;
                        if (dstX < 0 || dstX >= width || dstY < 0 || dstY >= height) {
                            continue;
                        }

                        const std::size_t dstIndex = static_cast<std::size_t>((dstY * width + dstX) * 4);
                        pixels[dstIndex + 0] = 255;
                        pixels[dstIndex + 1] = 255;
                        pixels[dstIndex + 2] = 255;
                        pixels[dstIndex + 3] = std::max(pixels[dstIndex + 3], alpha);
                    }
                }
            }

            int advance = 0;
            int leftSideBearing = 0;
            stbtt_GetCodepointHMetrics(&fontInfo, cp, &advance, &leftSideBearing);
            (void)leftSideBearing;
            penX += static_cast<float>(advance) * scale;
            if (i + 1 < line.size()) {
                penX += static_cast<float>(
                    stbtt_GetCodepointKernAdvance(&fontInfo, cp, line[i + 1]))
                    * scale;
            }
        }
    }

    destroyTexture(_director, _texture);
    TextureCreateInfo createInfo;
    createInfo.width = width;
    createInfo.height = height;
    createInfo.format = TextureFormat::RGBA8Unorm;
    createInfo.initialData.pixels = pixels.data();
    createInfo.initialData.rowPitchBytes = width * 4;
    createInfo.initialData.origin = TextureDataOrigin::TopLeft;
    _texture = device->createTexture(createInfo);
    _ready = _texture.isValid();
    _dirty = false;
    setContentSize({static_cast<float>(width), static_cast<float>(height)});
    return _ready;
}

void Label::draw(Renderer& renderer, const Mat4& world) {
    if (_dirty && !rebuildTexture()) {
        return;
    }
    if (!_ready || !_texture.isValid()) {
        return;
    }

    const RenderSortKey sortKey = makeRenderSortKey(0, 0, _texture.value);
    renderer.addDrawSprite(world, _contentSize, _texture, getOpacity(), sortKey);
}

} // namespace zocos
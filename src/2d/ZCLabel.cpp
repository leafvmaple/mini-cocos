#include "2d/ZCLabel.h"

#include "base/ZCDirector.h"
#include "base/ZCFontAtlas.h"
#include "base/ZCRenderer.h"
#include "base/ZCStringUtils.h"

#include <algorithm>
#include <cmath>
#include <new>
#include <vector>

namespace zocos {

Label::Label(Director& director) : _director(director) {}

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
    _atlasVersion = 0;
    _vertices.clear();
    _dirty = true;
    return true;
}

bool Label::setTTF(const std::string& fontPath) {
    if (fontPath.empty()) {
        return ensureFontAtlas();
    }

    if (_fontAtlas && _fontPath == fontPath &&
        std::fabs(_fontSize - _fontAtlas->getFontSize()) <= 1e-4f) {
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

    auto* atlas = FontAtlas::create(_director, _fontPath, _fontSize);
    if (atlas) {
        return setFontAtlas(atlas);
    }

    return false;
}

bool Label::updateContent() {
    if (!ensureFontAtlas()) {
        _ready = false;
        return false;
    }

    if (!_fontAtlas || !_fontAtlas->isValid()) {
        _ready = false;
        return false;
    }

    const std::string view = _text.empty() ? std::string(" ") : _text;
    const std::vector<int> cps = StringUtils::decodeUtf8(view);

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

    _vertices.clear();
    _vertices.reserve(cps.size() * 6);

    const float scale = _fontAtlas->getScale();
    const int baseline = _fontAtlas->getBaseline();
    const int lineHeight = _fontAtlas->getLineHeight();
    const float totalHeight =
        static_cast<float>(std::max(1, lineHeight * static_cast<int>(lines.size())));

    float maxWidthF = 0.f;
    for (std::size_t lineIndex = 0; lineIndex < lines.size(); ++lineIndex) {
        float penX = 0.f;
        int prevCodepoint = 0;
        bool hasPrev = false;
        const auto& line = lines[lineIndex];

        for (int cp : line) {
            FontGlyph glyph;
            if (!_fontAtlas->getGlyph(cp, glyph)) {
                continue;
            }

            if (hasPrev) {
                penX += static_cast<float>(_fontAtlas->getKerning(prevCodepoint, cp)) * scale;
            }

            if (glyph.width > 0 && glyph.height > 0) {
                const float x0 = penX + static_cast<float>(glyph.bearingX);
                const float baselineY =
                    totalHeight - static_cast<float>(lineIndex * lineHeight + baseline);
                const float glyphTopY = baselineY - static_cast<float>(glyph.bearingY);
                const float y0 = glyphTopY - static_cast<float>(glyph.height);
                const float x1 = x0 + static_cast<float>(glyph.width);
                const float y1 = glyphTopY;

                const float u0 = glyph.uvRect.x;
                const float v0 = glyph.uvRect.y;
                const float u1 = glyph.uvRect.x + glyph.uvRect.width;
                const float v1 = glyph.uvRect.y + glyph.uvRect.height;

                _vertices.push_back({{x0, y0}, {u0, v0}});
                _vertices.push_back({{x1, y0}, {u1, v0}});
                _vertices.push_back({{x1, y1}, {u1, v1}});
                _vertices.push_back({{x0, y0}, {u0, v0}});
                _vertices.push_back({{x1, y1}, {u1, v1}});
                _vertices.push_back({{x0, y1}, {u0, v1}});
            }

            penX += static_cast<float>(glyph.advance) * scale;
            prevCodepoint = cp;
            hasPrev = true;
        }

        maxWidthF = std::max(maxWidthF, penX);
    }

    if (!_fontAtlas->commitAtlasTexture()) {
        _vertices.clear();
        _ready = false;
        return false;
    }

    _atlasVersion = _fontAtlas->getAtlasVersion();
    const int width = std::max(1, static_cast<int>(std::ceil(maxWidthF)));
    const int height = static_cast<int>(totalHeight);
    _ready = true;
    _dirty = false;
    setContentSize({static_cast<float>(width), static_cast<float>(height)});
    return true;
}

void Label::draw(Renderer& renderer, const Mat4& world) {
    if (_fontAtlas && _atlasVersion != _fontAtlas->getAtlasVersion()) {
        _dirty = true;
    }

    if (_dirty && !updateContent()) {
        return;
    }
    if (!_ready || !_fontAtlas || !_fontAtlas->isValid() || _vertices.empty()) {
        return;
    }

    const TextureHandle atlasTexture = _fontAtlas->getAtlasTexture();
    if (!atlasTexture.isValid()) {
        return;
    }

    const RenderSortKey sortKey = makeRenderSortKey(0, 0, atlasTexture.value);
    renderer.addDrawQuads(world, atlasTexture, _vertices, getOpacity(), sortKey);
}

} // namespace zocos
#include "2d/ZCLabel.h"

#include "base/ZCDirector.h"
#include "base/ZCFontAtlas.h"
#include "base/ZCFontAtlasCache.h"
#include "base/ZCRenderer.h"
#include "base/ZCStringUtils.h"

#include <algorithm>
#include <cmath>
#include <new>

namespace zocos {

Label::Label(Director& director) : _director(director) {}

Label* Label::createWithTTF(Director& director, const std::string& text,
                            const std::string& fontPath, float fontSize) {
    auto* label = new (std::nothrow) Label(director);
    if (label && label->init()) {
        label->_fontSize = std::max(1.f, fontSize);
        auto* atlas = director.getFontAtlasCache().getFontAtlasTTF(fontPath, label->_fontSize);
        if (atlas && label->setFontAtlas(atlas)) {
            label->setString(text);
            label->autorelease();
            return label;
        }
    }
    delete label;
    return nullptr;
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
    StringUtils::UTF8ToUTF32(_text, _utf32Text);
    _contentDirty = true;
}

bool Label::setFontAtlas(FontAtlas* fontAtlas) {
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
    resetLayoutState();
    _contentDirty = true;
    return true;
}

bool Label::setTTF(const std::string& fontPath) {
    if (_fontAtlas && _fontPath == fontPath &&
        std::fabs(_fontSize - _fontAtlas->getFontSize()) <= 1e-4f) {
        return true;
    }

    auto* atlas = _director.getFontAtlasCache().getFontAtlasTTF(fontPath, _fontSize);
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

    auto* atlas = _director.getFontAtlasCache().getFontAtlasTTF(_fontPath, _fontSize);
    if (atlas) {
        setFontAtlas(atlas);
    }
}

void Label::setMaxLineWidth(float maxLineWidth) {
    const float clamped = std::max(0.f, maxLineWidth);
    if (std::fabs(_maxLineWidth - clamped) <= 1e-4f) {
        return;
    }
    _maxLineWidth = clamped;
    _contentDirty = true;
}

void Label::resetLayoutState() {
    _lettersInfo.clear();
    _horizontalKernings.clear();
    _quadsPerPage.clear();
    _contentWidth = 0.f;
    _contentHeight = 0.f;
    _ready = false;
}

bool Label::updateContent() {
    resetLayoutState();

    if (!_fontAtlas || !_fontAtlas->prepareLetterDefinitions(_utf32Text)) {
        return false;
    }

    if (_utf32Text.empty()) {
        _contentWidth = 1.f;
        _contentHeight = _fontAtlas->getLineHeight();
        setContentSize({_contentWidth, _contentHeight});
        _atlasVersion = _fontAtlas->getAtlasVersion();
        _ready = true;
        _contentDirty = false;
        return true;
    }

    computeHorizontalKernings();
    multilineTextWrap();
    alignText();
    updateQuads();

    _atlasVersion = _fontAtlas->getAtlasVersion();
    setContentSize({_contentWidth, _contentHeight});
    _ready = true;
    _contentDirty = false;
    return true;
}

void Label::computeHorizontalKernings() {
    _horizontalKernings.assign(_utf32Text.size(), 0.f);
    for (std::size_t i = 1; i < _utf32Text.size(); ++i) {
        _horizontalKernings[i] =
            _fontAtlas->getHorizontalKerningForChars(_utf32Text[i - 1], _utf32Text[i]);
    }
}

void Label::multilineTextWrap() {
    _lettersInfo.assign(_utf32Text.size(), LetterInfo{});

    const float lineHeight = _fontAtlas->getLineHeight();
    const float scale = _fontAtlas->getScale();
    const bool wrapByWidth = (_maxLineWidth > 0.f);

    int lineIndex = 0;
    float penX = 0.f;
    float maxLineWidth = 0.f;

    for (std::size_t i = 0; i < _utf32Text.size(); ++i) {
        const char32_t cp = _utf32Text[i];

        if (cp == U'\r') {
            recordLetterInfo(i, cp, penX, 0.f, 0, lineIndex);
            continue;
        }
        if (cp == U'\n') {
            recordLetterInfo(i, cp, penX, 0.f, 0, lineIndex);
            maxLineWidth = std::max(maxLineWidth, penX);
            ++lineIndex;
            penX = 0.f;
            continue;
        }

        LetterDefinition def;
        if (!_fontAtlas->findLetterDefinitionForChar(cp, def) || !def.validDefinition) {
            recordLetterInfo(i, cp, penX, 0.f, 0, lineIndex);
            continue;
        }

        const float kerning = _horizontalKernings[i];
        const float advance = static_cast<float>(def.xAdvance) * scale;

        // Soft wrap when the next glyph would exceed _maxLineWidth. We only
        // wrap when the current line already has content so a single glyph
        // wider than the limit still gets placed (cocos2d-x falls back to
        // char-break in that case too).
        if (wrapByWidth && penX > 0.f && (penX + kerning + advance) > _maxLineWidth) {
            maxLineWidth = std::max(maxLineWidth, penX);
            ++lineIndex;
            penX = 0.f;
        }

        penX += (penX > 0.f) ? kerning : 0.f;

        // Position stored is the top-left of the glyph quad in label-local
        // coords with the line's top sitting at y = lineIndex * lineHeight.
        // alignText() will translate Y into Y-up label space below.
        const float glyphX = penX + def.offsetX;
        const float glyphY = static_cast<float>(lineIndex) * lineHeight + def.offsetY;

        const bool hasGlyph = (def.width > 0.f && def.height > 0.f);
        recordLetterInfo(i, cp, glyphX, glyphY, def.textureID, lineIndex, hasGlyph);

        penX += advance;
    }

    maxLineWidth = std::max(maxLineWidth, penX);
    _contentWidth = std::max(1.f, std::ceil(maxLineWidth));
    _contentHeight = std::max(lineHeight, static_cast<float>(lineIndex + 1) * lineHeight);
}

void Label::alignText() {
    // Mini implementation: left + top alignment only. multilineTextWrap()
    // already wrote `positionY` as a downward distance from the content top;
    // here we convert it to label-local Y-up coordinates (origin bottom-left).
    for (LetterInfo& info : _lettersInfo) {
        info.positionY = _contentHeight - info.positionY;
    }
}

void Label::recordLetterInfo(std::size_t letterIndex, char32_t utf32Char, float positionX,
                             float positionY, int atlasIndex, int lineIndex, bool valid) {
    LetterInfo& info = _lettersInfo[letterIndex];
    info.utf32Char = utf32Char;
    info.valid = valid;
    info.positionX = positionX;
    info.positionY = positionY;
    info.atlasIndex = atlasIndex;
    info.lineIndex = lineIndex;
}

void Label::updateQuads() {
    const int pageCount = _fontAtlas->getAtlasPageCount();
    _quadsPerPage.assign(static_cast<std::size_t>(pageCount), {});

    const float atlasW = static_cast<float>(_fontAtlas->getAtlasWidth());
    const float atlasH = static_cast<float>(_fontAtlas->getAtlasHeight());
    const float invW = (atlasW > 0.f) ? 1.f / atlasW : 0.f;
    const float invH = (atlasH > 0.f) ? 1.f / atlasH : 0.f;

    for (const auto& info : _lettersInfo) {
        if (!info.valid) {
            continue;
        }
        LetterDefinition def;
        if (!_fontAtlas->findLetterDefinitionForChar(info.utf32Char, def) || !def.validDefinition) {
            continue;
        }

        const float xL = info.positionX;
        const float yT = info.positionY;
        const float xR = xL + def.width;
        const float yB = yT - def.height;

        // GL UVs: v=0 is bottom of texture. LetterDefinition.U/V are top-left
        // atlas pixel coordinates, so the top edge maps to v = 1 - V/atlasH
        // and the bottom edge to v = 1 - (V+height)/atlasH.
        const float uL = def.U * invW;
        const float uR = (def.U + def.width) * invW;
        const float vT = 1.f - def.V * invH;
        const float vB = 1.f - (def.V + def.height) * invH;

        auto& quads = _quadsPerPage[static_cast<std::size_t>(def.textureID)];
        const Color4B color{255, 255, 255, 255};
        quads.push_back({{xL, yB}, {uL, vB}, color});
        quads.push_back({{xR, yB}, {uR, vB}, color});
        quads.push_back({{xR, yT}, {uR, vT}, color});
        quads.push_back({{xL, yB}, {uL, vB}, color});
        quads.push_back({{xR, yT}, {uR, vT}, color});
        quads.push_back({{xL, yT}, {uL, vT}, color});
    }
    _bakedOpacity = 1.f;
}

void Label::draw(Renderer& renderer, const Mat4& world) {
    if (_fontAtlas && _atlasVersion != _fontAtlas->getAtlasVersion()) {
        _contentDirty = true;
    }
    if (_contentDirty && !updateContent()) {
        return;
    }
    if (!_ready || !_fontAtlas) {
        return;
    }

    const auto& textures = _fontAtlas->getTextures();
    const float opacity = getOpacity();
    if (opacity != _bakedOpacity) {
        const float clamped = std::max(0.f, std::min(1.f, opacity));
        const std::uint8_t alphaByte = static_cast<std::uint8_t>(clamped * 255.f + 0.5f);
        for (auto& quads : _quadsPerPage) {
            for (auto& v : quads) {
                v.color.a = alphaByte;
            }
        }
        _bakedOpacity = opacity;
    }
    for (std::size_t page = 0; page < _quadsPerPage.size() && page < textures.size(); ++page) {
        const auto& quads = _quadsPerPage[page];
        if (quads.empty()) {
            continue;
        }
        const TextureHandle texture = textures[page];
        if (!texture.isValid()) {
            continue;
        }
        const RenderSortKey sortKey = makeRenderSortKey(0, 0, texture.value);
        renderer.addDrawQuads(world, texture, quads, 1.f, sortKey);
    }
}

} // namespace zocos

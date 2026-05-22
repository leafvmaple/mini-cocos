#pragma once

#include "base/ZCRenderCommand.h"
#include "2d/ZCNode.h"

#include <cstdint>
#include "base/ZCStd.h"

namespace zocos {

class Director;
class Renderer;
class FontAtlas;

// Per-character layout record produced by Label::updateContent(), in the same
// spirit as cocos2d-x's LetterInfo. Coordinates are in label-local space
// (Y-up, origin at bottom-left) and point at the top-left corner of the glyph
// quad. `atlasIndex` selects which FontAtlas page the glyph lives on, which
// lets `updateQuads()` batch quads per page.
struct LetterInfo {
    char32_t utf32Char = 0;
    bool valid = false;
    float positionX = 0.f;
    float positionY = 0.f;
    int atlasIndex = 0;
    int lineIndex = 0;
};

class Label : public Node {
public:
    static Label* createWithTTF(Director& director, const mstd::string& text = "",
                                const mstd::string& fontPath = "", float fontSize = 24.f);

    ~Label() override;

    bool init() override;

    void setString(const mstd::string& text);
    const mstd::string& getString() const { return _text; }

    bool setFontAtlas(FontAtlas* fontAtlas);
    FontAtlas* getFontAtlas() const { return _fontAtlas; }

    bool setTTF(const mstd::string& fontPath);
    const mstd::string& getFontPath() const { return _fontPath; }

    void setFontSize(float fontSize);
    float getFontSize() const { return _fontSize; }

    // Maximum width (in label-local pixels) before a soft line break is
    // inserted. 0 (the default) disables wrapping and the label only breaks
    // on explicit '\n' characters. Mirrors cocos2d-x Label::setMaxLineWidth.
    void setMaxLineWidth(float maxLineWidth);
    float getMaxLineWidth() const { return _maxLineWidth; }

    void draw(Renderer& renderer, const Mat4& world) override;

protected:
    explicit Label(Director& director);

private:
    // Cocos2d-x style update pipeline.
    bool updateContent();
    void computeHorizontalKernings();
    void multilineTextWrap();
    void alignText();
    void recordLetterInfo(mstd::size_t letterIndex, char32_t utf32Char, float positionX,
                          float positionY, int atlasIndex, int lineIndex, bool valid = false);
    void updateQuads();
    void resetLayoutState();

    Director& _director;
    mstd::string _text;
    std::u32string _utf32Text;
    mstd::string _fontPath;
    float _fontSize = 24.f;
    float _maxLineWidth = 0.f;
    FontAtlas* _fontAtlas = nullptr;

    mstd::vector<LetterInfo> _lettersInfo;
    mstd::vector<float> _horizontalKernings;
    mstd::vector<mstd::vector<QuadVertex>> _quadsPerPage;

    float _contentWidth = 0.f;
    float _contentHeight = 0.f;
    std::uint32_t _atlasVersion = 0;
    float _bakedOpacity = -1.f;
    bool _ready = false;
    bool _contentDirty = true;
};

} // namespace zocos
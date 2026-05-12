#pragma once

#include "base/ZCRef.h"
#include "base/ZCRenderCommand.h"

#include <cstdint>
#include <string>
#include <unordered_map>
#include <vector>

namespace zocos {

class Director;
class Font;

struct FontGlyph {
    int codepoint = 0;
    int advance = 0;
    int bearingX = 0;
    int bearingY = 0;
    int width = 0;
    int height = 0;
    Rect uvRect{0.f, 0.f, 0.f, 0.f};
};

class FontAtlas : public Ref {
public:
    static FontAtlas* create(Director& director, const std::string& fontPath,
                             float fontSize = 24.f);

    ~FontAtlas() override;

    bool init(const std::string& fontPath, float fontSize);

    bool isValid() const;

    const std::string& getFontPath() const { return _fontPath; }
    float getFontSize() const { return _fontSize; }
    Font* getFont() const { return _font; }
    TextureHandle getAtlasTexture() const { return _atlasTexture; }
    std::uint32_t getAtlasVersion() const { return _atlasVersion; }

    float getScale() const { return _scale; }
    int getAscent() const { return _ascent; }
    int getDescent() const { return _descent; }
    int getLineGap() const { return _lineGap; }
    int getBaseline() const { return _baseline; }
    int getLineHeight() const { return _lineHeight; }

    bool getGlyph(int codepoint, FontGlyph& outGlyph);
    int getKerning(int lhsCodepoint, int rhsCodepoint) const;
    bool commitAtlasTexture();

protected:
    explicit FontAtlas(Director& director);

private:
    bool addGlyphToAtlas(int codepoint, FontGlyph& outGlyph);
    bool allocGlyphRect(int glyphWidth, int glyphHeight, int& outPixelX, int& outPixelY);
    bool uploadAtlasTexture();
    Rect toUvRectTopLeft(int x, int y, int width, int height) const;
    void releaseAtlasTexture();
    void releaseFont();

    Director& _director;
    Font* _font = nullptr;
    std::string _fontPath;
    float _fontSize = 24.f;
    float _scale = 1.f;
    int _ascent = 0;
    int _descent = 0;
    int _lineGap = 0;
    int _baseline = 0;
    int _lineHeight = 1;

    std::unordered_map<int, FontGlyph> _glyphs;
    std::vector<unsigned char> _atlasPixels;
    int _atlasWidth = 512;
    int _atlasHeight = 512;
    int _packCursorX = 0;
    int _packCursorY = 0;
    int _packRowHeight = 0;
    TextureHandle _atlasTexture{};
    std::uint32_t _atlasVersion = 0;
    bool _atlasDirty = false;
};

} // namespace zocos

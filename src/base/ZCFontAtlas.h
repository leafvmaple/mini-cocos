#pragma once

#include "base/ZCRef.h"
#include "base/ZCRenderCommand.h"

#include <cstdint>
#include <memory>
#include <string>
#include <unordered_map>
#include <vector>

struct stbtt_fontinfo;

namespace zocos {

class Director;
class Font;

// Cocos2d-x style glyph descriptor produced by FontAtlas. U/V/width/height are
// in pixels relative to the atlas page identified by `textureID`. offsetX is
// the glyph bearing along the pen; offsetY is the distance from the line top
// to the glyph top (positive for visible glyphs).
struct LetterDefinition {
    char32_t utf32Char = 0;
    float U = 0.f;
    float V = 0.f;
    float width = 0.f;
    float height = 0.f;
    float offsetX = 0.f;
    float offsetY = 0.f;
    int textureID = 0;
    int xAdvance = 0;
    bool validDefinition = false;
};

class FontAtlas : public Ref {
public:
    static FontAtlas* create(Director& director, const std::string& fontPath,
                             float fontSize = 24.f);

    ~FontAtlas() override;

    bool init(const std::string& fontPath, float fontSize);

    bool isValid() const { return _font != nullptr && !_atlasPages.empty(); }

    const std::string& getFontPath() const { return _fontPath; }
    float getFontSize() const { return _fontSize; }
    Font* getFont() const { return _font; }
    std::uint32_t getAtlasVersion() const { return _atlasVersion; }

    // Font metrics in pixels (already scaled).
    float getScale() const { return _scale; }
    float getLineHeight() const { return _lineHeight; }
    float getFontAscender() const { return _fontAscender; }
    float getFontDescender() const { return _fontDescender; }

    // Lazily rasterise every codepoint in `utf32Text` into the atlas pages.
    // Newly added glyphs cause affected pages to be re-uploaded before return.
    bool prepareLetterDefinitions(const std::u32string& utf32Text);

    bool findLetterDefinitionForChar(char32_t utf32Char, LetterDefinition& outDef);

    // Kerning expressed in pixels (already scaled).
    float getHorizontalKerningForChars(char32_t first, char32_t second) const;

    const std::vector<TextureHandle>& getTextures() const { return _atlasTextures; }
    int getAtlasPageCount() const { return static_cast<int>(_atlasPages.size()); }
    int getAtlasWidth() const { return _atlasWidth; }
    int getAtlasHeight() const { return _atlasHeight; }

protected:
    explicit FontAtlas(Director& director);

private:
    struct AtlasPage {
        std::vector<unsigned char> pixels; // RGBA8, top-left origin.
        TextureHandle texture{};
        int packCursorX = 0;
        int packCursorY = 0;
        int packRowHeight = 0;
        bool dirty = false;
    };

    bool prepareLetterDefinition(char32_t utf32Char, LetterDefinition& outDef);
    bool allocGlyphRect(int glyphWidth, int glyphHeight, int& outPageIndex, int& outPixelX,
                        int& outPixelY);
    int addNewPage();
    bool commitDirtyPages();
    bool uploadPage(AtlasPage& page);
    void releasePages();
    void releaseFont();

    Director& _director;
    Font* _font = nullptr;
    std::unique_ptr<stbtt_fontinfo> _fontInfo;
    std::string _fontPath;
    float _fontSize = 24.f;
    float _scale = 1.f;
    float _fontAscender = 0.f;
    float _fontDescender = 0.f;
    float _lineHeight = 1.f;

    std::unordered_map<char32_t, LetterDefinition> _letterDefinitions;

    std::vector<AtlasPage> _atlasPages;
    std::vector<TextureHandle> _atlasTextures; // mirrors _atlasPages[i].texture
    int _atlasWidth = 1024;
    int _atlasHeight = 1024;
    std::uint32_t _atlasVersion = 0;
};

} // namespace zocos

#pragma once

#include "base/ZCRef.h"

#include <string>

namespace zocos {

class Director;
class Font;

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

    float getScale() const { return _scale; }
    int getAscent() const { return _ascent; }
    int getDescent() const { return _descent; }
    int getLineGap() const { return _lineGap; }
    int getBaseline() const { return _baseline; }
    int getLineHeight() const { return _lineHeight; }

protected:
    explicit FontAtlas(Director& director);

private:
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
};

} // namespace zocos

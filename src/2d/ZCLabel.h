#pragma once

#include "base/ZCRenderCommand.h"
#include "2d/ZCNode.h"

#include <string>
#include <vector>

namespace zocos {

class Director;
class Renderer;
class FontAtlas;

class Label : public Node {
public:
    static Label* createWithTTF(Director& director, const std::string& text = "",
                                const std::string& fontPath = "", float fontSize = 24.f);

    ~Label() override;

    bool init() override;

    void setString(const std::string& text);
    const std::string& getString() const { return _text; }

    bool setFontAtlas(FontAtlas* fontAtlas);
    FontAtlas* getFontAtlas() const { return _fontAtlas; }

    bool setTTF(const std::string& fontPath);
    const std::string& getFontPath() const { return _fontPath; }

    void setFontSize(float fontSize);
    float getFontSize() const { return _fontSize; }

    void draw(Renderer& renderer, const Mat4& world) override;

protected:
    explicit Label(Director& director);

private:
    bool updateContent();
    bool ensureFontAtlas();

    Director& _director;
    std::string _text;
    std::string _fontPath;
    float _fontSize = 24.f;
    FontAtlas* _fontAtlas = nullptr;

    std::vector<QuadVertex> _vertices;
    std::uint32_t _atlasVersion = 0;
    bool _ready = false;
    bool _dirty = true;
};

} // namespace zocos
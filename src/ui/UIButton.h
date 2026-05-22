#pragma once

#include "base/ZCRenderCommand.h"
#include "ui/UIWidget.h"

#include "base/ZCStd.h"

namespace zocos {

class Director;
class Renderer;
class Label;
class FontAtlas;

namespace ui {

class Button : public Widget {
public:
    static Button* create(Director& director, const mstd::string& title = "");

    ~Button() override;

    bool init() override;

    void setString(const mstd::string& title);
    const mstd::string& getString() const { return _title; }

    bool setTitleFontName(const mstd::string& fontPath);
    void setTitleFontSize(float fontSize);

    bool setFontAtlas(FontAtlas* fontAtlas);

    void draw(Renderer& renderer, const Mat4& world) override;

protected:
    explicit Button(Director& director);

private:
    bool ensureTextures();
    void releaseTextures();
    void updateTitleLayout();

    Director& _director;
    TextureHandle _normalTexture{};
    TextureHandle _pressedTexture{};
    bool _ready = false;
    mstd::string _title;
    Label* _titleLabel = nullptr;
};

} // namespace ui

} // namespace zocos
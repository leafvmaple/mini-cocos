#pragma once

#include "2d/ZCNode.h"
#include "base/ZCRenderCommand.h"

#include <cstdint>
#include <functional>
#include <string>

namespace zocos {

class Director;
class Renderer;
class Label;
class FontAtlas;
class EventMouseButton;
class EventMouseMove;

namespace ui {

class Button : public Node {
public:
    using ClickCallback = std::function<void(Button&)>;

    static Button* create(Director& director, const std::string& title = "");

    ~Button() override;

    bool init() override;

    void onEnter() override;
    void onExit() override;

    void setString(const std::string& title);
    const std::string& getString() const { return _title; }

    bool setTitleFontName(const std::string& fontPath);
    void setTitleFontSize(float fontSize);

    bool setFontAtlas(FontAtlas* fontAtlas);

    void setOnClick(ClickCallback callback) { _onClick = std::move(callback); }

    void draw(Renderer& renderer, const Mat4& world) override;

protected:
    explicit Button(Director& director);

private:
    bool ensureTextures();
    void releaseTextures();
    void updateTitleLayout();

    void registerInputListener();
    void unregisterInputListener();

    bool containsWorldPoint(float x, float y) const;
    void handleMouseDown(EventMouseButton& event);
    void handleMouseUp(EventMouseButton& event);
    void handleMouseMove(EventMouseMove& event);

    Director& _director;
    TextureHandle _normalTexture{};
    TextureHandle _pressedTexture{};
    bool _ready = false;
    bool _pressed = false;
    bool _trackingPress = false;
    std::string _title;
    Label* _titleLabel = nullptr;
    std::uint64_t _listenerId = 0;
    ClickCallback _onClick;
};

} // namespace ui

} // namespace zocos
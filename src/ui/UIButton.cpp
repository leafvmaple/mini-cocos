#include "ui/UIButton.h"

#include "2d/ZCLabel.h"
#include "base/ZCFontAtlas.h"
#include "base/ZCDirector.h"
#include "base/ZCRenderDevice.h"
#include "base/ZCRenderer.h"

#include <new>

namespace zocos::ui {

namespace {

void destroyTexture(Director& director, TextureHandle& inOutTexture) {
    auto* device = director.getRenderDevice();
    if (!device || !inOutTexture.isValid()) {
        return;
    }
    device->destroyTexture(inOutTexture);
    inOutTexture = {};
}

bool createSolidTexture(Director& director, unsigned char r, unsigned char g, unsigned char b,
                        unsigned char a, TextureHandle& outTexture) {
    auto* device = director.getRenderDevice();
    if (!device) {
        return false;
    }

    const unsigned char pixel[4] = {r, g, b, a};
    TextureCreateInfo createInfo;
    createInfo.width = 1;
    createInfo.height = 1;
    createInfo.format = TextureFormat::RGBA8Unorm;
    createInfo.initialData.pixels = pixel;
    createInfo.initialData.rowPitchBytes = 4;
    createInfo.initialData.origin = TextureDataOrigin::TopLeft;

    outTexture = device->createTexture(createInfo);
    return outTexture.isValid();
}

} // namespace

Button::Button(Director& director) : _director(director) {}

Button* Button::create(Director& director, const std::string& title) {
    auto* button = new (std::nothrow) Button(director);
    if (button && button->init()) {
        button->setString(title);
        button->autorelease();
        return button;
    }
    delete button;
    return nullptr;
}

Button::~Button() {
    if (_titleLabel) {
        _titleLabel->release();
        _titleLabel = nullptr;
    }
    releaseTextures();
}

bool Button::init() {
    if (!Widget::init()) {
        return false;
    }

    setContentSize({180.f, 56.f});
    if (!ensureTextures()) {
        return false;
    }

    _titleLabel = Label::createWithTTF(_director);
    if (_titleLabel) {
        _titleLabel->retain();
        _titleLabel->setAnchorPoint({0.5f, 0.5f});
        addChild(_titleLabel);
    }

    updateTitleLayout();
    return true;
}

void Button::setString(const std::string& title) {
    _title = title;
    if (_titleLabel) {
        _titleLabel->setString(_title);
        updateTitleLayout();
    }
}

bool Button::setTitleFontName(const std::string& fontPath) {
    if (!_titleLabel) {
        return false;
    }
    return _titleLabel->setTTF(fontPath);
}

void Button::setTitleFontSize(float fontSize) {
    if (!_titleLabel) {
        return;
    }
    _titleLabel->setFontSize(fontSize);
    updateTitleLayout();
}

bool Button::setFontAtlas(FontAtlas* fontAtlas) {
    if (!_titleLabel) {
        return false;
    }
    return _titleLabel->setFontAtlas(fontAtlas);
}

void Button::draw(Renderer& renderer, const Mat4& world) {
    if (!_ready) {
        return;
    }

    const TextureHandle texture =
        (isPressed() && _pressedTexture.isValid()) ? _pressedTexture : _normalTexture;
    if (!texture.isValid()) {
        return;
    }

    updateTitleLayout();

    const RenderSortKey sortKey = makeRenderSortKey(0, 0, texture.value);
    renderer.addDrawSprite(world, _contentSize, texture, getOpacity(), sortKey);
}

bool Button::ensureTextures() {
    releaseTextures();

    if (!createSolidTexture(_director, 45, 122, 226, 255, _normalTexture)) {
        _ready = false;
        return false;
    }
    if (!createSolidTexture(_director, 30, 85, 165, 255, _pressedTexture)) {
        destroyTexture(_director, _normalTexture);
        _ready = false;
        return false;
    }

    _ready = true;
    return true;
}

void Button::releaseTextures() {
    destroyTexture(_director, _normalTexture);
    destroyTexture(_director, _pressedTexture);
    _ready = false;
}

void Button::updateTitleLayout() {
    if (!_titleLabel) {
        return;
    }

    const Size size = getContentSize();
    _titleLabel->setPosition(size.width * 0.5f, size.height * 0.5f);
}

} // namespace zocos::ui

#include "ui/UIButton.h"

#include "2d/ZCLabel.h"
#include "base/ZCFontAtlas.h"
#include "base/ZCDirector.h"
#include "base/ZCEvent.h"
#include "base/ZCEventDispatcher.h"
#include "base/ZCEventListener.h"
#include "base/ZCRenderDevice.h"
#include "base/ZCRenderer.h"

#include <cmath>
#include <new>
#include <vector>

namespace zocos::ui {

namespace {

constexpr float kPi = 3.14159265f;
constexpr int kLeftMouseButton = 0;

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

bool applyInverseNodeTransform(const Node* node, Vec2& point) {
    if (!node) {
        return false;
    }

    const Vec2 position = node->getPosition();
    const Vec2 scale = node->getScale();
    const float rotationDegrees = node->getRotation();
    const Vec2 anchor = node->getAnchorPoint();
    const Size size = node->getContentSize();

    point.x -= position.x;
    point.y -= position.y;

    const float rad = -rotationDegrees * kPi / 180.f;
    const float c = std::cos(rad);
    const float s = std::sin(rad);
    const float rx = point.x * c - point.y * s;
    const float ry = point.x * s + point.y * c;
    point.x = rx;
    point.y = ry;

    if (std::fabs(scale.x) <= 1e-6f || std::fabs(scale.y) <= 1e-6f) {
        return false;
    }
    point.x /= scale.x;
    point.y /= scale.y;

    point.x += anchor.x * size.width;
    point.y += anchor.y * size.height;
    return true;
}

Vec2 worldToLocal(const Node* node, Vec2 point, bool& ok) {
    ok = true;

    std::vector<const Node*> chain;
    for (const Node* current = node; current != nullptr; current = current->getParent()) {
        chain.push_back(current);
    }

    for (auto it = chain.rbegin(); it != chain.rend(); ++it) {
        if (!applyInverseNodeTransform(*it, point)) {
            ok = false;
            return point;
        }
    }

    return point;
}

} // namespace

Button::Button(Director& director) : _director(director) {
}

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
    unregisterInputListener();
    if (_titleLabel) {
        _titleLabel->release();
        _titleLabel = nullptr;
    }
    releaseTextures();
}

bool Button::init() {
    if (!Node::init()) {
        return false;
    }

    setContentSize({180.f, 56.f});
    if (!ensureTextures()) {
        return false;
    }

    _titleLabel = Label::create(_director, "");
    if (_titleLabel) {
        _titleLabel->retain();
        _titleLabel->setAnchorPoint({0.5f, 0.5f});
        addChild(_titleLabel);
    }

    updateTitleLayout();
    return true;
}

void Button::onEnter() {
    Node::onEnter();
    registerInputListener();
}

void Button::onExit() {
    _pressed = false;
    _trackingPress = false;
    unregisterInputListener();
    Node::onExit();
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

    const TextureHandle texture = (_pressed && _pressedTexture.isValid()) ? _pressedTexture : _normalTexture;
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

void Button::registerInputListener() {
    if (_listenerId != 0) {
        return;
    }

    auto* listener = EventListenerMouse::create();
    if (!listener) {
        return;
    }

    listener->onMouseDown = [this](EventMouseButton& event) {
        handleMouseDown(event);
    };
    listener->onMouseUp = [this](EventMouseButton& event) {
        handleMouseUp(event);
    };
    listener->onMouseMove = [this](EventMouseMove& event) {
        handleMouseMove(event);
    };

    _listenerId = _director.getEventDispatcher().addEventListener(listener, this);
}

void Button::unregisterInputListener() {
    if (_listenerId == 0) {
        return;
    }

    _director.getEventDispatcher().removeListener(_listenerId);
    _listenerId = 0;
}

bool Button::containsWorldPoint(float x, float y) const {
    bool ok = false;
    const Vec2 local = worldToLocal(this, Vec2{x, y}, ok);
    if (!ok) {
        return false;
    }

    const Size size = getContentSize();
    if (size.width <= 0.f || size.height <= 0.f) {
        return false;
    }

    return local.x >= 0.f && local.y >= 0.f && local.x <= size.width && local.y <= size.height;
}

void Button::handleMouseDown(EventMouseButton& event) {
    if (event.getButton() != kLeftMouseButton) {
        return;
    }

    if (!containsWorldPoint(event.getX(), event.getY())) {
        return;
    }

    _trackingPress = true;
    _pressed = true;
    event.stopPropagation();
}

void Button::handleMouseUp(EventMouseButton& event) {
    if (event.getButton() != kLeftMouseButton) {
        return;
    }

    if (!_trackingPress) {
        return;
    }

    const bool inside = containsWorldPoint(event.getX(), event.getY());
    _trackingPress = false;
    _pressed = false;

    if (inside && _onClick) {
        _onClick(*this);
    }

    event.stopPropagation();
}

void Button::handleMouseMove(EventMouseMove& event) {
    if (!_trackingPress) {
        return;
    }

    _pressed = containsWorldPoint(event.getX(), event.getY());
}

} // namespace zocos::ui
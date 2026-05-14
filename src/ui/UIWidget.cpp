#include "ui/UIWidget.h"

#include "base/ZCDirector.h"
#include "base/ZCEvent.h"
#include "base/ZCEventDispatcher.h"
#include "base/ZCEventListener.h"

#include <cmath>
#include <utility>
#include <vector>

namespace zocos::ui {

namespace {

constexpr float kPi = 3.14159265f;

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

Widget::~Widget() { unregisterInputListener(); }

bool Widget::init() { return Node::init(); }

void Widget::onEnter() {
    Node::onEnter();
    registerInputListener();
}

void Widget::onExit() {
    if (_pressed) {
        _pressed = false;
        onPressStateChanged(false);
    }
    _trackingPress = false;
    unregisterInputListener();
    Node::onExit();
}

void Widget::addEventListener(EventCallback callback) { _eventCallback = std::move(callback); }

bool Widget::hitTest(float worldX, float worldY) const {
    return containsWorldPoint(worldX, worldY);
}

void Widget::onPressStateChanged(bool /*pressed*/) {}

void Widget::registerInputListener() {
    if (_listenerId != 0) {
        return;
    }

    auto* listener = EventListenerMouse::create();
    if (!listener) {
        return;
    }

    listener->onMouseDown = [this](EventMouse& event) { handleMouseDown(event); };
    listener->onMouseUp = [this](EventMouse& event) { handleMouseUp(event); };
    listener->onMouseMove = [this](EventMouse& event) { handleMouseMove(event); };

    _listenerId = Director::getInstance().getEventDispatcher().addEventListenerWithNodePriority(
        listener, this);
}

void Widget::unregisterInputListener() {
    if (_listenerId == 0) {
        return;
    }

    Director::getInstance().getEventDispatcher().removeEventListener(_listenerId);
    _listenerId = 0;
}

bool Widget::containsWorldPoint(float x, float y) const {
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

void Widget::handleMouseDown(EventMouse& event) {
    if (event.getMouseButton() != EventMouse::MouseButton::BUTTON_LEFT) {
        return;
    }

    if (!hitTest(event.getX(), event.getY())) {
        return;
    }

    _trackingPress = true;
    _pressed = true;
    onPressStateChanged(true);
    event.stopPropagation();
}

void Widget::handleMouseUp(EventMouse& event) {
    if (event.getMouseButton() != EventMouse::MouseButton::BUTTON_LEFT) {
        return;
    }

    if (!_trackingPress) {
        return;
    }

    const bool inside = hitTest(event.getX(), event.getY());
    _trackingPress = false;
    if (_pressed) {
        _pressed = false;
        onPressStateChanged(false);
    }

    if (inside && _eventCallback) {
        _eventCallback(*this);
    }

    event.stopPropagation();
}

void Widget::handleMouseMove(EventMouse& event) {
    if (!_trackingPress) {
        return;
    }

    const bool pressed = hitTest(event.getX(), event.getY());
    if (pressed == _pressed) {
        return;
    }

    _pressed = pressed;
    onPressStateChanged(pressed);
}

} // namespace zocos::ui

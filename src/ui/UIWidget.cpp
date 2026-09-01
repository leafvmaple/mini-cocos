#include "ui/UIWidget.h"

#include "base/ZCDirector.h"
#include "base/ZCEvent.h"
#include "base/ZCEventDispatcher.h"
#include "base/ZCEventListener.h"

#include "base/ZCStd.h"

namespace zocos::ui {

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

void Widget::addEventListener(EventCallback callback) { _eventCallback = mstd::move(callback); }

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
    Vec2 local;
    if (!convertToNodeSpace(Vec2{x, y}, local)) {
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

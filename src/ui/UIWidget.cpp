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

    auto* listener = EventListenerTouchOneByOne::create();
    if (!listener) {
        return;
    }

    listener->setSwallowTouches(true);
    listener->onTouchBegan =
        [this](Touch& touch, EventTouch&) { return handleTouchBegan(touch); };
    listener->onTouchMoved = [this](Touch& touch, EventTouch&) { handleTouchMoved(touch); };
    listener->onTouchEnded = [this](Touch& touch, EventTouch&) { handleTouchEnded(touch); };
    listener->onTouchCancelled = [this](Touch&, EventTouch&) { handleTouchCancelled(); };

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

bool Widget::handleTouchBegan(Touch& touch) {
    const Vec2& location = touch.getLocation();
    if (!hitTest(location.x, location.y)) {
        return false;
    }

    _trackingPress = true;
    _pressed = true;
    onPressStateChanged(true);
    return true;
}

void Widget::handleTouchEnded(Touch& touch) {
    if (!_trackingPress) {
        return;
    }

    const Vec2& location = touch.getLocation();
    const bool inside = hitTest(location.x, location.y);
    _trackingPress = false;
    if (_pressed) {
        _pressed = false;
        onPressStateChanged(false);
    }

    if (inside && _eventCallback) {
        auto callback = _eventCallback;
        retain();
        callback(*this);
        release();
    }
}

void Widget::handleTouchMoved(Touch& touch) {
    if (!_trackingPress) {
        return;
    }

    const Vec2& location = touch.getLocation();
    const bool pressed = hitTest(location.x, location.y);
    if (pressed == _pressed) {
        return;
    }

    _pressed = pressed;
    onPressStateChanged(pressed);
}

void Widget::handleTouchCancelled() {
    _trackingPress = false;
    if (_pressed) {
        _pressed = false;
        onPressStateChanged(false);
    }
}

} // namespace zocos::ui

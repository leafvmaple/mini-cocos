#pragma once

#include "2d/ZCNode.h"
#include "base/ZCEventDispatcher.h"

#include <cstdint>
#include "base/ZCStd.h"

namespace zocos {

class EventMouse;

namespace ui {

class Widget : public Node {
public:
    using EventCallback = mstd::function<void(Widget&)>;

    ~Widget() override;

    bool init() override;

    void onEnter() override;
    void onExit() override;

    void addEventListener(EventCallback callback);

protected:
    Widget() = default;

    bool isPressed() const { return _pressed; }

    virtual bool hitTest(float worldX, float worldY) const;
    virtual void onPressStateChanged(bool pressed);

private:
    void registerInputListener();
    void unregisterInputListener();

    bool containsWorldPoint(float x, float y) const;
    void handleMouseDown(EventMouse& event);
    void handleMouseUp(EventMouse& event);
    void handleMouseMove(EventMouse& event);

    EventDispatcher::ListenerHandle _listenerId = 0;
    bool _pressed = false;
    bool _trackingPress = false;
    EventCallback _eventCallback;
};

} // namespace ui

} // namespace zocos

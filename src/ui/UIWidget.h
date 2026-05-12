#pragma once

#include "2d/ZCNode.h"

#include <cstdint>
#include <functional>

namespace zocos {

class EventMouseButton;
class EventMouseMove;

namespace ui {

class Widget : public Node {
public:
    using EventCallback = std::function<void(Widget&)>;

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
    void handleMouseDown(EventMouseButton& event);
    void handleMouseUp(EventMouseButton& event);
    void handleMouseMove(EventMouseMove& event);

    std::uint64_t _listenerId = 0;
    bool _pressed = false;
    bool _trackingPress = false;
    EventCallback _eventCallback;
};

} // namespace ui

} // namespace zocos

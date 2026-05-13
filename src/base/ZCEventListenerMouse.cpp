#include "base/ZCEventListener.h"

#include <new>

namespace zocos {

EventListenerMouse* EventListenerMouse::create() {
    auto* listener = new (std::nothrow) EventListenerMouse();
    if (listener && listener->init()) {
        listener->autorelease();
        return listener;
    }
    delete listener;
    return nullptr;
}

bool EventListenerMouse::init() { return true; }

bool EventListenerMouse::hasCallbacks() const {
    return static_cast<bool>(onMouseDown) || static_cast<bool>(onMouseUp) ||
           static_cast<bool>(onMouseMove) || static_cast<bool>(onMouseScroll);
}

bool EventListenerMouse::dispatchEvent(Event& event) {
    if (event.getType() == Event::Type::Keyboard) {
        return false;
    }

    auto& mouseEvent = static_cast<EventMouse&>(event);

    bool dispatched = false;
    switch (mouseEvent.getMouseEventType()) {
    case EventMouse::MouseEventType::MOUSE_DOWN:
        if (onMouseDown != nullptr) {
            onMouseDown(static_cast<EventMouseButton&>(mouseEvent));
            dispatched = true;
        }
        break;
    case EventMouse::MouseEventType::MOUSE_UP:
        if (onMouseUp != nullptr) {
            onMouseUp(static_cast<EventMouseButton&>(mouseEvent));
            dispatched = true;
        }
        break;
    case EventMouse::MouseEventType::MOUSE_MOVE:
        if (onMouseMove != nullptr) {
            onMouseMove(static_cast<EventMouseMove&>(mouseEvent));
            dispatched = true;
        }
        break;
    case EventMouse::MouseEventType::MOUSE_SCROLL:
        if (onMouseScroll != nullptr) {
            onMouseScroll(static_cast<EventMouseScroll&>(mouseEvent));
            dispatched = true;
        }
        break;
    default:
        break;
    }

    return dispatched;
}

EventListenerMouse::EventListenerMouse() : EventListener(Type::Mouse) {}

} // namespace zocos

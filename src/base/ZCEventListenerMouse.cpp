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
    switch (event.getType()) {
    case Event::Type::MouseButton: {
        auto& mouseButtonEvent = static_cast<EventMouseButton&>(event);
        if (mouseButtonEvent.isPressed()) {
            if (!onMouseDown) {
                return false;
            }
            onMouseDown(mouseButtonEvent);
            return true;
        }

        if (!onMouseUp) {
            return false;
        }
        onMouseUp(mouseButtonEvent);
        return true;
    }
    case Event::Type::MouseMove:
        if (!onMouseMove) {
            return false;
        }
        onMouseMove(static_cast<EventMouseMove&>(event));
        return true;
    case Event::Type::MouseScroll:
        if (!onMouseScroll) {
            return false;
        }
        onMouseScroll(static_cast<EventMouseScroll&>(event));
        return true;
    case Event::Type::Keyboard:
        return false;
    }

    return false;
}

EventListenerMouse::EventListenerMouse() : EventListener(Type::Mouse) {}

} // namespace zocos

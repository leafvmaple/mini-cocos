#include "base/ZCEventListener.h"

#include "base/ZCStd.h"

namespace zocos {

EventListenerKeyboard* EventListenerKeyboard::create(Callback onPressed, Callback onReleased) {
    auto* listener = new (mstd::nothrow) EventListenerKeyboard();
    if (listener && listener->init(mstd::move(onPressed), mstd::move(onReleased))) {
        listener->autorelease();
        return listener;
    }
    delete listener;
    return nullptr;
}

bool EventListenerKeyboard::init(Callback onPressed, Callback onReleased) {
    onKeyPressed = mstd::move(onPressed);
    onKeyReleased = mstd::move(onReleased);
    return true;
}

bool EventListenerKeyboard::hasCallbacks() const {
    return static_cast<bool>(onKeyPressed) || static_cast<bool>(onKeyReleased);
}

bool EventListenerKeyboard::dispatchEvent(Event& event) {
    if (event.getType() != Event::Type::Keyboard) {
        return false;
    }

    auto& keyEvent = static_cast<EventKeyboard&>(event);
    if (keyEvent.isPressed()) {
        if (!onKeyPressed) {
            return false;
        }
        onKeyPressed(keyEvent);
        return true;
    }

    if (!onKeyReleased) {
        return false;
    }
    onKeyReleased(keyEvent);
    return true;
}

EventListenerKeyboard::EventListenerKeyboard() : EventListener(Type::Keyboard) {}

} // namespace zocos

#pragma once

#include "base/ZCEvent.h"
#include "base/ZCRef.h"

#include <functional>
#include <new>
#include <utility>

namespace zocos {

class EventListener : public Ref {
public:
    enum class Type {
        Keyboard,
        Mouse,
    };

    Type getType() const { return _type; }

    bool isEnabled() const { return _enabled; }
    void setEnabled(bool enabled) { _enabled = enabled; }

protected:
    explicit EventListener(Type type) : _type(type) {}
    ~EventListener() override = default;

private:
    Type _type;
    bool _enabled = true;
};

class EventListenerKeyboard final : public EventListener {
public:
    using Callback = std::function<void(EventKeyboard&)>;

    static EventListenerKeyboard* create(Callback onPressed = {}, Callback onReleased = {}) {
        auto* listener = new (std::nothrow) EventListenerKeyboard();
        if (listener && listener->init(std::move(onPressed), std::move(onReleased))) {
            listener->autorelease();
            return listener;
        }
        delete listener;
        return nullptr;
    }

    bool init(Callback onPressed = {}, Callback onReleased = {}) {
        onKeyPressed = std::move(onPressed);
        onKeyReleased = std::move(onReleased);
        return true;
    }

    Callback onKeyPressed;
    Callback onKeyReleased;

private:
    EventListenerKeyboard() : EventListener(Type::Keyboard) {}
};

class EventListenerMouse final : public EventListener {
public:
    using ButtonCallback = std::function<void(EventMouseButton&)>;
    using MoveCallback = std::function<void(EventMouseMove&)>;
    using ScrollCallback = std::function<void(EventMouseScroll&)>;

    static EventListenerMouse* create() {
        auto* listener = new (std::nothrow) EventListenerMouse();
        if (listener && listener->init()) {
            listener->autorelease();
            return listener;
        }
        delete listener;
        return nullptr;
    }

    bool init() { return true; }

    ButtonCallback onMouseDown;
    ButtonCallback onMouseUp;
    MoveCallback onMouseMove;
    ScrollCallback onMouseScroll;

private:
    EventListenerMouse() : EventListener(Type::Mouse) {}
};

} // namespace zocos
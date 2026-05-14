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

    virtual bool hasCallbacks() const = 0;
    virtual bool dispatchEvent(Event& event) = 0;

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

    static EventListenerKeyboard* create(Callback onPressed = {}, Callback onReleased = {});

    bool init(Callback onPressed = {}, Callback onReleased = {});

    bool hasCallbacks() const override;

    bool dispatchEvent(Event& event) override;

    Callback onKeyPressed;
    Callback onKeyReleased;

private:
    EventListenerKeyboard();
};

class EventListenerMouse final : public EventListener {
public:
    using Callback = std::function<void(EventMouse&)>;
    using ButtonCallback = Callback;
    using MoveCallback = Callback;
    using ScrollCallback = Callback;

    static EventListenerMouse* create();

    bool init();

    bool hasCallbacks() const override;

    bool dispatchEvent(Event& event) override;

    ButtonCallback onMouseDown;
    ButtonCallback onMouseUp;
    MoveCallback onMouseMove;
    ScrollCallback onMouseScroll;

private:
    EventListenerMouse();
};

} // namespace zocos
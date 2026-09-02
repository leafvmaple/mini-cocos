#pragma once

#include "base/ZCEvent.h"
#include "base/ZCRef.h"

#include "base/ZCStd.h"

namespace zocos {

class EventListener : public Ref {
public:
    enum class Type {
        Keyboard,
        Mouse,
        TouchOneByOne,
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
    using Callback = mstd::function<void(EventKeyboard&)>;

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
    using Callback = mstd::function<void(EventMouse&)>;
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

class EventListenerTouchOneByOne final : public EventListener {
public:
    using TouchBeganCallback = mstd::function<bool(Touch&, EventTouch&)>;
    using TouchCallback = mstd::function<void(Touch&, EventTouch&)>;

    static EventListenerTouchOneByOne* create();

    bool init();
    bool hasCallbacks() const override;
    bool dispatchEvent(Event& event) override;

    void setSwallowTouches(bool swallowTouches) { _swallowTouches = swallowTouches; }
    bool isSwallowTouches() const { return _swallowTouches; }

    TouchBeganCallback onTouchBegan;
    TouchCallback onTouchMoved;
    TouchCallback onTouchEnded;
    TouchCallback onTouchCancelled;

private:
    EventListenerTouchOneByOne();

    bool hasClaimedTouch(int touchId) const;
    void claimTouch(int touchId);
    void unclaimTouch(int touchId);

    mstd::vector<int> _claimedTouchIds;
    bool _swallowTouches = false;

    friend class EventDispatcher;
};

} // namespace zocos

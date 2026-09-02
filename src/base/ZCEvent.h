#pragma once

#include "base/ZCStd.h"
#include "math/ZCMath.h"

namespace zocos {

class Node;

class Touch {
public:
    void setTouchInfo(int id, float x, float y) {
        _id = id;
        _previousLocation = _location;
        _location = {x, y};
        if (!_startLocationCaptured) {
            _startLocation = _location;
            _previousLocation = _location;
            _startLocationCaptured = true;
        }
    }

    int getID() const { return _id; }
    const Vec2& getLocation() const { return _location; }
    const Vec2& getPreviousLocation() const { return _previousLocation; }
    const Vec2& getStartLocation() const { return _startLocation; }
    Vec2 getDelta() const {
        return {_location.x - _previousLocation.x, _location.y - _previousLocation.y};
    }

private:
    int _id = 0;
    bool _startLocationCaptured = false;
    Vec2 _startLocation{};
    Vec2 _location{};
    Vec2 _previousLocation{};
};

class Event {
public:
    enum class Type {
        Keyboard,
        Mouse,
        Touch,
    };

    virtual ~Event() = default;

    Type getType() const { return _type; }
    void stopPropagation() { _stopped = true; }
    bool isStopped() const { return _stopped; }
    Node* getCurrentTarget() const { return _currentTarget; }

protected:
    explicit Event(Type type) : _type(type) {}

private:
    friend class EventDispatcher;

    void resetForDispatch() {
        _stopped = false;
        _currentTarget = nullptr;
    }

    void setCurrentTarget(Node* node) { _currentTarget = node; }

    Type _type;
    bool _stopped = false;
    Node* _currentTarget = nullptr;
};

class EventKeyboard : public Event {
public:
    EventKeyboard(int keyCode, int scanCode, int modifiers, bool pressed, bool repeated)
        : Event(Type::Keyboard), _keyCode(keyCode), _scanCode(scanCode), _modifiers(modifiers),
          _pressed(pressed), _repeated(repeated) {}

    int getKeyCode() const { return _keyCode; }
    int getScanCode() const { return _scanCode; }
    int getModifiers() const { return _modifiers; }
    bool isPressed() const { return _pressed; }
    bool isRepeated() const { return _repeated; }

private:
    int _keyCode = 0;
    int _scanCode = 0;
    int _modifiers = 0;
    bool _pressed = false;
    bool _repeated = false;
};

class EventMouse : public Event {
public:
    enum class MouseEventType {
        MOUSE_DOWN,
        MOUSE_UP,
        MOUSE_MOVE,
        MOUSE_SCROLL,
    };

    enum class MouseButton {
        BUTTON_UNSET = -1,
        BUTTON_LEFT = 0,
        BUTTON_RIGHT = 1,
        BUTTON_MIDDLE = 2,
        BUTTON_4 = 3,
        BUTTON_5 = 4,
        BUTTON_6 = 5,
        BUTTON_7 = 6,
        BUTTON_8 = 7,
    };

    explicit EventMouse(MouseEventType mouseEventType)
        : Event(Type::Mouse), _mouseEventType(mouseEventType) {}

    void setMouseButton(MouseButton mouseButton) { _mouseButton = mouseButton; }
    void setModifiers(int modifiers) { _modifiers = modifiers; }
    void setPosition(float x, float y) {
        _x = x;
        _y = y;
    }
    void setDelta(float deltaX, float deltaY) {
        _deltaX = deltaX;
        _deltaY = deltaY;
    }
    void setOffset(float offsetX, float offsetY) {
        _offsetX = offsetX;
        _offsetY = offsetY;
    }

    MouseEventType getMouseEventType() const { return _mouseEventType; }
    MouseButton getMouseButton() const { return _mouseButton; }
    int getModifiers() const { return _modifiers; }
    float getX() const { return _x; }
    float getY() const { return _y; }
    float getDeltaX() const { return _deltaX; }
    float getDeltaY() const { return _deltaY; }
    float getOffsetX() const { return _offsetX; }
    float getOffsetY() const { return _offsetY; }

private:
    MouseEventType _mouseEventType;
    MouseButton _mouseButton = MouseButton::BUTTON_UNSET;
    int _modifiers = 0;
    float _x = 0.f;
    float _y = 0.f;
    float _deltaX = 0.f;
    float _deltaY = 0.f;
    float _offsetX = 0.f;
    float _offsetY = 0.f;
};

class EventTouch : public Event {
public:
    enum class EventCode {
        BEGAN,
        MOVED,
        ENDED,
        CANCELLED,
    };

    explicit EventTouch(EventCode eventCode) : Event(Type::Touch), _eventCode(eventCode) {}
    EventTouch(EventCode eventCode, Touch* touch) : EventTouch(eventCode) {
        if (touch) {
            _touches.push_back(touch);
        }
    }

    EventCode getEventCode() const { return _eventCode; }
    const mstd::vector<Touch*>& getTouches() const { return _touches; }
    void addTouch(Touch* touch) {
        if (touch) {
            _touches.push_back(touch);
        }
    }

private:
    EventCode _eventCode;
    mstd::vector<Touch*> _touches;
};

} // namespace zocos

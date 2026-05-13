#pragma once

namespace zocos {

class Node;

class Event {
public:
    enum class Type {
        Keyboard,
        Mouse,
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

    MouseEventType getMouseEventType() const { return _mouseEventType; }

protected:
    explicit EventMouse(MouseEventType mouseEventType)
        : Event(Type::Mouse), _mouseEventType(mouseEventType) {}

private:
    MouseEventType _mouseEventType;
};

class EventMouseButton : public EventMouse {
public:
    EventMouseButton(int button, int modifiers, MouseEventType mouseEventType, float x, float y)
        : EventMouse(mouseEventType), _button(button), _modifiers(modifiers), _x(x), _y(y) {}

    int getButton() const { return _button; }
    int getModifiers() const { return _modifiers; }
    float getX() const { return _x; }
    float getY() const { return _y; }

private:
    int _button = 0;
    int _modifiers = 0;
    float _x = 0.f;
    float _y = 0.f;
};

class EventMouseMove : public EventMouse {
public:
    EventMouseMove(float x, float y, float deltaX, float deltaY)
        : EventMouse(EventMouse::MouseEventType::MOUSE_MOVE), _x(x), _y(y), _deltaX(deltaX),
          _deltaY(deltaY) {}

    float getX() const { return _x; }
    float getY() const { return _y; }
    float getDeltaX() const { return _deltaX; }
    float getDeltaY() const { return _deltaY; }

private:
    float _x = 0.f;
    float _y = 0.f;
    float _deltaX = 0.f;
    float _deltaY = 0.f;
};

class EventMouseScroll : public EventMouse {
public:
    EventMouseScroll(float offsetX, float offsetY, float x, float y)
        : EventMouse(EventMouse::MouseEventType::MOUSE_SCROLL), _offsetX(offsetX),
          _offsetY(offsetY), _x(x), _y(y) {}

    float getOffsetX() const { return _offsetX; }
    float getOffsetY() const { return _offsetY; }
    float getX() const { return _x; }
    float getY() const { return _y; }

private:
    float _offsetX = 0.f;
    float _offsetY = 0.f;
    float _x = 0.f;
    float _y = 0.f;
};

} // namespace zocos

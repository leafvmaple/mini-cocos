#pragma once

namespace zocos {

class ViewDelegate {
public:
    virtual ~ViewDelegate() = default;

    virtual void onViewResized(int width, int height) = 0;
    virtual bool onViewKeyEvent(int keyCode, int scanCode, int modifiers, bool pressed,
                                bool repeated) = 0;
    virtual void onViewMouseButtonEvent(int button, int modifiers, bool pressed, float x,
                                        float y) = 0;
    virtual void onViewMouseMoveEvent(float x, float y, float deltaX, float deltaY) = 0;
    virtual void onViewMouseScrollEvent(float offsetX, float offsetY, float x, float y) = 0;
};

class View {
public:
    virtual ~View() = default;

    virtual bool init(int width, int height, const char* title) = 0;
    virtual void shutdown() = 0;

    virtual bool shouldClose() const = 0;
    virtual void pollEvents() = 0;
    virtual void swapBuffers() = 0;

    virtual int getFramebufferWidth() const = 0;
    virtual int getFramebufferHeight() const = 0;

    virtual bool getMousePosition(float& outX, float& outY) const = 0;
    virtual double getTimeSeconds() const = 0;

    virtual void setDelegate(ViewDelegate* delegate) = 0;
};

} // namespace zocos

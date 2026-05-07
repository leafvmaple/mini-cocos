#pragma once

#include "base/ZCEventDispatcher.h"
#include "2d/ZCScene.h"
#include "base/ZCScheduler.h"

struct GLFWwindow;

namespace zocos {

class Director {
public:
    static Director& getInstance();

    bool init(int width, int height, const char* title);
    void shutdown();

    void runWithScene(Scene* scene);
    void mainLoop();

    Scheduler& getScheduler() { return _scheduler; }
    EventDispatcher& getEventDispatcher() { return _eventDispatcher; }

    bool hasMousePosition() const { return _hasMousePosition; }
    float getMouseX() const { return _mouseX; }
    float getMouseY() const { return _mouseY; }
    void setMousePosition(float x, float y) {
        _mouseX = x;
        _mouseY = y;
        _hasMousePosition = true;
    }

    const Mat4& projectionMatrix() const { return _projection; }
    GLFWwindow* getWindow() const { return _window; }

    int getFramebufferWidth() const { return _fbWidth; }
    int getFramebufferHeight() const { return _fbHeight; }

    void onFramebufferResize(int w, int h);

private:
    Director() = default;

    void updateProjection();

    GLFWwindow* _window = nullptr;
    Scene* _runningScene = nullptr;
    Scheduler _scheduler;
    EventDispatcher _eventDispatcher;
    Mat4 _projection = Mat4::identity();
    int _fbWidth = 0;
    int _fbHeight = 0;
    float _mouseX = 0.f;
    float _mouseY = 0.f;
    bool _hasMousePosition = false;
    double _lastTime = 0.0;
};

} // namespace zocos

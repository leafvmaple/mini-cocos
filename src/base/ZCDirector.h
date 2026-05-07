#pragma once

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
    Mat4 _projection = Mat4::identity();
    int _fbWidth = 0;
    int _fbHeight = 0;
    double _lastTime = 0.0;
};

} // namespace zocos

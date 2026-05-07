#pragma once

#include "2d/ZCNode.h"
#include <memory>

struct GLFWwindow;

namespace zocos {

class ZCDirector {
public:
    static ZCDirector& getInstance();

    bool init(int width, int height, const char* title);
    void shutdown();

    void runWithScene(std::unique_ptr<ZCScene> scene);
    void mainLoop();

    const ZCMat4& projectionMatrix() const { return _projection; }
    GLFWwindow* getWindow() const { return _window; }

    int getFramebufferWidth() const { return _fbWidth; }
    int getFramebufferHeight() const { return _fbHeight; }

    void onFramebufferResize(int w, int h);

private:
    ZCDirector() = default;

    void updateProjection();

    GLFWwindow* _window = nullptr;
    std::unique_ptr<ZCScene> _runningScene;
    ZCMat4 _projection = ZCMat4::identity();
    int _fbWidth = 0;
    int _fbHeight = 0;
    double _lastTime = 0.0;
};

} // namespace zocos

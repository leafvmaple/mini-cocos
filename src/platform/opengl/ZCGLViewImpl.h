#pragma once

#include "base/ZCView.h"

struct GLFWwindow;

namespace zocos {

class GLViewImpl final : public View {
public:
    GLViewImpl() = default;
    ~GLViewImpl() override;

    bool init(int width, int height, const char* title) override;
    void shutdown() override;

    bool shouldClose() const override;
    void pollEvents() override;
    void swapBuffers() override;

    int getFramebufferWidth() const override { return _framebufferWidth; }
    int getFramebufferHeight() const override { return _framebufferHeight; }

    bool getMousePosition(float& outX, float& outY) const override;
    double getTimeSeconds() const override;

    void setDelegate(ViewDelegate* delegate) override { _delegate = delegate; }

    // Internal callback entry points used by GLFW's C callbacks.
    void onFramebufferSize(int width, int height);
    void onKey(int key, int scanCode, int action, int mods);
    void onMouseButton(int button, int action, int mods);
    void onCursorPos(double x, double y);
    void onScroll(double offsetX, double offsetY);

private:
    void toEnginePoint(double x, double y, float& outX, float& outY) const;

    GLFWwindow* _window = nullptr;
    ViewDelegate* _delegate = nullptr;
    int _framebufferWidth = 0;
    int _framebufferHeight = 0;
    float _mouseX = 0.f;
    float _mouseY = 0.f;
    bool _hasMousePosition = false;
};

} // namespace zocos

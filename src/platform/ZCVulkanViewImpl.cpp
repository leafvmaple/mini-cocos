#include "platform/ZCVulkanViewImpl.h"

#define GLFW_INCLUDE_NONE
#include <GLFW/glfw3.h>

namespace zocos {

namespace {

void framebufferSizeCallback(GLFWwindow* window, int width, int height) {
    auto* view = static_cast<VulkanViewImpl*>(glfwGetWindowUserPointer(window));
    if (view) {
        view->onFramebufferSize(width, height);
    }
}

void keyCallback(GLFWwindow* window, int key, int scanCode, int action, int mods) {
    auto* view = static_cast<VulkanViewImpl*>(glfwGetWindowUserPointer(window));
    if (view) {
        view->onKey(key, scanCode, action, mods);
    }
}

void mouseButtonCallback(GLFWwindow* window, int button, int action, int mods) {
    auto* view = static_cast<VulkanViewImpl*>(glfwGetWindowUserPointer(window));
    if (view) {
        view->onMouseButton(button, action, mods);
    }
}

void cursorPosCallback(GLFWwindow* window, double x, double y) {
    auto* view = static_cast<VulkanViewImpl*>(glfwGetWindowUserPointer(window));
    if (view) {
        view->onCursorPos(x, y);
    }
}

void scrollCallback(GLFWwindow* window, double offsetX, double offsetY) {
    auto* view = static_cast<VulkanViewImpl*>(glfwGetWindowUserPointer(window));
    if (view) {
        view->onScroll(offsetX, offsetY);
    }
}

} // namespace

VulkanViewImpl::~VulkanViewImpl() { shutdown(); }

bool VulkanViewImpl::init(int width, int height, const char* title) {
    if (_window) {
        return true;
    }

    if (!glfwInit()) {
        return false;
    }

    glfwWindowHint(GLFW_CLIENT_API, GLFW_NO_API);

    _window = glfwCreateWindow(width, height, title, nullptr, nullptr);
    if (!_window) {
        glfwTerminate();
        return false;
    }

    glfwSetWindowUserPointer(_window, this);
    glfwSetFramebufferSizeCallback(_window, framebufferSizeCallback);
    glfwSetKeyCallback(_window, keyCallback);
    glfwSetMouseButtonCallback(_window, mouseButtonCallback);
    glfwSetCursorPosCallback(_window, cursorPosCallback);
    glfwSetScrollCallback(_window, scrollCallback);

    glfwGetFramebufferSize(_window, &_framebufferWidth, &_framebufferHeight);

    double x = 0.0;
    double y = 0.0;
    glfwGetCursorPos(_window, &x, &y);
    toEnginePoint(x, y, _mouseX, _mouseY);
    _hasMousePosition = true;

    if (_delegate) {
        _delegate->onViewResized(_framebufferWidth, _framebufferHeight);
    }

    return true;
}

void VulkanViewImpl::shutdown() {
    if (_window) {
        glfwDestroyWindow(_window);
        _window = nullptr;
        glfwTerminate();
    }

    _framebufferWidth = 0;
    _framebufferHeight = 0;
    _mouseX = 0.f;
    _mouseY = 0.f;
    _hasMousePosition = false;
}

bool VulkanViewImpl::shouldClose() const { return !_window || glfwWindowShouldClose(_window); }

void VulkanViewImpl::pollEvents() {
    if (_window) {
        glfwPollEvents();
    }
}

void VulkanViewImpl::swapBuffers() {
    // Vulkan presents through the render device, not via GLFW swap buffers.
}

bool VulkanViewImpl::getMousePosition(float& outX, float& outY) const {
    if (!_hasMousePosition) {
        return false;
    }

    outX = _mouseX;
    outY = _mouseY;
    return true;
}

double VulkanViewImpl::getTimeSeconds() const { return glfwGetTime(); }

void VulkanViewImpl::onFramebufferSize(int width, int height) {
    _framebufferWidth = width;
    _framebufferHeight = height;
    if (_delegate) {
        _delegate->onViewResized(width, height);
    }
}

void VulkanViewImpl::onKey(int key, int scanCode, int action, int mods) {
    if (action != GLFW_PRESS && action != GLFW_REPEAT && action != GLFW_RELEASE) {
        return;
    }

    const bool pressed = action != GLFW_RELEASE;
    const bool repeated = action == GLFW_REPEAT;
    bool handled = false;
    if (_delegate) {
        handled = _delegate->onViewKeyEvent(key, scanCode, mods, pressed, repeated);
    }

    if (!handled && key == GLFW_KEY_ESCAPE && action == GLFW_PRESS && _window) {
        glfwSetWindowShouldClose(_window, GLFW_TRUE);
    }
}

void VulkanViewImpl::onMouseButton(int button, int action, int mods) {
    if (action != GLFW_PRESS && action != GLFW_RELEASE) {
        return;
    }

    double x = 0.0;
    double y = 0.0;
    glfwGetCursorPos(_window, &x, &y);

    float px = 0.f;
    float py = 0.f;
    toEnginePoint(x, y, px, py);

    if (_delegate) {
        _delegate->onViewMouseButtonEvent(button, mods, action == GLFW_PRESS, px, py);
    }
}

void VulkanViewImpl::onCursorPos(double x, double y) {
    float px = 0.f;
    float py = 0.f;
    toEnginePoint(x, y, px, py);

    float deltaX = 0.f;
    float deltaY = 0.f;
    if (_hasMousePosition) {
        deltaX = px - _mouseX;
        deltaY = py - _mouseY;
    }

    _mouseX = px;
    _mouseY = py;
    _hasMousePosition = true;

    if (_delegate) {
        _delegate->onViewMouseMoveEvent(px, py, deltaX, deltaY);
    }
}

void VulkanViewImpl::onScroll(double offsetX, double offsetY) {
    double x = 0.0;
    double y = 0.0;
    glfwGetCursorPos(_window, &x, &y);

    float px = 0.f;
    float py = 0.f;
    toEnginePoint(x, y, px, py);

    if (_delegate) {
        _delegate->onViewMouseScrollEvent(static_cast<float>(offsetX), static_cast<float>(offsetY),
                                          px, py);
    }
}

void VulkanViewImpl::toEnginePoint(double x, double y, float& outX, float& outY) const {
    int windowWidth = 0;
    int windowHeight = 0;
    glfwGetWindowSize(_window, &windowWidth, &windowHeight);

    if (windowWidth <= 0 || windowHeight <= 0 || _framebufferWidth <= 0 ||
        _framebufferHeight <= 0) {
        outX = static_cast<float>(x);
        outY = static_cast<float>(y);
        return;
    }

    const double scaleX = static_cast<double>(_framebufferWidth) / static_cast<double>(windowWidth);
    const double scaleY =
        static_cast<double>(_framebufferHeight) / static_cast<double>(windowHeight);
    outX = static_cast<float>(x * scaleX);
    outY = static_cast<float>((static_cast<double>(windowHeight) - y) * scaleY);
}

} // namespace zocos

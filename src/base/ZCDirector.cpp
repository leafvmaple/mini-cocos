#include "base/ZCDirector.h"
#include "base/ZCAutoreleasePool.h"
#include "base/ZCEvent.h"
#include "base/ZCFontCache.h"
#include "base/ZCTextureCache.h"
#include "platform/ZCOpenGLLoader.h"
#include "platform/ZCRenderDeviceGL.h"

#define GLFW_INCLUDE_NONE
#include <GLFW/glfw3.h>

#include <cassert>
#include <memory>

namespace zocos {

static void framebufferSizeCallback(GLFWwindow* win, int w, int h) {
    auto* self = static_cast<Director*>(glfwGetWindowUserPointer(win));
    if (self) self->onFramebufferResize(w, h);
}

static void toEnginePoint(Director* director, GLFWwindow* win, double x, double y, float& outX,
                          float& outY) {
    int winW = 0;
    int winH = 0;
    glfwGetWindowSize(win, &winW, &winH);
    const int fbW = director->getFramebufferWidth();
    const int fbH = director->getFramebufferHeight();
    if (winW <= 0 || winH <= 0 || fbW <= 0 || fbH <= 0) {
        outX = static_cast<float>(x);
        outY = static_cast<float>(y);
        return;
    }

    const double sx = static_cast<double>(fbW) / static_cast<double>(winW);
    const double sy = static_cast<double>(fbH) / static_cast<double>(winH);
    outX = static_cast<float>(x * sx);
    outY = static_cast<float>((static_cast<double>(winH) - y) * sy);
}

static void keyCallback(GLFWwindow* win, int key, int scancode, int action, int mods) {
    auto* self = static_cast<Director*>(glfwGetWindowUserPointer(win));
    if (!self) {
        return;
    }
    if (action != GLFW_PRESS && action != GLFW_REPEAT && action != GLFW_RELEASE) {
        return;
    }

    const bool pressed = action != GLFW_RELEASE;
    const bool repeated = action == GLFW_REPEAT;
    EventKeyboard event(key, scancode, mods, pressed, repeated);
    self->getEventDispatcher().dispatchEvent(event);

    if (!event.isStopped() && key == GLFW_KEY_ESCAPE && action == GLFW_PRESS) {
        glfwSetWindowShouldClose(win, GLFW_TRUE);
    }
}

static void mouseButtonCallback(GLFWwindow* win, int button, int action, int mods) {
    auto* self = static_cast<Director*>(glfwGetWindowUserPointer(win));
    if (!self) {
        return;
    }
    if (action != GLFW_PRESS && action != GLFW_RELEASE) {
        return;
    }

    double x = 0.0;
    double y = 0.0;
    glfwGetCursorPos(win, &x, &y);
    float px = 0.f;
    float py = 0.f;
    toEnginePoint(self, win, x, y, px, py);

    EventMouseButton event(button, mods, action == GLFW_PRESS, px, py);
    self->getEventDispatcher().dispatchEvent(event);
}

static void cursorPosCallback(GLFWwindow* win, double x, double y) {
    auto* self = static_cast<Director*>(glfwGetWindowUserPointer(win));
    if (!self) {
        return;
    }

    float px = 0.f;
    float py = 0.f;
    toEnginePoint(self, win, x, y, px, py);

    float deltaX = 0.f;
    float deltaY = 0.f;
    if (self->hasMousePosition()) {
        deltaX = px - self->getMouseX();
        deltaY = py - self->getMouseY();
    }
    self->setMousePosition(px, py);

    EventMouseMove event(px, py, deltaX, deltaY);
    self->getEventDispatcher().dispatchEvent(event);
}

static void scrollCallback(GLFWwindow* win, double offsetX, double offsetY) {
    auto* self = static_cast<Director*>(glfwGetWindowUserPointer(win));
    if (!self) {
        return;
    }

    double x = 0.0;
    double y = 0.0;
    glfwGetCursorPos(win, &x, &y);
    float px = 0.f;
    float py = 0.f;
    toEnginePoint(self, win, x, y, px, py);

    EventMouseScroll event(static_cast<float>(offsetX), static_cast<float>(offsetY), px, py);
    self->getEventDispatcher().dispatchEvent(event);
}

void Director::onFramebufferResize(int w, int h) {
    _fbWidth = w;
    _fbHeight = h;
    glViewport(0, 0, w, h);
    updateProjection();
}

Director& Director::getInstance() {
    static Director inst;
    return inst;
}

TextureCache& Director::getTextureCache() {
    if (!_textureCache) {
        _textureCache = std::make_unique<TextureCache>();
    }
    return *_textureCache;
}

FontCache& Director::getFontCache() {
    if (!_fontCache) {
        _fontCache = std::make_unique<FontCache>();
    }
    return *_fontCache;
}

bool Director::init(int width, int height, const char* title) {
    if (!glfwInit()) return false;
    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 3);
    glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);
#ifdef __APPLE__
    glfwWindowHint(GLFW_OPENGL_FORWARD_COMPAT, GL_TRUE);
#endif
    _window = glfwCreateWindow(width, height, title, nullptr, nullptr);
    if (!_window) {
        glfwTerminate();
        return false;
    }
    glfwMakeContextCurrent(_window);
    glfwSetWindowUserPointer(_window, this);
    glfwSetFramebufferSizeCallback(_window, framebufferSizeCallback);
    glfwSetKeyCallback(_window, keyCallback);
    glfwSetMouseButtonCallback(_window, mouseButtonCallback);
    glfwSetCursorPosCallback(_window, cursorPosCallback);
    glfwSetScrollCallback(_window, scrollCallback);

    if (!loadOpenGL(reinterpret_cast<void* (*)(const char*)>(glfwGetProcAddress))) {
        shutdown();
        return false;
    }

    _renderDevice = std::make_unique<RenderDeviceGL>();
    _textureCache = std::make_unique<TextureCache>();
    _fontCache = std::make_unique<FontCache>();

    glfwGetFramebufferSize(_window, &_fbWidth, &_fbHeight);
    glViewport(0, 0, _fbWidth, _fbHeight);

    double x = 0.0;
    double y = 0.0;
    glfwGetCursorPos(_window, &x, &y);
    float px = 0.f;
    float py = 0.f;
    toEnginePoint(this, _window, x, y, px, py);
    setMousePosition(px, py);

    updateProjection();
    _lastTime = glfwGetTime();
    return true;
}

void Director::shutdown() {
    if (_runningScene) {
        if (_runningScene->isRunning()) {
            _runningScene->onExit();
        }
        _runningScene->release();
        _runningScene = nullptr;
    }

    assert(_scheduler.getScheduledCount() == 0 && "Scheduled callbacks were not fully released.");
    assert(_actionManager.getRunningActionCount() == 0 && "Actions were not fully released.");
    assert(_eventDispatcher.getListenerCount() == 0 && "Event listeners were not fully released.");

    _actionManager.removeAllActions();
    _eventDispatcher.removeAllListeners();
    _renderer.endFrame();
    if (_fontCache) {
        _fontCache->removeAllFonts();
        _fontCache.reset();
    }
    if (_textureCache) {
        _textureCache->removeAllTextures(*this);
        _textureCache.reset();
    }
    _renderDevice.reset();
    PoolManager::getInstance().clearRootPool();
    if (_window) {
        glfwDestroyWindow(_window);
        _window = nullptr;
    }
    glfwTerminate();
}

void Director::updateProjection() {
    if (_fbWidth <= 0 || _fbHeight <= 0) return;
    _projection = Mat4::ortho(0.f, static_cast<float>(_fbWidth), 0.f, static_cast<float>(_fbHeight), -1.f, 1.f);
}

void Director::runWithScene(Scene* scene) {
    if (_runningScene == scene) {
        return;
    }

    if (_runningScene) {
        if (_runningScene->isRunning()) {
            _runningScene->onExit();
        }
        _runningScene->release();
        _runningScene = nullptr;
    }

    _runningScene = scene;
    if (_runningScene) {
        _runningScene->retain();
        _runningScene->onEnter();
    }

    // Flush startup autoreleased objects; retained objects stay alive.
    PoolManager::getInstance().clearRootPool();
}

void Director::mainLoop() {
    if (!_window || glfwWindowShouldClose(_window)) {
        return;
    }

    AutoreleasePool framePool("frame autorelease pool");

    const double now = glfwGetTime();
    const float dt = static_cast<float>(now - _lastTime);
    _lastTime = now;

    glfwPollEvents();
    _scheduler.update(dt);
    _actionManager.update(dt);
    if (_runningScene) _runningScene->updateTree(dt);

    if (_renderDevice) {
        _renderer.beginFrame(_projection);
        if (_runningScene) {
            _runningScene->visitScene(_renderer);
        }
        _renderer.flush(*_renderDevice, _fbWidth, _fbHeight);
        _renderer.endFrame();
    }

    glfwSwapBuffers(_window);
}

} // namespace zocos

#include "base/ZCDirector.h"
#include "base/ZCAutoreleasePool.h"
#include "base/ZCEvent.h"
#include "base/ZCFontCache.h"
#include "base/ZCPlatformFactory.h"
#include "base/ZCTextureCache.h"

#include <cassert>
#include <memory>

namespace zocos {

void Director::onFramebufferResize(int w, int h) {
    _fbWidth = w;
    _fbHeight = h;
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
    if (!_view) {
        _view = createDefaultView();
    }
    _view->setDelegate(this);
    if (!_view->init(width, height, title)) {
        _view.reset();
        return false;
    }

    _renderDevice = createDefaultRenderDevice(*_view);
    _textureCache = std::make_unique<TextureCache>();
    _fontCache = std::make_unique<FontCache>();

    _fbWidth = _view->getFramebufferWidth();
    _fbHeight = _view->getFramebufferHeight();

    float mouseX = 0.f;
    float mouseY = 0.f;
    if (_view->getMousePosition(mouseX, mouseY)) {
        setMousePosition(mouseX, mouseY);
    }

    updateProjection();
    _lastTime = _view->getTimeSeconds();
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
    if (_view) {
        _view->setDelegate(nullptr);
        _view->shutdown();
        _view.reset();
    }

    _fbWidth = 0;
    _fbHeight = 0;
    _hasMousePosition = false;
}

void Director::updateProjection() {
    if (_fbWidth <= 0 || _fbHeight <= 0)
        return;
    _projection = Mat4::ortho(0.f, static_cast<float>(_fbWidth), 0.f, static_cast<float>(_fbHeight),
                              -1.f, 1.f);
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

bool Director::mainLoop() {
    if (!_view || _view->shouldClose()) {
        return false;
    }

    AutoreleasePool framePool("frame autorelease pool");

    const double now = _view->getTimeSeconds();
    const float dt = static_cast<float>(now - _lastTime);
    _lastTime = now;

    _view->pollEvents();
    _scheduler.update(dt);
    _actionManager.update(dt);
    if (_runningScene)
        _runningScene->updateTree(dt);

    if (_renderDevice) {
        _renderer.beginFrame(_projection);
        if (_runningScene) {
            _runningScene->visitScene(_renderer);
        }
        _renderer.flush(*_renderDevice, _fbWidth, _fbHeight);
        _renderer.endFrame();
    }

    _view->swapBuffers();
    return !_view->shouldClose();
}

void Director::onViewResized(int width, int height) { onFramebufferResize(width, height); }

bool Director::onViewKeyEvent(int keyCode, int scanCode, int modifiers, bool pressed,
                              bool repeated) {
    EventKeyboard event(keyCode, scanCode, modifiers, pressed, repeated);
    _eventDispatcher.dispatchEvent(event);
    return event.isStopped();
}

void Director::onViewMouseButtonEvent(int button, int modifiers, bool pressed, float x, float y) {
    EventMouseButton event(button, modifiers, pressed, x, y);
    _eventDispatcher.dispatchEvent(event);
}

void Director::onViewMouseMoveEvent(float x, float y, float deltaX, float deltaY) {
    setMousePosition(x, y);
    EventMouseMove event(x, y, deltaX, deltaY);
    _eventDispatcher.dispatchEvent(event);
}

void Director::onViewMouseScrollEvent(float offsetX, float offsetY, float x, float y) {
    EventMouseScroll event(offsetX, offsetY, x, y);
    _eventDispatcher.dispatchEvent(event);
}

} // namespace zocos

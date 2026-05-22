#include "base/ZCDirector.h"
#include "base/ZCAutoreleasePool.h"
#include "base/ZCEvent.h"
#include "base/ZCFontAtlasCache.h"
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

FontAtlasCache& Director::getFontAtlasCache() {
    if (!_fontAtlasCache) {
        _fontAtlasCache = std::make_unique<FontAtlasCache>(*this);
    }
    return *_fontAtlasCache;
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
    _fontAtlasCache = std::make_unique<FontAtlasCache>(*this);

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
    _eventDispatcher.removeAllEventListeners();
    _renderer.endFrame();
    if (_fontAtlasCache) {
        _fontAtlasCache->removeAllFontAtlas();
        _fontAtlasCache.reset();
    }
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

    // Flush startup auto released objects; retained objects stay alive.
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

    _renderer.beginFrame(_projection);
    if (_runningScene) {
        _runningScene->render(_renderer);
    }
    _renderer.flush(*_renderDevice, _fbWidth, _fbHeight);
    _renderer.endFrame();

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

void Director::onViewMouseButtonEvent(int button, int modifiers, bool buttonActive, float x,
                                      float y) {
    const auto mouseEventType = buttonActive ? EventMouse::MouseEventType::MOUSE_DOWN
                                             : EventMouse::MouseEventType::MOUSE_UP;
    const auto mouseButton = (button >= static_cast<int>(EventMouse::MouseButton::BUTTON_LEFT) &&
                              button <= static_cast<int>(EventMouse::MouseButton::BUTTON_8))
                                 ? static_cast<EventMouse::MouseButton>(button)
                                 : EventMouse::MouseButton::BUTTON_UNSET;
    EventMouse event(mouseEventType);
    event.setPosition(x, y);
    event.setMouseButton(mouseButton);
    event.setModifiers(modifiers);
    _eventDispatcher.dispatchEvent(event);
}

void Director::onViewMouseMoveEvent(float x, float y, float deltaX, float deltaY) {
    setMousePosition(x, y);
    EventMouse event(EventMouse::MouseEventType::MOUSE_MOVE);
    event.setPosition(x, y);
    event.setDelta(deltaX, deltaY);
    _eventDispatcher.dispatchEvent(event);
}

void Director::onViewMouseScrollEvent(float offsetX, float offsetY, float x, float y) {
    EventMouse event(EventMouse::MouseEventType::MOUSE_SCROLL);
    event.setPosition(x, y);
    event.setOffset(offsetX, offsetY);
    _eventDispatcher.dispatchEvent(event);
}

} // namespace zocos

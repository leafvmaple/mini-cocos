#include "base/ZCDirector.h"
#include "base/ZCAutoreleasePool.h"
#include "base/ZCEvent.h"
#include "base/ZCFontAtlasCache.h"
#include "base/ZCFontCache.h"
#include "base/ZCPlatformFactory.h"
#include "base/ZCTextureCache.h"

#include <cassert>
#include "base/ZCStd.h"

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
        _textureCache = mstd::make_unique<TextureCache>();
    }
    return *_textureCache;
}

FontCache& Director::getFontCache() {
    if (!_fontCache) {
        _fontCache = mstd::make_unique<FontCache>();
    }
    return *_fontCache;
}

FontAtlasCache& Director::getFontAtlasCache() {
    if (!_fontAtlasCache) {
        _fontAtlasCache = mstd::make_unique<FontAtlasCache>(*this);
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
    _textureCache = mstd::make_unique<TextureCache>();
    _fontCache = mstd::make_unique<FontCache>();
    _fontAtlasCache = mstd::make_unique<FontAtlasCache>(*this);

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
    clearPendingSceneOperations();
    cancelSceneTransition();
    Scene* runningScene = _runningScene;
    if (_runningScene) {
        if (_runningScene->isRunning()) {
            _runningScene->onExitTransitionDidStart();
            _runningScene->onExit();
        }
        _runningScene->cleanup();
        _runningScene = nullptr;
    }
    _eventDispatcher.setSceneGraphRoot(nullptr);
    for (auto* scene : _sceneStack) {
        if (scene != runningScene) {
            scene->cleanup();
        }
        scene->release();
    }
    _sceneStack.clear();

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
    _transitionDuration = 0.f;
    _transitionElapsed = 0.f;
    _transitionSceneSwitched = false;
    _insideMainLoop = false;
    _applyingSceneOperations = false;
}

void Director::updateProjection() {
    if (_fbWidth <= 0 || _fbHeight <= 0)
        return;
    _projection = Mat4::ortho(0.f, static_cast<float>(_fbWidth), 0.f, static_cast<float>(_fbHeight),
                              -1.f, 1.f);
}

void Director::runWithScene(Scene* scene) {
    if (!_runningScene) {
        pushScene(scene);
    } else {
        replaceScene(scene);
    }

    // Flush startup auto released objects; retained objects stay alive.
    PoolManager::getInstance().clearRootPool();
}

void Director::setRunningScene(Scene* scene, bool cleanupOutgoing, bool finishIncoming) {
    if (_runningScene == scene) {
        return;
    }

    if (_runningScene) {
        if (_runningScene->isRunning()) {
            _runningScene->onExitTransitionDidStart();
            _runningScene->onExit();
        }
        if (cleanupOutgoing) {
            _runningScene->cleanup();
        }
    }

    _runningScene = scene;
    if (_runningScene) {
        _runningScene->onEnter();
        if (finishIncoming) {
            _runningScene->onEnterTransitionDidFinish();
        }
    }
    _eventDispatcher.setSceneGraphRoot(_runningScene);
}

void Director::replaceScene(Scene* scene, float fadeDuration) {
    if (!scene) {
        return;
    }
    queueSceneOperation(SceneOperationType::Replace, scene, fadeDuration);
}

void Director::pushScene(Scene* scene) {
    if (!scene) {
        return;
    }
    queueSceneOperation(SceneOperationType::Push, scene);
}

void Director::popScene() { queueSceneOperation(SceneOperationType::Pop); }

void Director::popToRootScene() { queueSceneOperation(SceneOperationType::PopToRoot); }

void Director::queueSceneOperation(SceneOperationType type, Scene* scene, float fadeDuration) {
    if (scene) {
        scene->retain();
    }
    _pendingSceneOperations.push_back({type, scene, fadeDuration});

    if (!_insideMainLoop && !_applyingSceneOperations) {
        applyPendingSceneOperations();
    }
}

void Director::applyPendingSceneOperations() {
    if (_applyingSceneOperations) {
        return;
    }

    _applyingSceneOperations = true;
    while (!_pendingSceneOperations.empty()) {
        auto operations = mstd::move(_pendingSceneOperations);
        _pendingSceneOperations.clear();

        for (auto& operation : operations) {
            switch (operation.type) {
            case SceneOperationType::Replace:
                cancelSceneTransition();
                if (_runningScene && operation.fadeDuration > 0.f) {
                    beginSceneTransition(operation.scene, operation.fadeDuration);
                } else {
                    replaceSceneNow(operation.scene);
                }
                break;
            case SceneOperationType::Push:
                cancelSceneTransition();
                pushSceneNow(operation.scene);
                break;
            case SceneOperationType::Pop:
                cancelSceneTransition();
                popSceneNow();
                break;
            case SceneOperationType::PopToRoot:
                cancelSceneTransition();
                popToRootSceneNow();
                break;
            }

            if (operation.scene) {
                operation.scene->release();
            }
        }
    }
    _applyingSceneOperations = false;
}

void Director::clearPendingSceneOperations() {
    for (auto& operation : _pendingSceneOperations) {
        if (operation.scene) {
            operation.scene->release();
        }
    }
    _pendingSceneOperations.clear();
}

void Director::pushSceneNow(Scene* scene) {
    if (!scene || scene == _runningScene) {
        return;
    }

    scene->retain();
    _sceneStack.push_back(scene);
    setRunningScene(scene, false);
}

void Director::popSceneNow() {
    if (_sceneStack.empty()) {
        return;
    }

    Scene* outgoing = _sceneStack.back();
    Scene* incoming = _sceneStack.size() > 1 ? _sceneStack[_sceneStack.size() - 2] : nullptr;
    setRunningScene(incoming, true);
    _sceneStack.pop_back();
    outgoing->release();
}

void Director::popToRootSceneNow() {
    if (_sceneStack.size() <= 1) {
        return;
    }

    Scene* root = _sceneStack.front();
    Scene* outgoing = _runningScene;
    setRunningScene(root, true);

    for (mstd::size_t i = 1; i < _sceneStack.size(); ++i) {
        Scene* scene = _sceneStack[i];
        if (scene != outgoing) {
            scene->cleanup();
        }
        scene->release();
    }
    _sceneStack.resize(1);
}

void Director::cancelSceneTransition() {
    if (_transitionSceneSwitched && _runningScene && _runningScene->isRunning()) {
        _runningScene->onEnterTransitionDidFinish();
    }
    if (_nextScene) {
        _nextScene->release();
        _nextScene = nullptr;
    }
    _transitionDuration = 0.f;
    _transitionElapsed = 0.f;
    _transitionSceneSwitched = false;
}

void Director::beginSceneTransition(Scene* scene, float fadeDuration) {
    if (!scene || scene == _runningScene || fadeDuration <= 0.f) {
        replaceSceneNow(scene);
        return;
    }

    _nextScene = scene;
    _nextScene->retain();
    _transitionDuration = fadeDuration;
    _transitionElapsed = 0.f;
    _transitionSceneSwitched = false;
}

void Director::replaceSceneNow(Scene* scene, bool finishIncoming) {
    if (!scene || scene == _runningScene) {
        return;
    }

    scene->retain();
    Scene* outgoing = _sceneStack.empty() ? nullptr : _sceneStack.back();
    setRunningScene(scene, true, finishIncoming);
    if (_sceneStack.empty()) {
        _sceneStack.push_back(scene);
    } else {
        _sceneStack.back() = scene;
        outgoing->release();
    }
}

void Director::updateSceneTransition(float dt) {
    if (!_nextScene) {
        return;
    }

    _transitionElapsed += mstd::max(dt, 0.f);
    const float halfway = _transitionDuration * 0.5f;
    if (!_transitionSceneSwitched && _transitionElapsed >= halfway) {
        replaceSceneNow(_nextScene, false);
        _transitionSceneSwitched = true;
    }

    if (_transitionElapsed >= _transitionDuration) {
        if (_transitionSceneSwitched && _runningScene && _runningScene->isRunning()) {
            _runningScene->onEnterTransitionDidFinish();
        }
        _nextScene->release();
        _nextScene = nullptr;
        _transitionDuration = 0.f;
        _transitionElapsed = 0.f;
        _transitionSceneSwitched = false;
    }
}

bool Director::mainLoop() {
    if (!_view || _view->shouldClose()) {
        return false;
    }

    AutoreleasePool framePool("frame autorelease pool");
    _insideMainLoop = true;

    const double now = _view->getTimeSeconds();
    const float dt = static_cast<float>(now - _lastTime);
    _lastTime = now;

    _view->pollEvents();
    _scheduler.update(dt);
    _actionManager.update(dt);
    applyPendingSceneOperations();
    updateSceneTransition(dt);
    if (_runningScene)
        _runningScene->updateTree(dt);

    _renderer.beginFrame(_projection);
    if (_runningScene) {
        if (_nextScene) {
            const float progress = mstd::clamp(_transitionElapsed / _transitionDuration, 0.f, 1.f);
            const float opacity =
                _transitionSceneSwitched ? (progress - 0.5f) * 2.f : 1.f - progress * 2.f;
            _renderer.setGlobalOpacity(opacity);
        }
        _runningScene->render(_renderer);
        _renderer.setGlobalOpacity(1.f);
    }
    _renderer.flush(*_renderDevice, _fbWidth, _fbHeight);
    _renderer.endFrame();

    _view->swapBuffers();
    _insideMainLoop = false;
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

#pragma once

#include "base/ZCActionManager.h"
#include "base/ZCEventDispatcher.h"
#include "base/ZCView.h"
#include "base/ZCRenderDevice.h"
#include "base/ZCRenderer.h"
#include "2d/ZCScene.h"
#include "base/ZCScheduler.h"

#include "base/ZCStd.h"

namespace zocos {

class TextureCache;
class FontCache;
class FontAtlasCache;

class Director : public ViewDelegate {
public:
    static Director& getInstance();

    bool init(int width, int height, const char* title);
    void shutdown();

    void runWithScene(Scene* scene);
    void replaceScene(Scene* scene, float fadeDuration = 0.f);
    Scene* getRunningScene() const { return _runningScene; }
    bool isSceneTransitioning() const { return _nextScene != nullptr; }

    bool mainLoop();

    Scheduler& getScheduler() { return _scheduler; }
    ActionManager& getActionManager() { return _actionManager; }
    EventDispatcher& getEventDispatcher() { return _eventDispatcher; }
    Renderer& getRenderer() { return _renderer; }
    RenderDevice* getRenderDevice() const { return _renderDevice.get(); }
    TextureCache& getTextureCache();
    FontCache& getFontCache();
    FontAtlasCache& getFontAtlasCache();

    bool hasMousePosition() const { return _hasMousePosition; }
    float getMouseX() const { return _mouseX; }
    float getMouseY() const { return _mouseY; }
    void setMousePosition(float x, float y) {
        _mouseX = x;
        _mouseY = y;
        _hasMousePosition = true;
    }

    const Mat4& projectionMatrix() const { return _projection; }

    int getFramebufferWidth() const { return _fbWidth; }
    int getFramebufferHeight() const { return _fbHeight; }

private:
    Director() = default;

    void onFramebufferResize(int w, int h);
    void updateProjection();
    void setRunningScene(Scene* scene);
    void updateSceneTransition(float dt);

    void onViewResized(int width, int height) override;
    bool onViewKeyEvent(int keyCode, int scanCode, int modifiers, bool pressed,
                        bool repeated) override;
    void onViewMouseButtonEvent(int button, int modifiers, bool buttonActive, float x,
                                float y) override;
    void onViewMouseMoveEvent(float x, float y, float deltaX, float deltaY) override;
    void onViewMouseScrollEvent(float offsetX, float offsetY, float x, float y) override;

    mstd::unique_ptr<View> _view;
    Scene* _runningScene = nullptr;
    Scene* _nextScene = nullptr;
    float _transitionDuration = 0.f;
    float _transitionElapsed = 0.f;
    bool _transitionSceneSwitched = false;
    Scheduler _scheduler;
    ActionManager _actionManager;
    EventDispatcher _eventDispatcher;
    Renderer _renderer;
    mstd::unique_ptr<RenderDevice> _renderDevice;
    mstd::unique_ptr<TextureCache> _textureCache;
    mstd::unique_ptr<FontCache> _fontCache;
    mstd::unique_ptr<FontAtlasCache> _fontAtlasCache;
    Mat4 _projection = Mat4::identity();
    int _fbWidth = 0;
    int _fbHeight = 0;
    float _mouseX = 0.f;
    float _mouseY = 0.f;
    bool _hasMousePosition = false;
    double _lastTime = 0.0;
};

} // namespace zocos

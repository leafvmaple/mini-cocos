// Demo: compare with cocos2d-x flow — Application / Director::runWithScene / Scene / Node / Sprite.
#include "base/ZCApplication.h"
#include "base/ZCAction.h"
#include "base/ZCDirector.h"
#include "base/ZCEventListener.h"
#include "base/ZCRef.h"
#include "2d/ZCNode.h"
#include "2d/ZCScene.h"
#include "2d/ZCLabel.h"
#include "2d/ZCSprite.h"

#define GLFW_INCLUDE_NONE
#include <GLFW/glfw3.h>

#include <algorithm>
#include <cassert>
#include <cmath>
#include <cstdio>
#include <new>

using namespace zocos;

class DemoSprite : public Sprite {
public:
    static DemoSprite* create(Director& director) {
        auto* sprite = new (std::nothrow) DemoSprite(director);
        if (sprite && sprite->init()) {
            sprite->autorelease();
            return sprite;
        }
        delete sprite;
        return nullptr;
    }

    ~DemoSprite() override {
        assert(s_liveCount > 0);
        --s_liveCount;
    }

    void update(float dt) override {
        float vx = 0.f;
        float vy = 0.f;
        if (_moveLeft) vx -= 1.f;
        if (_moveRight) vx += 1.f;
        if (_moveDown) vy -= 1.f;
        if (_moveUp) vy += 1.f;

        if (vx != 0.f || vy != 0.f) {
            if (vx != 0.f && vy != 0.f) {
                constexpr float kInvSqrt2 = 0.70710678f;
                vx *= kInvSqrt2;
                vy *= kInvSqrt2;
            }
            const float speed = _sprint ? _moveSpeed * 1.8f : _moveSpeed;
            setPosition(getPosition().x + vx * speed * dt, getPosition().y + vy * speed * dt);
        }
    }

    void onKeyPressed(int keyCode) {
        switch (keyCode) {
        case GLFW_KEY_A:
        case GLFW_KEY_LEFT:
            _moveLeft = true;
            break;
        case GLFW_KEY_D:
        case GLFW_KEY_RIGHT:
            _moveRight = true;
            break;
        case GLFW_KEY_W:
        case GLFW_KEY_UP:
            _moveUp = true;
            break;
        case GLFW_KEY_S:
        case GLFW_KEY_DOWN:
            _moveDown = true;
            break;
        case GLFW_KEY_LEFT_SHIFT:
        case GLFW_KEY_RIGHT_SHIFT:
            _sprint = true;
            break;
        default:
            break;
        }
    }

    void onKeyReleased(int keyCode) {
        switch (keyCode) {
        case GLFW_KEY_A:
        case GLFW_KEY_LEFT:
            _moveLeft = false;
            break;
        case GLFW_KEY_D:
        case GLFW_KEY_RIGHT:
            _moveRight = false;
            break;
        case GLFW_KEY_W:
        case GLFW_KEY_UP:
            _moveUp = false;
            break;
        case GLFW_KEY_S:
        case GLFW_KEY_DOWN:
            _moveDown = false;
            break;
        case GLFW_KEY_LEFT_SHIFT:
        case GLFW_KEY_RIGHT_SHIFT:
            _sprint = false;
            break;
        default:
            break;
        }
    }

    bool containsPoint(float x, float y) const {
        const auto& size = getContentSize();
        const auto& scale = getScale();
        const auto& anchor = getAnchorPoint();
        const auto& position = getPosition();

        const float width = size.width * std::abs(scale.x);
        const float height = size.height * std::abs(scale.y);
        const float left = position.x - anchor.x * width;
        const float bottom = position.y - anchor.y * height;
        const float right = left + width;
        const float top = bottom + height;
        return x >= left && x <= right && y >= bottom && y <= top;
    }

    void beginDrag(float x, float y) {
        _dragging = true;
        _dragOffsetX = getPosition().x - x;
        _dragOffsetY = getPosition().y - y;
    }

    void dragTo(float x, float y) {
        if (!_dragging) {
            return;
        }
        setPosition(x + _dragOffsetX, y + _dragOffsetY);
    }

    void endDrag() { _dragging = false; }
    bool isDragging() const { return _dragging; }

    void applyScroll(float scrollY) {
        float uniformScale = getScale().x + scrollY * 0.08f;
        uniformScale = std::clamp(uniformScale, 0.25f, 4.0f);
        setScale(uniformScale);
    }

    static int getLiveCount() { return s_liveCount; }

protected:
    explicit DemoSprite(Director& d) : Sprite(d) {
        ++s_liveCount;
    }

private:
    static int s_liveCount;

    bool _moveLeft = false;
    bool _moveRight = false;
    bool _moveUp = false;
    bool _moveDown = false;
    bool _sprint = false;

    bool _dragging = false;
    float _dragOffsetX = 0.f;
    float _dragOffsetY = 0.f;
    float _moveSpeed = 320.f;
};

int DemoSprite::s_liveCount = 0;

int main(int argc, char** argv) {
    Director& dir = Director::getInstance();
    if (!dir.init(960, 540, "zocos (learn cocos2d-x)")) {
        std::fprintf(stderr, "Director::init failed\n");
        return 1;
    }

    auto* scene = Scene::create();
    if (!scene) {
        std::fprintf(stderr, "Scene::create failed\n");
        dir.shutdown();
        return 1;
    }

    auto* child = DemoSprite::create(dir);
    if (!child) {
        std::fprintf(stderr, "DemoSprite::create failed\n");
        dir.shutdown();
        return 1;
    }

    if (argc >= 2) {
        if (!child->initWithFile(argv[1])) {
            std::fprintf(stderr, "Falling back to checkerboard (could not load %s)\n", argv[1]);
            child->initWithCheckerboard();
        }
    } else {
        child->initWithCheckerboard();
    }
    const float centerX = static_cast<float>(dir.getFramebufferWidth()) * 0.5f;
    const float centerY = static_cast<float>(dir.getFramebufferHeight()) * 0.5f;
    const float orbitRadius = std::min(centerX, centerY) * 0.5f;
    child->setPosition(centerX + orbitRadius, centerY);

    // Action demo: orbit around screen center and spin itself continuously.
    child->runAction(RepeatForever::create(Sequence::create(
        {MoveTo::create(0.8f, Vec2{centerX, centerY + orbitRadius}),
            MoveTo::create(0.8f, Vec2{centerX - orbitRadius, centerY}),
            MoveTo::create(0.8f, Vec2{centerX, centerY - orbitRadius}),
            MoveTo::create(0.8f, Vec2{centerX + orbitRadius, centerY})})));
    child->runAction(RepeatForever::create(RotateBy::create(1.0f, 360.f)));

    scene->addChild(child);

    auto* fpsLabel = Label::create(dir, "FPS: 0.0");
    if (!fpsLabel) {
        std::fprintf(stderr, "Label::create failed\n");
        dir.shutdown();
        return 1;
    }
    fpsLabel->setAnchorPoint({0.f, 0.f});
    fpsLabel->setPosition(12.f, 12.f);
    // fpsLabel->setScale(3.f);
    scene->addChild(fpsLabel);

    float fpsAccumTime = 0.f;
    int fpsAccumFrames = 0;
    scene->schedule("demo_fps_label", [fpsLabel, &fpsAccumTime, &fpsAccumFrames](float dt) {
        fpsAccumTime += dt;
        ++fpsAccumFrames;
        if (fpsAccumTime < 0.25f) {
            return;
        }

        const float fps = static_cast<float>(fpsAccumFrames) / fpsAccumTime;
        char text[32];
        std::snprintf(text, sizeof(text), "FPS: %.1f", fps);
        fpsLabel->setString(text);
        fpsAccumTime = 0.f;
        fpsAccumFrames = 0;
    });

    auto* keyboardListener = EventListenerKeyboard::create();
    if (!keyboardListener) {
        std::fprintf(stderr, "EventListenerKeyboard::create failed\n");
        dir.shutdown();
        return 1;
    }
    keyboardListener->onKeyPressed = [&dir, child](EventKeyboard& event) {
        if (event.getKeyCode() == GLFW_KEY_ESCAPE) {
            if (!event.isRepeated()) {
                glfwSetWindowShouldClose(dir.getWindow(), GLFW_TRUE);
            }
            event.stopPropagation();
            return;
        }
        child->onKeyPressed(event.getKeyCode());
    };
    keyboardListener->onKeyReleased = [child](EventKeyboard& event) {
        child->onKeyReleased(event.getKeyCode());
    };
    dir.getEventDispatcher().addEventListener(keyboardListener, child, -200);

    auto* mouseListener = EventListenerMouse::create();
    if (!mouseListener) {
        std::fprintf(stderr, "EventListenerMouse::create failed\n");
        dir.shutdown();
        return 1;
    }
    mouseListener->onMouseDown = [child](EventMouseButton& event) {
        if (event.getButton() == GLFW_MOUSE_BUTTON_LEFT && child->containsPoint(event.getX(), event.getY())) {
            child->beginDrag(event.getX(), event.getY());
            event.stopPropagation();
        }
    };
    mouseListener->onMouseUp = [child](EventMouseButton& event) {
        if (event.getButton() == GLFW_MOUSE_BUTTON_LEFT) {
            child->endDrag();
        }
    };
    mouseListener->onMouseMove = [child](EventMouseMove& event) {
        if (child->isDragging()) {
            child->dragTo(event.getX(), event.getY());
            event.stopPropagation();
        }
    };
    mouseListener->onMouseScroll = [child](EventMouseScroll& event) {
        if (child->isDragging() || child->containsPoint(event.getX(), event.getY())) {
            child->applyScroll(event.getOffsetY());
            event.stopPropagation();
        }
    };
    dir.getEventDispatcher().addEventListener(mouseListener, child, -100);

    dir.runWithScene(scene);
    Application app;
    const int exitCode = app.run();
    dir.shutdown();

    assert(DemoSprite::getLiveCount() == 0 && "DemoSprite instances were not released.");
    assert(Ref::getLiveCount() == 0 && "Ref-managed objects leaked.");

    return exitCode;
}

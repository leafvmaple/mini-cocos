// Demo: compare with cocos2d-x flow — Application / Director::runWithScene / Scene / Node / Sprite.
#include "base/ZCDirector.h"
#include "2d/ZCNode.h"
#include "2d/ZCScene.h"
#include "2d/ZCSprite.h"

#include <cstdio>
#include <new>

using namespace zocos;

class DemoSprite : public Sprite {
public:
    static DemoSprite* create(Director& director) {
        auto* sprite = new (std::nothrow) DemoSprite(director);
        if (!sprite) {
            return nullptr;
        }
        if (sprite->init()) {
            return static_cast<DemoSprite*>(sprite->autorelease());
        }
        delete sprite;
        return nullptr;
    }

    void update(float dt) override { _angle += 45.f * dt; setRotation(_angle); }

protected:
    explicit DemoSprite(Director& d) : Sprite(d) {}

private:
    float _angle = 0.f;
};

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
    child->setPosition(static_cast<float>(dir.getFramebufferWidth()) * 0.5f,
        static_cast<float>(dir.getFramebufferHeight()) * 0.5f);

    scene->addChild(child);

    dir.runWithScene(scene);
    dir.mainLoop();
    dir.shutdown();
    return 0;
}

// Demo: compare with cocos2d-x flow — Application / ZCDirector::runWithScene / ZCScene / ZCNode / ZCSprite.
#include "base/ZCDirector.h"
#include "2d/ZCNode.h"
#include "2d/ZCSprite.h"

#include <cstdio>
#include <memory>

using namespace zocos;

class DemoZCSprite : public ZCSprite {
public:
    explicit DemoZCSprite(ZCDirector& d) : ZCSprite(d) {}
    void update(float dt) override { _angle += 45.f * dt; setRotation(_angle); }

private:
    float _angle = 0.f;
};

int main(int argc, char** argv) {
    ZCDirector& dir = ZCDirector::getInstance();
    if (!dir.init(960, 540, "zocos (learn cocos2d-x)")) {
        std::fprintf(stderr, "ZCDirector::init failed\n");
        return 1;
    }

    auto scene = std::make_unique<ZCScene>();

    auto child = std::make_unique<DemoZCSprite>(dir);
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

    scene->addChild(std::move(child));

    dir.runWithScene(std::move(scene));
    dir.mainLoop();
    dir.shutdown();
    return 0;
}

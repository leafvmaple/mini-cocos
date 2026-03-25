// Demo: compare with cocos2d-x flow - Application / Director::runWithScene / Scene / Node / Sprite.
#include "Director.h"
#include "Node.h"
#include "Sprite.h"

#include <cstdio>
#include <memory>

using namespace mini;

class DemoSprite : public Sprite {
public:
  explicit DemoSprite(Director& d) : Sprite(d) {}
  void update(float dt) override { _angle += 45.f * dt; setRotation(_angle); }

private:
  float _angle = 0.f;
};

int main(int argc, char** argv) {
  Director& dir = Director::getInstance();
  if (!dir.init(960, 540, "mini-cocos (learn cocos2d-x)")) {
    std::fprintf(stderr, "Director::init failed\n");
    return 1;
  }

  auto scene = std::make_unique<Scene>();

  auto child = std::make_unique<DemoSprite>(dir);
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

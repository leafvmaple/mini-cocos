#include "2d/ZCScene.h"

#include <new>

namespace zocos {

Scene* Scene::create() {
    auto* scene = new (std::nothrow) Scene();
    if (scene && scene->init()) {
        scene->autorelease();
        return scene;
    }
    delete scene;
    return nullptr;
}

void Scene::visitScene() {
    const Mat4 identity = Mat4::identity();
    visit(identity);
}

} // namespace zocos

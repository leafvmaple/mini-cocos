#include "2d/ZCScene.h"

#include <new>

namespace zocos {

Scene* Scene::create() {
    auto* scene = new (std::nothrow) Scene();
    if (!scene) {
        return nullptr;
    }
    if (scene->init()) {
        return static_cast<Scene*>(scene->autorelease());
    }
    delete scene;
    return nullptr;
}

void Scene::visitScene() {
    const Mat4 identity = Mat4::identity();
    visit(identity);
}

} // namespace zocos

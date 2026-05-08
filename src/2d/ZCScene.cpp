#include "2d/ZCScene.h"

#include "base/ZCRenderer.h"

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

void Scene::visitScene(Renderer& renderer) {
    const Mat4 identity = Mat4::identity();
    visit(renderer, identity);
}

} // namespace zocos

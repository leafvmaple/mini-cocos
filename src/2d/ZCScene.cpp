#include "2d/ZCScene.h"

#include "base/ZCRenderer.h"

#include "base/ZCStd.h"

namespace zocos {

Scene* Scene::create() {
    auto* scene = new (mstd::nothrow) Scene();
    if (scene && scene->init()) {
        scene->autorelease();
        return scene;
    }
    delete scene;
    return nullptr;
}

void Scene::render(Renderer& renderer) {
    const Mat4 identity = Mat4::identity();
    visit(renderer, identity);
}

} // namespace zocos

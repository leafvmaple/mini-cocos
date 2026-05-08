#pragma once

#include "2d/ZCNode.h"

namespace zocos {

class Renderer;

class Scene : public Node {
public:
    static Scene* create();

    void visitScene(Renderer& renderer);

protected:
    Scene() = default;
};

} // namespace zocos

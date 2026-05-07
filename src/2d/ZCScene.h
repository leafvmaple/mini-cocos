#pragma once

#include "2d/ZCNode.h"

namespace zocos {

class Scene : public Node {
public:
    static Scene* create();

    void visitScene();

protected:
    Scene() = default;
};

} // namespace zocos

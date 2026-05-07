#include "2d/ZCNode.h"

namespace zocos {

ZCNode::~ZCNode() = default;

void ZCNode::addChild(std::unique_ptr<ZCNode> child) {
    if (!child) return;
    child->_parent = this;
    _children.push_back(std::move(child));
}

void ZCNode::updateTree(float dt) {
    update(dt);
    for (auto& ch : _children) {
        if (ch) ch->updateTree(dt);
    }
}

void ZCNode::visit(const ZCMat4& parentWorld) {
    if (!_visible) return;
    const ZCMat4 world = parentWorld * localMatrix();
    draw(world);
    for (auto& ch : _children) {
        if (ch) ch->visit(world);
    }
}

void ZCScene::visitScene() {
    const ZCMat4 identity = ZCMat4::identity();
    visit(identity);
}

} // namespace zocos

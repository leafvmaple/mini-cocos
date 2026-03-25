#include "Node.h"

namespace mini {

Node::~Node() = default;

void Node::addChild(std::unique_ptr<Node> child) {
  if (!child) return;
  child->_parent = this;
  _children.push_back(std::move(child));
}

void Node::updateTree(float dt) {
  update(dt);
  for (auto& ch : _children) {
    if (ch) ch->updateTree(dt);
  }
}

void Node::visit(const Mat4& parentWorld) {
  if (!_visible) return;
  const Mat4 world = parentWorld * localMatrix();
  draw(world);
  for (auto& ch : _children) {
    if (ch) ch->visit(world);
  }
}

void Scene::visitScene() {
  const Mat4 identity = Mat4::identity();
  visit(identity);
}

} // namespace mini

#pragma once

#include "MiniMath.h"
#include <memory>
#include <vector>

namespace mini {

class Node {
public:
  virtual ~Node();

  void setPosition(const Vec2& p) { _position = p; }
  void setPosition(float x, float y) { _position = {x, y}; }
  const Vec2& getPosition() const { return _position; }

  void setScale(float s) { _scale = {s, s}; }
  void setScale(const Vec2& s) { _scale = s; }
  const Vec2& getScale() const { return _scale; }

  void setRotation(float degrees) { _rotation = degrees; }
  float getRotation() const { return _rotation; }

  void setAnchorPoint(const Vec2& a) { _anchorPoint = a; }
  const Vec2& getAnchorPoint() const { return _anchorPoint; }

  void setContentSize(const Size& s) { _contentSize = s; }
  const Size& getContentSize() const { return _contentSize; }

  void setVisible(bool v) { _visible = v; }
  bool isVisible() const { return _visible; }

  void setTag(int t) { _tag = t; }
  int getTag() const { return _tag; }

  void addChild(std::unique_ptr<Node> child);
  const std::vector<std::unique_ptr<Node>>& getChildren() const { return _children; }

  Node* getParent() const { return _parent; }

  virtual void update(float /*dt*/) {}

  void updateTree(float dt);

  void visit(const Mat4& parentWorld);

protected:
  virtual void draw(const Mat4& /*world*/) {}

  Mat4 localMatrix() const {
    return nodeLocalMatrix(_position, _scale, _rotation, _anchorPoint, _contentSize);
  }

  Node* _parent = nullptr;
  std::vector<std::unique_ptr<Node>> _children;

  Vec2 _position{};
  Vec2 _scale{1.f, 1.f};
  float _rotation = 0.f;
  Vec2 _anchorPoint{0.5f, 0.5f};
  Size _contentSize{};
  bool _visible = true;
  int _tag = -1;
};

class Scene : public Node {
public:
  void visitScene();
};

} // namespace mini

#pragma once

#include "math/ZCMath.h"
#include <memory>
#include <vector>

namespace zocos {

class ZCNode {
public:
    virtual ~ZCNode();

    void setPosition(const ZCVec2& p) { _position = p; }
    void setPosition(float x, float y) { _position = {x, y}; }
    const ZCVec2& getPosition() const { return _position; }

    void setScale(float s) { _scale = {s, s}; }
    void setScale(const ZCVec2& s) { _scale = s; }
    const ZCVec2& getScale() const { return _scale; }

    void setRotation(float degrees) { _rotation = degrees; }
    float getRotation() const { return _rotation; }

    void setAnchorPoint(const ZCVec2& a) { _anchorPoint = a; }
    const ZCVec2& getAnchorPoint() const { return _anchorPoint; }

    void setContentSize(const ZCSize& s) { _contentSize = s; }
    const ZCSize& getContentSize() const { return _contentSize; }

    void setVisible(bool v) { _visible = v; }
    bool isVisible() const { return _visible; }

    void setTag(int t) { _tag = t; }
    int getTag() const { return _tag; }

    void addChild(std::unique_ptr<ZCNode> child);
    const std::vector<std::unique_ptr<ZCNode>>& getChildren() const { return _children; }

    ZCNode* getParent() const { return _parent; }

    virtual void update(float /*dt*/) {}

    void updateTree(float dt);

    void visit(const ZCMat4& parentWorld);

protected:
    virtual void draw(const ZCMat4& /*world*/) {}

    ZCMat4 localMatrix() const {
        return zcNodeLocalMatrix(_position, _scale, _rotation, _anchorPoint, _contentSize);
    }

    ZCNode* _parent = nullptr;
    std::vector<std::unique_ptr<ZCNode>> _children;

    ZCVec2 _position{};
    ZCVec2 _scale{1.f, 1.f};
    float _rotation = 0.f;
    ZCVec2 _anchorPoint{0.5f, 0.5f};
    ZCSize _contentSize{};
    bool _visible = true;
    int _tag = -1;
};

class ZCScene : public ZCNode {
public:
    void visitScene();
};

} // namespace zocos

#pragma once

#include "base/ZCRef.h"
#include "math/ZCMath.h"
#include "base/ZCStd.h"

namespace zocos {

class Renderer;
class Action;

class Node : public Ref {
public:
    using ScheduleCallback = mstd::function<void(float)>;
    static constexpr int RepeatForever = -1;

    static Node* create();

    virtual ~Node();

    virtual bool init() { return true; }

    void setPosition(const Vec2& p) {
        _position = p;
        _transformDirty = true;
    }
    void setPosition(float x, float y) {
        _position = {x, y};
        _transformDirty = true;
    }
    const Vec2& getPosition() const { return _position; }

    void setScale(float s) {
        _scale = {s, s};
        _transformDirty = true;
    }
    void setScale(const Vec2& s) {
        _scale = s;
        _transformDirty = true;
    }
    const Vec2& getScale() const { return _scale; }

    void setRotation(float degrees) {
        _rotation = degrees;
        _transformDirty = true;
    }
    float getRotation() const { return _rotation; }

    void setAnchorPoint(const Vec2& a) {
        _anchorPoint = a;
        _transformDirty = true;
    }
    const Vec2& getAnchorPoint() const { return _anchorPoint; }

    void setContentSize(const Size& s) {
        _contentSize = s;
        _transformDirty = true;
    }
    const Size& getContentSize() const { return _contentSize; }

    void setVisible(bool v) { _visible = v; }
    bool isVisible() const { return _visible; }

    void setOpacity(float opacity) {
        if (opacity < 0.f) {
            _opacity = 0.f;
        } else if (opacity > 1.f) {
            _opacity = 1.f;
        } else {
            _opacity = opacity;
        }
    }
    float getOpacity() const { return _opacity; }

    void setTag(int t) { _tag = t; }
    int getTag() const { return _tag; }

    void setLocalZOrder(int localZOrder);
    int getLocalZOrder() const { return _localZOrder; }

    bool isRunning() const { return _running; }
    bool isPaused() const { return _paused; }

    void pause() { _paused = true; }
    void resume() { _paused = false; }

    virtual void onEnter();
    virtual void onExit();
    virtual void cleanup();

    void schedule(const mstd::string& key, ScheduleCallback callback, float interval = 0.f,
                  int repeat = RepeatForever, float delay = 0.f, int priority = 0);
    void scheduleOnce(const mstd::string& key, ScheduleCallback callback, float delay = 0.f,
                      int priority = 0);
    void unschedule(const mstd::string& key);
    void unscheduleAllCallbacks();

    Action* runAction(Action* action);
    void stopAction(Action* action);
    void stopAllActions();

    void addChild(Node* child);
    void addChild(Node* child, int localZOrder);
    void addChild(Node* child, int localZOrder, int tag);
    void reorderChild(Node* child, int localZOrder);
    Node* getChildByTag(int tag) const;
    void removeChild(Node* child, bool cleanup = true);
    void removeChildByTag(int tag, bool cleanup = true);
    void removeAllChildren();
    void removeAllChildrenWithCleanup(bool cleanup);
    void removeFromParent();
    void removeFromParentAndCleanup(bool cleanup);
    const mstd::vector<Node*>& getChildren() const { return _children; }
    void sortAllChildren();

    Node* getParent() const { return _parent; }
    const Mat4& getNodeToParentTransform() const {
        if (_transformDirty) {
            _localMatrix =
                zcNodeLocalMatrix(_position, _scale, _rotation, _anchorPoint, _contentSize);
            _transformDirty = false;
        }
        return _localMatrix;
    }
    Mat4 getNodeToWorldTransform() const;
    Vec2 convertToWorldSpace(const Vec2& nodePoint) const;
    bool convertToNodeSpace(const Vec2& worldPoint, Vec2& nodePoint) const;

    virtual void update(float /*dt*/) {}

    void updateTree(float dt);

    void visit(Renderer& renderer, const Mat4& parentWorld);

protected:
    Node() = default;

    virtual void draw(Renderer& /*renderer*/, const Mat4& /*world*/) {}

    Node* _parent = nullptr;
    mstd::vector<Node*> _children;

    bool _running = false;
    bool _paused = false;

    Vec2 _position{};
    Vec2 _scale{1.f, 1.f};
    float _rotation = 0.f;
    Vec2 _anchorPoint{0.5f, 0.5f};
    Size _contentSize{};
    bool _visible = true;
    float _opacity = 1.f;
    int _tag = -1;
    int _localZOrder = 0;
    mstd::size_t _orderOfArrival = 0;
    bool _reorderChildDirty = false;

    // Lazily rebuilt node-to-parent transform. The setters above mark it dirty;
    // getNodeToParentTransform() recomputes only when something actually moved.
    mutable Mat4 _localMatrix = Mat4::identity();
    mutable bool _transformDirty = true;
};

} // namespace zocos

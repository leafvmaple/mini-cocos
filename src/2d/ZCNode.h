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

    bool isRunning() const { return _running; }
    bool isPaused() const { return _paused; }

    void pause() { _paused = true; }
    void resume() { _paused = false; }

    virtual void onEnter();
    virtual void onExit();

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
    void removeChild(Node* child);
    void removeAllChildren();
    const mstd::vector<Node*>& getChildren() const { return _children; }
    void sortAllChildren();

    Node* getParent() const { return _parent; }

    virtual void update(float /*dt*/) {}

    void updateTree(float dt);

    void visit(Renderer& renderer, const Mat4& parentWorld);

protected:
    Node() = default;

    virtual void draw(Renderer& /*renderer*/, const Mat4& /*world*/) {}

    Mat4 localMatrix() const {
        return zcNodeLocalMatrix(_position, _scale, _rotation, _anchorPoint, _contentSize);
    }

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
    mstd::size_t _orderOfArrival = 0;
    bool _reorderChildDirty = false;
};

} // namespace zocos

#pragma once

#include "base/ZCRef.h"
#include "math/ZCMath.h"
#include <functional>
#include <string>
#include <vector>

namespace zocos {

class Node : public Ref {
public:
    using ScheduleCallback = std::function<void(float)>;
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

    void setTag(int t) { _tag = t; }
    int getTag() const { return _tag; }

    bool isRunning() const { return _running; }
    bool isPaused() const { return _paused; }

    void pause() { _paused = true; }
    void resume() { _paused = false; }

    virtual void onEnter();
    virtual void onExit();

    void schedule(const std::string& key, ScheduleCallback callback, float interval = 0.f,
        int repeat = RepeatForever, float delay = 0.f, int priority = 0);
    void scheduleOnce(const std::string& key, ScheduleCallback callback, float delay = 0.f,
        int priority = 0);
    void unschedule(const std::string& key);
    void unscheduleAllCallbacks();

    void addChild(Node* child);
    void removeChild(Node* child);
    void removeAllChildren();
    const std::vector<Node*>& getChildren() const { return _children; }

    Node* getParent() const { return _parent; }

    virtual void update(float /*dt*/) {}

    void updateTree(float dt);

    void visit(const Mat4& parentWorld);

protected:
    Node() = default;

    virtual void draw(const Mat4& /*world*/) {}

    Mat4 localMatrix() const {
        return zcNodeLocalMatrix(_position, _scale, _rotation, _anchorPoint, _contentSize);
    }

    Node* _parent = nullptr;
    std::vector<Node*> _children;

    bool _running = false;
    bool _paused = false;

    Vec2 _position{};
    Vec2 _scale{1.f, 1.f};
    float _rotation = 0.f;
    Vec2 _anchorPoint{0.5f, 0.5f};
    Size _contentSize{};
    bool _visible = true;
    int _tag = -1;
};

} // namespace zocos

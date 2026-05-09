#pragma once

#include "base/ZCAnimation.h"
#include "base/ZCRef.h"
#include "math/ZCMath.h"

#include <functional>
#include <vector>

namespace zocos {

class Node;

class Action : public Ref {
public:
    ~Action() override = default;

    virtual void startWithTarget(Node* target);
    virtual void stop();

    virtual void step(float dt) = 0;
    virtual bool isDone() const = 0;

    Node* getTarget() const { return _target; }
    Node* getOriginalTarget() const { return _originalTarget; }

    void setTag(int tag) { _tag = tag; }
    int getTag() const { return _tag; }

protected:
    Action() = default;

    Node* _target = nullptr;
    Node* _originalTarget = nullptr;
    int _tag = -1;
};

class FiniteTimeAction : public Action {
public:
    float getDuration() const { return _duration; }

protected:
    explicit FiniteTimeAction(float duration);

    float _duration = 0.f;
};

class ActionInterval : public FiniteTimeAction {
public:
    void startWithTarget(Node* target) override;
    void step(float dt) override;
    bool isDone() const override;

protected:
    explicit ActionInterval(float duration);
    virtual void update(float t) = 0;

private:
    float _elapsed = 0.f;
    bool _firstTick = true;
};

class MoveTo : public ActionInterval {
public:
    static MoveTo* create(float duration, const Vec2& endPosition);

    void startWithTarget(Node* target) override;

protected:
    MoveTo(float duration, const Vec2& endPosition);
    void update(float t) override;

private:
    Vec2 _startPosition{};
    Vec2 _delta{};
    Vec2 _endPosition{};
};

class MoveBy : public ActionInterval {
public:
    static MoveBy* create(float duration, const Vec2& deltaPosition);

    void startWithTarget(Node* target) override;

protected:
    MoveBy(float duration, const Vec2& deltaPosition);
    void update(float t) override;

private:
    Vec2 _startPosition{};
    Vec2 _deltaPosition{};
};

class RotateTo : public ActionInterval {
public:
    static RotateTo* create(float duration, float endRotation);

    void startWithTarget(Node* target) override;

protected:
    RotateTo(float duration, float endRotation);
    void update(float t) override;

private:
    float _startRotation = 0.f;
    float _deltaRotation = 0.f;
    float _endRotation = 0.f;
};

class RotateBy : public ActionInterval {
public:
    static RotateBy* create(float duration, float deltaRotation);

    void startWithTarget(Node* target) override;

protected:
    RotateBy(float duration, float deltaRotation);
    void update(float t) override;

private:
    float _startRotation = 0.f;
    float _deltaRotation = 0.f;
};

class ScaleTo : public ActionInterval {
public:
    static ScaleTo* create(float duration, const Vec2& endScale);

    void startWithTarget(Node* target) override;

protected:
    ScaleTo(float duration, const Vec2& endScale);
    void update(float t) override;

private:
    Vec2 _startScale{};
    Vec2 _deltaScale{};
    Vec2 _endScale{};
};

class ScaleBy : public ActionInterval {
public:
    static ScaleBy* create(float duration, const Vec2& deltaScale);

    void startWithTarget(Node* target) override;

protected:
    ScaleBy(float duration, const Vec2& deltaScale);
    void update(float t) override;

private:
    Vec2 _startScale{};
    Vec2 _deltaScale{};
};

class FadeTo : public ActionInterval {
public:
    static FadeTo* create(float duration, float endOpacity);

    void startWithTarget(Node* target) override;

protected:
    FadeTo(float duration, float endOpacity);
    void update(float t) override;

private:
    float _startOpacity = 1.f;
    float _deltaOpacity = 0.f;
    float _endOpacity = 1.f;
};

class DelayTime : public ActionInterval {
public:
    static DelayTime* create(float duration);

protected:
    explicit DelayTime(float duration);
    void update(float t) override;
};

class Animate : public ActionInterval {
public:
    static Animate* create(Animation* animation);

    ~Animate() override;
    void startWithTarget(Node* target) override;

protected:
    explicit Animate(Animation* animation);
    void update(float t) override;

private:
    Animation* _animation = nullptr;
    std::size_t _lastFrameIndex = static_cast<std::size_t>(-1);
};

class CallFunc : public Action {
public:
    using Callback = std::function<void()>;

    static CallFunc* create(Callback callback);

    void startWithTarget(Node* target) override;
    void step(float dt) override;
    bool isDone() const override;

protected:
    explicit CallFunc(Callback callback);

private:
    Callback _callback;
    bool _done = false;
};

class Sequence : public Action {
public:
    static Sequence* create(const std::vector<Action*>& actions);

    ~Sequence() override;

    void startWithTarget(Node* target) override;
    void stop() override;
    void step(float dt) override;
    bool isDone() const override;

protected:
    explicit Sequence(const std::vector<Action*>& actions);

private:
    std::vector<Action*> _actions;
    std::size_t _currentIndex = 0;
};

class Spawn : public Action {
public:
    static Spawn* create(const std::vector<Action*>& actions);

    ~Spawn() override;

    void startWithTarget(Node* target) override;
    void stop() override;
    void step(float dt) override;
    bool isDone() const override;

protected:
    explicit Spawn(const std::vector<Action*>& actions);

private:
    std::vector<Action*> _actions;
};

class Repeat : public Action {
public:
    static Repeat* create(Action* action, int times);

    ~Repeat() override;

    void startWithTarget(Node* target) override;
    void stop() override;
    void step(float dt) override;
    bool isDone() const override;

protected:
    Repeat(Action* action, int times);

private:
    Action* _innerAction = nullptr;
    int _times = 0;
    int _total = 0;
};

class RepeatForever : public Action {
public:
    static RepeatForever* create(Action* action);

    ~RepeatForever() override;

    void startWithTarget(Node* target) override;
    void stop() override;
    void step(float dt) override;
    bool isDone() const override;

protected:
    explicit RepeatForever(Action* action);

private:
    Action* _innerAction = nullptr;
};

} // namespace zocos

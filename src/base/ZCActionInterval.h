#pragma once


#include "base/ZCStd.h"
#include "base/ZCAction.h"
#include "base/ZCAnimation.h"

#include <cstddef>

namespace zocos {

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
    mstd::size_t _lastFrameIndex = static_cast<mstd::size_t>(-1);
};

} // namespace zocos

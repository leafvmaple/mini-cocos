#pragma once

#include "base/ZCActionInterval.h"

#include "base/ZCStd.h"

namespace zocos {

// Wraps an inner ActionInterval and feeds it a time value remapped through an
// easing curve, so a linear MoveTo/ScaleTo/etc. accelerates or decelerates.
// Mirrors cocos2d-x ActionEase: the wrapper shares the inner action's duration
// and forwards lifecycle calls, overriding only update() to apply tween().
class ActionEase : public ActionInterval {
public:
    void startWithTarget(Node* target) override;
    void stop() override;
    void update(float t) override;

protected:
    explicit ActionEase(ActionInterval* inner);
    ~ActionEase() override;

    virtual float tween(float t) const = 0;

    ActionInterval* _inner = nullptr;
};

class EaseSineIn : public ActionEase {
public:
    static EaseSineIn* create(ActionInterval* inner);

protected:
    explicit EaseSineIn(ActionInterval* inner);
    float tween(float t) const override;
};

class EaseSineOut : public ActionEase {
public:
    static EaseSineOut* create(ActionInterval* inner);

protected:
    explicit EaseSineOut(ActionInterval* inner);
    float tween(float t) const override;
};

class EaseSineInOut : public ActionEase {
public:
    static EaseSineInOut* create(ActionInterval* inner);

protected:
    explicit EaseSineInOut(ActionInterval* inner);
    float tween(float t) const override;
};

class EaseCubicIn : public ActionEase {
public:
    static EaseCubicIn* create(ActionInterval* inner);

protected:
    explicit EaseCubicIn(ActionInterval* inner);
    float tween(float t) const override;
};

class EaseCubicOut : public ActionEase {
public:
    static EaseCubicOut* create(ActionInterval* inner);

protected:
    explicit EaseCubicOut(ActionInterval* inner);
    float tween(float t) const override;
};

class EaseCubicInOut : public ActionEase {
public:
    static EaseCubicInOut* create(ActionInterval* inner);

protected:
    explicit EaseCubicInOut(ActionInterval* inner);
    float tween(float t) const override;
};

} // namespace zocos

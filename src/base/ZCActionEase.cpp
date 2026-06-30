#include "base/ZCActionEase.h"

#include "base/ZCEasing.h"

#include "base/ZCStd.h"

namespace zocos {

ActionEase::ActionEase(ActionInterval* inner)
    : ActionInterval(inner ? inner->getDuration() : 0.f), _inner(inner) {
    if (_inner) {
        _inner->retain();
    }
}

ActionEase::~ActionEase() {
    if (_inner) {
        _inner->release();
        _inner = nullptr;
    }
}

void ActionEase::startWithTarget(Node* target) {
    ActionInterval::startWithTarget(target);
    if (_inner) {
        _inner->startWithTarget(target);
    }
}

void ActionEase::stop() {
    if (_inner) {
        _inner->stop();
    }
    ActionInterval::stop();
}

void ActionEase::update(float t) {
    if (_inner) {
        _inner->update(tween(t));
    }
}

// Boilerplate is near-identical per curve; a macro keeps the create/ctor/tween
// trio in sync. Each create() requires a non-null inner interval.
#define ZC_DEFINE_EASE(Name, Curve)                                                                \
    Name::Name(ActionInterval* inner) : ActionEase(inner) {}                                        \
    Name* Name::create(ActionInterval* inner) {                                                     \
        auto* action = new (mstd::nothrow) Name(inner);                                             \
        if (action && inner) {                                                                      \
            action->autorelease();                                                                  \
            return action;                                                                          \
        }                                                                                          \
        delete action;                                                                              \
        return nullptr;                                                                             \
    }                                                                                              \
    float Name::tween(float t) const { return Curve(t); }

ZC_DEFINE_EASE(EaseSineIn, easing::sineIn)
ZC_DEFINE_EASE(EaseSineOut, easing::sineOut)
ZC_DEFINE_EASE(EaseSineInOut, easing::sineInOut)
ZC_DEFINE_EASE(EaseCubicIn, easing::cubicIn)
ZC_DEFINE_EASE(EaseCubicOut, easing::cubicOut)
ZC_DEFINE_EASE(EaseCubicInOut, easing::cubicInOut)

#undef ZC_DEFINE_EASE

EaseRateAction::EaseRateAction(ActionInterval* inner, float rate)
    : ActionEase(inner), _rate(rate) {}

// As ZC_DEFINE_EASE, but threads a rate through create()/ctor and into the curve.
#define ZC_DEFINE_RATE_EASE(Name, Curve)                                                           \
    Name::Name(ActionInterval* inner, float rate) : EaseRateAction(inner, rate) {}                  \
    Name* Name::create(ActionInterval* inner, float rate) {                                         \
        auto* action = new (mstd::nothrow) Name(inner, rate);                                       \
        if (action && inner) {                                                                      \
            action->autorelease();                                                                  \
            return action;                                                                          \
        }                                                                                          \
        delete action;                                                                              \
        return nullptr;                                                                             \
    }                                                                                              \
    float Name::tween(float t) const { return Curve(t, _rate); }

ZC_DEFINE_RATE_EASE(EaseIn, easing::rateIn)
ZC_DEFINE_RATE_EASE(EaseOut, easing::rateOut)
ZC_DEFINE_RATE_EASE(EaseInOut, easing::rateInOut)

#undef ZC_DEFINE_RATE_EASE

} // namespace zocos

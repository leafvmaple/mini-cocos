#include "base/ZCActionInterval.h"

#include "2d/ZCNode.h"
#include "2d/ZCSprite.h"

#include "base/ZCStd.h"

namespace zocos {

namespace {

float clamp01(float value) { return mstd::clamp(value, 0.f, 1.f); }

} // namespace

ActionInterval::ActionInterval(float duration) : FiniteTimeAction(duration) {}

void ActionInterval::startWithTarget(Node* target) {
    Action::startWithTarget(target);
    _elapsed = 0.f;
    _firstTick = true;
}

void ActionInterval::step(float dt) {
    if (_firstTick) {
        _firstTick = false;
        _elapsed = 0.f;
    } else {
        _elapsed += dt;
    }

    if (_duration <= 0.f) {
        update(1.f);
        return;
    }

    update(clamp01(_elapsed / _duration));
}

bool ActionInterval::isDone() const { return _duration <= 0.f || _elapsed >= _duration; }

MoveTo::MoveTo(float duration, const Vec2& endPosition)
    : ActionInterval(duration), _endPosition(endPosition) {}

MoveTo* MoveTo::create(float duration, const Vec2& endPosition) {
    auto* action = new (mstd::nothrow) MoveTo(duration, endPosition);
    if (action) {
        action->autorelease();
        return action;
    }
    delete action;
    return nullptr;
}

void MoveTo::startWithTarget(Node* target) {
    ActionInterval::startWithTarget(target);
    _startPosition = _target->getPosition();
    _delta = {_endPosition.x - _startPosition.x, _endPosition.y - _startPosition.y};
}

void MoveTo::update(float t) {
    _target->setPosition(_startPosition.x + _delta.x * t, _startPosition.y + _delta.y * t);
}

MoveBy::MoveBy(float duration, const Vec2& deltaPosition)
    : ActionInterval(duration), _deltaPosition(deltaPosition) {}

MoveBy* MoveBy::create(float duration, const Vec2& deltaPosition) {
    auto* action = new (mstd::nothrow) MoveBy(duration, deltaPosition);
    if (action) {
        action->autorelease();
        return action;
    }
    delete action;
    return nullptr;
}

void MoveBy::startWithTarget(Node* target) {
    ActionInterval::startWithTarget(target);
    _startPosition = _target->getPosition();
}

void MoveBy::update(float t) {
    _target->setPosition(_startPosition.x + _deltaPosition.x * t,
                         _startPosition.y + _deltaPosition.y * t);
}

RotateTo::RotateTo(float duration, float endRotation)
    : ActionInterval(duration), _endRotation(endRotation) {}

RotateTo* RotateTo::create(float duration, float endRotation) {
    auto* action = new (mstd::nothrow) RotateTo(duration, endRotation);
    if (action) {
        action->autorelease();
        return action;
    }
    delete action;
    return nullptr;
}

void RotateTo::startWithTarget(Node* target) {
    ActionInterval::startWithTarget(target);
    _startRotation = _target->getRotation();
    _deltaRotation = _endRotation - _startRotation;
}

void RotateTo::update(float t) { _target->setRotation(_startRotation + _deltaRotation * t); }

RotateBy::RotateBy(float duration, float deltaRotation)
    : ActionInterval(duration), _deltaRotation(deltaRotation) {}

RotateBy* RotateBy::create(float duration, float deltaRotation) {
    auto* action = new (mstd::nothrow) RotateBy(duration, deltaRotation);
    if (action) {
        action->autorelease();
        return action;
    }
    delete action;
    return nullptr;
}

void RotateBy::startWithTarget(Node* target) {
    ActionInterval::startWithTarget(target);
    _startRotation = _target->getRotation();
}

void RotateBy::update(float t) { _target->setRotation(_startRotation + _deltaRotation * t); }

ScaleTo::ScaleTo(float duration, const Vec2& endScale)
    : ActionInterval(duration), _endScale(endScale) {}

ScaleTo* ScaleTo::create(float duration, const Vec2& endScale) {
    auto* action = new (mstd::nothrow) ScaleTo(duration, endScale);
    if (action) {
        action->autorelease();
        return action;
    }
    delete action;
    return nullptr;
}

void ScaleTo::startWithTarget(Node* target) {
    ActionInterval::startWithTarget(target);
    _startScale = _target->getScale();
    _deltaScale = {_endScale.x - _startScale.x, _endScale.y - _startScale.y};
}

void ScaleTo::update(float t) {
    _target->setScale({_startScale.x + _deltaScale.x * t, _startScale.y + _deltaScale.y * t});
}

ScaleBy::ScaleBy(float duration, const Vec2& deltaScale)
    : ActionInterval(duration), _deltaScale(deltaScale) {}

ScaleBy* ScaleBy::create(float duration, const Vec2& deltaScale) {
    auto* action = new (mstd::nothrow) ScaleBy(duration, deltaScale);
    if (action) {
        action->autorelease();
        return action;
    }
    delete action;
    return nullptr;
}

void ScaleBy::startWithTarget(Node* target) {
    ActionInterval::startWithTarget(target);
    _startScale = _target->getScale();
}

void ScaleBy::update(float t) {
    _target->setScale({_startScale.x + _deltaScale.x * t, _startScale.y + _deltaScale.y * t});
}

FadeTo::FadeTo(float duration, float endOpacity)
    : ActionInterval(duration), _endOpacity(clamp01(endOpacity)) {}

FadeTo* FadeTo::create(float duration, float endOpacity) {
    auto* action = new (mstd::nothrow) FadeTo(duration, endOpacity);
    if (action) {
        action->autorelease();
        return action;
    }
    delete action;
    return nullptr;
}

void FadeTo::startWithTarget(Node* target) {
    ActionInterval::startWithTarget(target);
    _startOpacity = clamp01(_target->getOpacity());
    _deltaOpacity = _endOpacity - _startOpacity;
}

void FadeTo::update(float t) { _target->setOpacity(_startOpacity + _deltaOpacity * t); }

DelayTime::DelayTime(float duration) : ActionInterval(duration) {}

DelayTime* DelayTime::create(float duration) {
    auto* action = new (mstd::nothrow) DelayTime(duration);
    if (action) {
        action->autorelease();
        return action;
    }
    delete action;
    return nullptr;
}

void DelayTime::update(float t) { (void)t; }

Animate::Animate(Animation* animation)
    : ActionInterval(animation ? animation->getDuration() : 0.f), _animation(animation) {
    if (_animation) {
        _animation->retain();
    }
}

Animate* Animate::create(Animation* animation) {
    if (!animation) {
        return nullptr;
    }

    auto* animate = new (mstd::nothrow) Animate(animation);
    if (animate) {
        animate->autorelease();
        return animate;
    }
    delete animate;
    return nullptr;
}

Animate::~Animate() {
    if (_animation) {
        _animation->release();
        _animation = nullptr;
    }
}

void Animate::startWithTarget(Node* target) {
    ActionInterval::startWithTarget(target);
    _lastFrameIndex = static_cast<mstd::size_t>(-1);
    update(0.f);
}

void Animate::update(float t) {
    auto* sprite = dynamic_cast<Sprite*>(_target);
    const auto& frames = _animation->getFrames();
    if (frames.empty()) {
        return;
    }

    const float clamped = clamp01(t);
    mstd::size_t index = static_cast<mstd::size_t>(clamped * static_cast<float>(frames.size()));
    if (index >= frames.size()) {
        index = frames.size() - 1;
    }

    if (index == _lastFrameIndex) {
        return;
    }

    sprite->setTextureRect(frames[index], true);
    _lastFrameIndex = index;
}

} // namespace zocos

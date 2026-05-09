#include "base/ZCAction.h"

#include "base/ZCAnimation.h"
#include "2d/ZCNode.h"
#include "2d/ZCSprite.h"

#include <algorithm>
#include <new>
#include <utility>

namespace zocos {

namespace {

float clamp01(float value) {
    return std::clamp(value, 0.f, 1.f);
}

std::vector<Action*> retainActions(const std::vector<Action*>& actions) {
    std::vector<Action*> retained;
    retained.reserve(actions.size());
    for (auto* action : actions) {
        if (!action) {
            continue;
        }
        action->retain();
        retained.push_back(action);
    }
    return retained;
}

void releaseActions(std::vector<Action*>& actions) {
    for (auto* action : actions) {
        if (action) {
            action->release();
        }
    }
    actions.clear();
}

} // namespace

void Action::startWithTarget(Node* target) {
    _originalTarget = target;
    _target = target;
}

void Action::stop() {
    _target = nullptr;
}

FiniteTimeAction::FiniteTimeAction(float duration) {
    _duration = std::max(0.f, duration);
}

ActionInterval::ActionInterval(float duration) : FiniteTimeAction(duration) {
}

void ActionInterval::startWithTarget(Node* target) {
    Action::startWithTarget(target);
    _elapsed = 0.f;
    _firstTick = true;
}

void ActionInterval::step(float dt) {
    if (!_target) {
        return;
    }

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

bool ActionInterval::isDone() const {
    return _duration <= 0.f || _elapsed >= _duration;
}

MoveTo::MoveTo(float duration, const Vec2& endPosition)
    : ActionInterval(duration), _endPosition(endPosition) {
}

MoveTo* MoveTo::create(float duration, const Vec2& endPosition) {
    auto* action = new (std::nothrow) MoveTo(duration, endPosition);
    if (action) {
        action->autorelease();
        return action;
    }
    delete action;
    return nullptr;
}

void MoveTo::startWithTarget(Node* target) {
    ActionInterval::startWithTarget(target);
    if (!_target) {
        return;
    }
    _startPosition = _target->getPosition();
    _delta = {_endPosition.x - _startPosition.x, _endPosition.y - _startPosition.y};
}

void MoveTo::update(float t) {
    if (!_target) {
        return;
    }
    _target->setPosition(_startPosition.x + _delta.x * t, _startPosition.y + _delta.y * t);
}

MoveBy::MoveBy(float duration, const Vec2& deltaPosition)
    : ActionInterval(duration), _deltaPosition(deltaPosition) {
}

MoveBy* MoveBy::create(float duration, const Vec2& deltaPosition) {
    auto* action = new (std::nothrow) MoveBy(duration, deltaPosition);
    if (action) {
        action->autorelease();
        return action;
    }
    delete action;
    return nullptr;
}

void MoveBy::startWithTarget(Node* target) {
    ActionInterval::startWithTarget(target);
    if (!_target) {
        return;
    }
    _startPosition = _target->getPosition();
}

void MoveBy::update(float t) {
    if (!_target) {
        return;
    }
    _target->setPosition(_startPosition.x + _deltaPosition.x * t, _startPosition.y + _deltaPosition.y * t);
}

RotateTo::RotateTo(float duration, float endRotation)
    : ActionInterval(duration), _endRotation(endRotation) {
}

RotateTo* RotateTo::create(float duration, float endRotation) {
    auto* action = new (std::nothrow) RotateTo(duration, endRotation);
    if (action) {
        action->autorelease();
        return action;
    }
    delete action;
    return nullptr;
}

void RotateTo::startWithTarget(Node* target) {
    ActionInterval::startWithTarget(target);
    if (!_target) {
        return;
    }
    _startRotation = _target->getRotation();
    _deltaRotation = _endRotation - _startRotation;
}

void RotateTo::update(float t) {
    if (!_target) {
        return;
    }
    _target->setRotation(_startRotation + _deltaRotation * t);
}

RotateBy::RotateBy(float duration, float deltaRotation)
    : ActionInterval(duration), _deltaRotation(deltaRotation) {
}

RotateBy* RotateBy::create(float duration, float deltaRotation) {
    auto* action = new (std::nothrow) RotateBy(duration, deltaRotation);
    if (action) {
        action->autorelease();
        return action;
    }
    delete action;
    return nullptr;
}

void RotateBy::startWithTarget(Node* target) {
    ActionInterval::startWithTarget(target);
    if (!_target) {
        return;
    }
    _startRotation = _target->getRotation();
}

void RotateBy::update(float t) {
    if (!_target) {
        return;
    }
    _target->setRotation(_startRotation + _deltaRotation * t);
}

ScaleTo::ScaleTo(float duration, const Vec2& endScale)
    : ActionInterval(duration), _endScale(endScale) {
}

ScaleTo* ScaleTo::create(float duration, const Vec2& endScale) {
    auto* action = new (std::nothrow) ScaleTo(duration, endScale);
    if (action) {
        action->autorelease();
        return action;
    }
    delete action;
    return nullptr;
}

void ScaleTo::startWithTarget(Node* target) {
    ActionInterval::startWithTarget(target);
    if (!_target) {
        return;
    }
    _startScale = _target->getScale();
    _deltaScale = {_endScale.x - _startScale.x, _endScale.y - _startScale.y};
}

void ScaleTo::update(float t) {
    if (!_target) {
        return;
    }
    _target->setScale({_startScale.x + _deltaScale.x * t, _startScale.y + _deltaScale.y * t});
}

ScaleBy::ScaleBy(float duration, const Vec2& deltaScale)
    : ActionInterval(duration), _deltaScale(deltaScale) {
}

ScaleBy* ScaleBy::create(float duration, const Vec2& deltaScale) {
    auto* action = new (std::nothrow) ScaleBy(duration, deltaScale);
    if (action) {
        action->autorelease();
        return action;
    }
    delete action;
    return nullptr;
}

void ScaleBy::startWithTarget(Node* target) {
    ActionInterval::startWithTarget(target);
    if (!_target) {
        return;
    }
    _startScale = _target->getScale();
}

void ScaleBy::update(float t) {
    if (!_target) {
        return;
    }
    _target->setScale({_startScale.x + _deltaScale.x * t, _startScale.y + _deltaScale.y * t});
}

FadeTo::FadeTo(float duration, float endOpacity)
    : ActionInterval(duration), _endOpacity(clamp01(endOpacity)) {
}

FadeTo* FadeTo::create(float duration, float endOpacity) {
    auto* action = new (std::nothrow) FadeTo(duration, endOpacity);
    if (action) {
        action->autorelease();
        return action;
    }
    delete action;
    return nullptr;
}

void FadeTo::startWithTarget(Node* target) {
    ActionInterval::startWithTarget(target);
    if (!_target) {
        return;
    }
    _startOpacity = clamp01(_target->getOpacity());
    _deltaOpacity = _endOpacity - _startOpacity;
}

void FadeTo::update(float t) {
    if (!_target) {
        return;
    }
    _target->setOpacity(_startOpacity + _deltaOpacity * t);
}

DelayTime::DelayTime(float duration) : ActionInterval(duration) {
}

DelayTime* DelayTime::create(float duration) {
    auto* action = new (std::nothrow) DelayTime(duration);
    if (action) {
        action->autorelease();
        return action;
    }
    delete action;
    return nullptr;
}

void DelayTime::update(float t) {
    (void)t;
}

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

    auto* animate = new (std::nothrow) Animate(animation);
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
    _lastFrameIndex = static_cast<std::size_t>(-1);
    update(0.f);
}

void Animate::update(float t) {
    if (!_target || !_animation) {
        return;
    }

    auto* sprite = dynamic_cast<Sprite*>(_target);
    if (!sprite) {
        return;
    }

    const auto& frames = _animation->getFrames();
    if (frames.empty()) {
        return;
    }

    const float clamped = clamp01(t);
    std::size_t index = static_cast<std::size_t>(clamped * static_cast<float>(frames.size()));
    if (index >= frames.size()) {
        index = frames.size() - 1;
    }

    if (index == _lastFrameIndex) {
        return;
    }

    sprite->setTextureRect(frames[index], true);
    _lastFrameIndex = index;
}

CallFunc::CallFunc(Callback callback) : _callback(std::move(callback)) {
}

CallFunc* CallFunc::create(Callback callback) {
    auto* action = new (std::nothrow) CallFunc(std::move(callback));
    if (action) {
        action->autorelease();
        return action;
    }
    delete action;
    return nullptr;
}

void CallFunc::startWithTarget(Node* target) {
    Action::startWithTarget(target);
    _done = false;
}

void CallFunc::step(float dt) {
    (void)dt;
    if (_done) {
        return;
    }
    if (_callback) {
        _callback();
    }
    _done = true;
}

bool CallFunc::isDone() const {
    return _done;
}

Sequence::Sequence(const std::vector<Action*>& actions)
    : _actions(retainActions(actions)) {
}

Sequence* Sequence::create(const std::vector<Action*>& actions) {
    auto* action = new (std::nothrow) Sequence(actions);
    if (action) {
        action->autorelease();
        return action;
    }
    delete action;
    return nullptr;
}

Sequence::~Sequence() {
    releaseActions(_actions);
}

void Sequence::startWithTarget(Node* target) {
    Action::startWithTarget(target);
    _currentIndex = 0;
    if (_actions.empty()) {
        return;
    }
    _actions[_currentIndex]->startWithTarget(target);
}

void Sequence::stop() {
    if (_currentIndex < _actions.size() && _actions[_currentIndex]) {
        _actions[_currentIndex]->stop();
    }
    Action::stop();
}

void Sequence::step(float dt) {
    if (_currentIndex >= _actions.size()) {
        return;
    }

    auto* current = _actions[_currentIndex];
    if (current) {
        current->step(dt);
    }

    std::size_t guard = _actions.size() + 1;
    while (_currentIndex < _actions.size()) {
        auto* active = _actions[_currentIndex];
        if (active && !active->isDone()) {
            break;
        }

        if (active) {
            active->stop();
        }

        ++_currentIndex;
        if (_currentIndex >= _actions.size()) {
            break;
        }

        auto* next = _actions[_currentIndex];
        if (next) {
            next->startWithTarget(_target);
            next->step(0.f);
        }

        if (guard-- == 0) {
            break;
        }
    }
}

bool Sequence::isDone() const {
    return _actions.empty() || _currentIndex >= _actions.size();
}

Spawn::Spawn(const std::vector<Action*>& actions)
    : _actions(retainActions(actions)) {
}

Spawn* Spawn::create(const std::vector<Action*>& actions) {
    auto* action = new (std::nothrow) Spawn(actions);
    if (action) {
        action->autorelease();
        return action;
    }
    delete action;
    return nullptr;
}

Spawn::~Spawn() {
    releaseActions(_actions);
}

void Spawn::startWithTarget(Node* target) {
    Action::startWithTarget(target);
    for (auto* action : _actions) {
        if (action) {
            action->startWithTarget(target);
        }
    }
}

void Spawn::stop() {
    for (auto* action : _actions) {
        if (action) {
            action->stop();
        }
    }
    Action::stop();
}

void Spawn::step(float dt) {
    for (auto* action : _actions) {
        if (action && !action->isDone()) {
            action->step(dt);
        }
    }
}

bool Spawn::isDone() const {
    if (_actions.empty()) {
        return true;
    }
    for (auto* action : _actions) {
        if (action && !action->isDone()) {
            return false;
        }
    }
    return true;
}

Repeat::Repeat(Action* action, int times) : _innerAction(action), _times(std::max(0, times)) {
    if (_innerAction) {
        _innerAction->retain();
    }
}

Repeat* Repeat::create(Action* action, int times) {
    auto* repeat = new (std::nothrow) Repeat(action, times);
    if (repeat) {
        repeat->autorelease();
        return repeat;
    }
    delete repeat;
    return nullptr;
}

Repeat::~Repeat() {
    if (_innerAction) {
        _innerAction->release();
        _innerAction = nullptr;
    }
}

void Repeat::startWithTarget(Node* target) {
    Action::startWithTarget(target);
    _total = 0;
    if (_innerAction && _times > 0) {
        _innerAction->startWithTarget(target);
    }
}

void Repeat::stop() {
    if (_innerAction) {
        _innerAction->stop();
    }
    Action::stop();
}

void Repeat::step(float dt) {
    if (!_innerAction || _times <= 0 || _total >= _times) {
        return;
    }

    _innerAction->step(dt);
    std::size_t guard = static_cast<std::size_t>(_times - _total) + 1;
    while (_innerAction->isDone() && _total < _times) {
        _innerAction->stop();
        ++_total;
        if (_total >= _times) {
            break;
        }
        _innerAction->startWithTarget(_target);
        _innerAction->step(0.f);
        if (guard-- == 0) {
            break;
        }
    }
}

bool Repeat::isDone() const {
    return !_innerAction || _times <= 0 || _total >= _times;
}

RepeatForever::RepeatForever(Action* action) : _innerAction(action) {
    if (_innerAction) {
        _innerAction->retain();
    }
}

RepeatForever* RepeatForever::create(Action* action) {
    auto* repeat = new (std::nothrow) RepeatForever(action);
    if (repeat) {
        repeat->autorelease();
        return repeat;
    }
    delete repeat;
    return nullptr;
}

RepeatForever::~RepeatForever() {
    if (_innerAction) {
        _innerAction->release();
        _innerAction = nullptr;
    }
}

void RepeatForever::startWithTarget(Node* target) {
    Action::startWithTarget(target);
    if (_innerAction) {
        _innerAction->startWithTarget(target);
    }
}

void RepeatForever::stop() {
    if (_innerAction) {
        _innerAction->stop();
    }
    Action::stop();
}

void RepeatForever::step(float dt) {
    if (!_innerAction) {
        return;
    }

    _innerAction->step(dt);
    std::size_t guard = 8;
    while (_innerAction->isDone() && guard-- > 0) {
        _innerAction->stop();
        _innerAction->startWithTarget(_target);
        _innerAction->step(0.f);
        if (!_innerAction->isDone()) {
            break;
        }
    }
}

bool RepeatForever::isDone() const {
    return _innerAction == nullptr;
}

} // namespace zocos

#include "base/ZCAction.h"
#include "2d/ZCNode.h"

#include <algorithm>
#include <new>
#include <utility>

namespace zocos {

namespace {

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

void Action::stop() { _target = nullptr; }

FiniteTimeAction::FiniteTimeAction(float duration) { _duration = std::max(0.f, duration); }

CallFunc::CallFunc(Callback callback) : _callback(std::move(callback)) {}

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

bool CallFunc::isDone() const { return _done; }

Sequence::Sequence(const std::vector<Action*>& actions) : _actions(retainActions(actions)) {}

Sequence* Sequence::create(const std::vector<Action*>& actions) {
    auto* action = new (std::nothrow) Sequence(actions);
    if (action) {
        action->autorelease();
        return action;
    }
    delete action;
    return nullptr;
}

Sequence::~Sequence() { releaseActions(_actions); }

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

bool Sequence::isDone() const { return _actions.empty() || _currentIndex >= _actions.size(); }

Spawn::Spawn(const std::vector<Action*>& actions) : _actions(retainActions(actions)) {}

Spawn* Spawn::create(const std::vector<Action*>& actions) {
    auto* action = new (std::nothrow) Spawn(actions);
    if (action) {
        action->autorelease();
        return action;
    }
    delete action;
    return nullptr;
}

Spawn::~Spawn() { releaseActions(_actions); }

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

bool Repeat::isDone() const { return !_innerAction || _times <= 0 || _total >= _times; }

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

bool RepeatForever::isDone() const { return _innerAction == nullptr; }

} // namespace zocos

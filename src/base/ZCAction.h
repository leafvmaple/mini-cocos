#pragma once

#include "base/ZCRef.h"

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

#include "ZCTestFramework.h"

#include "2d/ZCNode.h"
#include "base/ZCAction.h"
#include "base/ZCActionManager.h"
#include "base/ZCAutoreleasePool.h"
#include "base/ZCDirector.h"
#include "base/ZCEventDispatcher.h"
#include "base/ZCEventListener.h"
#include "base/ZCScheduler.h"

using namespace zocos;

namespace {
class TestNode : public Node {
public:
    void enter() { onEnter(); }
    void leave() { onExit(); }

    void cleanup() override {
        ++cleanupCount;
        Node::cleanup();
    }

    int cleanupCount = 0;
};

class CountingAction final : public Action {
public:
    explicit CountingAction(int stepsToFinish) : _stepsToFinish(stepsToFinish) {}

    void step(float) override { ++steps; }
    bool isDone() const override { return steps >= _stepsToFinish; }

    int steps = 0;

private:
    int _stepsToFinish = 1;
};

class LifetimeNode final : public TestNode {
public:
    explicit LifetimeNode(int& destructionCount) : _destructionCount(destructionCount) {}
    ~LifetimeNode() override { ++_destructionCount; }

private:
    int& _destructionCount;
};

class KeyboardListener final : public EventListener {
public:
    explicit KeyboardListener(mstd::function<void(EventKeyboard&)> callback)
        : EventListener(Type::Keyboard), _callback(mstd::move(callback)) {}

    bool hasCallbacks() const override { return static_cast<bool>(_callback); }
    bool dispatchEvent(Event& event) override {
        if (event.getType() != Event::Type::Keyboard || !_callback) {
            return false;
        }
        _callback(static_cast<EventKeyboard&>(event));
        return true;
    }

private:
    mstd::function<void(EventKeyboard&)> _callback;
};
} // namespace

ZC_TEST(scheduler_honors_lifecycle_pause_and_repeat) {
    Scheduler scheduler;
    TestNode node;
    int calls = 0;

    scheduler.schedule(&node, "tick", [&](float) { ++calls; }, 0.f, 1);
    node.enter();
    scheduler.update(0.1f);
    node.pause();
    scheduler.update(0.1f);
    ZC_CHECK_EQ(calls, 1);

    node.resume();
    scheduler.update(0.1f);
    ZC_CHECK_EQ(calls, 2);
    ZC_CHECK_EQ(scheduler.getScheduledCount(), static_cast<mstd::size_t>(0));
    node.leave();
}

ZC_TEST(action_manager_finishes_and_releases_actions) {
    ActionManager manager;
    TestNode node;
    node.enter();
    auto* action = new CountingAction(2);

    manager.addAction(action, &node);
    manager.update(0.1f);
    ZC_CHECK_EQ(action->steps, 1);
    ZC_CHECK_EQ(manager.getRunningActionCount(), static_cast<mstd::size_t>(1));
    manager.update(0.1f);
    ZC_CHECK_EQ(action->steps, 2);
    ZC_CHECK_EQ(manager.getRunningActionCount(), static_cast<mstd::size_t>(0));

    action->release();
    node.leave();
}

ZC_TEST(action_instant_sequence_runs_callbacks_in_order) {
    AutoreleasePool pool("action instant test");
    ActionManager manager;
    TestNode node;
    mstd::vector<int> order;

    node.enter();
    auto* sequence = Sequence::create({
        CallFunc::create([&]() { order.push_back(1); }),
        CallFunc::create([&]() { order.push_back(2); }),
    });
    manager.addAction(sequence, &node);
    manager.update(0.1f);

    ZC_CHECK_EQ(order.size(), static_cast<mstd::size_t>(2));
    ZC_CHECK_EQ(order[0], 1);
    ZC_CHECK_EQ(order[1], 2);
    ZC_CHECK_EQ(manager.getRunningActionCount(), static_cast<mstd::size_t>(0));
    node.leave();
}

ZC_TEST(remove_self_honors_cleanup_flag) {
    AutoreleasePool pool("remove self test");
    ActionManager manager;
    TestNode parent;
    TestNode withoutCleanup;
    TestNode withCleanup;

    parent.enter();

    parent.addChild(&withoutCleanup);
    manager.addAction(RemoveSelf::create(false), &withoutCleanup);
    manager.update(0.f);
    ZC_CHECK_EQ(withoutCleanup.getParent(), nullptr);
    ZC_CHECK_EQ(withoutCleanup.cleanupCount, 0);

    parent.addChild(&withCleanup);
    manager.addAction(Sequence::create({CallFunc::create([]() {}), RemoveSelf::create()}),
                      &withCleanup);
    manager.update(0.f);
    ZC_CHECK_EQ(withCleanup.getParent(), nullptr);
    ZC_CHECK_EQ(withCleanup.cleanupCount, 1);
    ZC_CHECK(parent.getChildren().empty());
    ZC_CHECK_EQ(manager.getRunningActionCount(), static_cast<mstd::size_t>(0));

    parent.leave();
}

ZC_TEST(remove_self_can_destroy_target_during_action_manager_update) {
    AutoreleasePool pool("remove self lifetime test");
    auto& manager = Director::getInstance().getActionManager();
    TestNode parent;
    int destructionCount = 0;

    ZC_CHECK_EQ(manager.getRunningActionCount(), static_cast<mstd::size_t>(0));
    parent.enter();

    auto* child = new LifetimeNode(destructionCount);
    child->autorelease();
    parent.addChild(child);
    child->runAction(Sequence::create({CallFunc::create([]() {}), RemoveSelf::create()}));

    pool.clear();
    manager.update(0.f);

    ZC_CHECK_EQ(destructionCount, 1);
    ZC_CHECK(parent.getChildren().empty());
    ZC_CHECK_EQ(manager.getRunningActionCount(), static_cast<mstd::size_t>(0));
    parent.leave();
}

ZC_TEST(event_dispatcher_honors_priority_and_mutation_during_dispatch) {
    EventDispatcher dispatcher;
    mstd::vector<int> order;
    EventDispatcher::ListenerHandle secondHandle = 0;

    auto* first = new KeyboardListener([&](EventKeyboard&) {
        order.push_back(1);
        dispatcher.removeEventListener(secondHandle);
    });
    auto* second = new KeyboardListener([&](EventKeyboard&) { order.push_back(2); });
    dispatcher.addEventListenerWithFixedPriority(first, -10);
    secondHandle = dispatcher.addEventListenerWithFixedPriority(second, 10);

    EventKeyboard event(65, 0, 0, true, false);
    dispatcher.dispatchEvent(event);
    ZC_CHECK_EQ(order.size(), static_cast<mstd::size_t>(1));
    ZC_CHECK_EQ(order[0], 1);
    ZC_CHECK_EQ(dispatcher.getListenerCount(), static_cast<mstd::size_t>(1));

    dispatcher.removeAllEventListeners();
    first->release();
    second->release();
}

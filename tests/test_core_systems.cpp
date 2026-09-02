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

ZC_TEST(scheduler_target_pause_stops_later_callbacks) {
    Scheduler scheduler;
    TestNode node;
    mstd::vector<int> order;

    scheduler.schedule(
        &node, "pause",
        [&](float) {
            order.push_back(1);
            scheduler.pauseTarget(&node);
        },
        0.f, 0, 0.f, -1);
    scheduler.schedule(&node, "second", [&](float) { order.push_back(2); }, 0.f, 0);
    node.enter();
    scheduler.update(0.1f);

    ZC_CHECK_EQ(order.size(), static_cast<mstd::size_t>(1));
    ZC_CHECK_EQ(order[0], 1);
    ZC_CHECK(scheduler.isTargetPaused(&node));

    scheduler.schedule(&node, "third", [&](float) { order.push_back(3); }, 0.f, 0);
    scheduler.update(0.1f);
    ZC_CHECK_EQ(order.size(), static_cast<mstd::size_t>(1));

    scheduler.resumeTarget(&node);
    scheduler.update(0.1f);
    ZC_CHECK_EQ(order.size(), static_cast<mstd::size_t>(3));
    ZC_CHECK_EQ(order[1], 2);
    ZC_CHECK_EQ(order[2], 3);
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

ZC_TEST(action_manager_target_pause_stops_later_and_new_actions) {
    AutoreleasePool pool("action target pause test");
    ActionManager manager;
    TestNode node;
    auto* later = new CountingAction(10);
    auto* addedWhilePaused = new CountingAction(10);

    node.enter();
    manager.addAction(CallFunc::create([&]() { manager.pauseTarget(&node); }), &node);
    manager.addAction(later, &node);
    manager.update(0.1f);

    ZC_CHECK_EQ(later->steps, 0);
    ZC_CHECK(manager.isTargetPaused(&node));

    manager.addAction(addedWhilePaused, &node);
    manager.update(0.1f);
    ZC_CHECK_EQ(later->steps, 0);
    ZC_CHECK_EQ(addedWhilePaused->steps, 0);

    manager.resumeTarget(&node);
    manager.update(0.1f);
    ZC_CHECK_EQ(later->steps, 1);
    ZC_CHECK_EQ(addedWhilePaused->steps, 1);

    manager.removeAllActions();
    later->release();
    addedWhilePaused->release();
    node.leave();
}

ZC_TEST(node_pause_forwards_to_actions_and_scheduler) {
    auto& director = Director::getInstance();
    auto& actionManager = director.getActionManager();
    auto& scheduler = director.getScheduler();
    TestNode node;
    auto* action = new CountingAction(10);
    int scheduleCalls = 0;

    ZC_CHECK_EQ(actionManager.getRunningActionCount(), static_cast<mstd::size_t>(0));
    ZC_CHECK_EQ(scheduler.getScheduledCount(), static_cast<mstd::size_t>(0));
    node.enter();
    node.runAction(action);
    node.schedule("tick", [&](float) { ++scheduleCalls; });

    node.pause();
    ZC_CHECK(node.isPaused());
    ZC_CHECK(actionManager.isTargetPaused(&node));
    ZC_CHECK(scheduler.isTargetPaused(&node));
    actionManager.update(0.1f);
    scheduler.update(0.1f);
    ZC_CHECK_EQ(action->steps, 0);
    ZC_CHECK_EQ(scheduleCalls, 0);

    node.resume();
    ZC_CHECK(!node.isPaused());
    ZC_CHECK(!actionManager.isTargetPaused(&node));
    ZC_CHECK(!scheduler.isTargetPaused(&node));
    actionManager.update(0.1f);
    scheduler.update(0.1f);
    ZC_CHECK_EQ(action->steps, 1);
    ZC_CHECK_EQ(scheduleCalls, 1);

    node.stopAllActions();
    node.unscheduleAllCallbacks();
    action->release();
    node.leave();
}

ZC_TEST(node_action_tags_select_count_and_remove_actions) {
    auto& manager = Director::getInstance().getActionManager();
    TestNode node;
    auto* first = new CountingAction(10);
    auto* second = new CountingAction(10);
    auto* third = new CountingAction(10);

    ZC_CHECK_EQ(manager.getRunningActionCount(), static_cast<mstd::size_t>(0));
    node.enter();
    first->setTag(7);
    second->setTag(7);
    third->setTag(8);
    node.runAction(first);
    node.runAction(second);
    node.runAction(third);

    ZC_CHECK_EQ(node.getNumberOfRunningActions(), static_cast<mstd::size_t>(3));
    ZC_CHECK_EQ(node.getNumberOfRunningActionsByTag(7), static_cast<mstd::size_t>(2));
    ZC_CHECK_EQ(node.getActionByTag(7), first);

    node.stopActionByTag(7);
    ZC_CHECK_EQ(first->getTarget(), nullptr);
    ZC_CHECK_EQ(node.getActionByTag(7), second);
    ZC_CHECK_EQ(node.getNumberOfRunningActions(), static_cast<mstd::size_t>(2));

    node.stopAllActionsByTag(7);
    ZC_CHECK_EQ(second->getTarget(), nullptr);
    ZC_CHECK_EQ(node.getActionByTag(7), nullptr);
    ZC_CHECK_EQ(node.getNumberOfRunningActionsByTag(7), static_cast<mstd::size_t>(0));
    ZC_CHECK_EQ(node.getNumberOfRunningActions(), static_cast<mstd::size_t>(1));

    node.stopAllActions();
    ZC_CHECK_EQ(third->getTarget(), nullptr);
    ZC_CHECK_EQ(node.getNumberOfRunningActions(), static_cast<mstd::size_t>(0));

    first->release();
    second->release();
    third->release();
    node.leave();
}

ZC_TEST(action_tag_queries_include_pending_actions) {
    AutoreleasePool pool("pending action tag test");
    ActionManager manager;
    TestNode node;
    auto* pending = new CountingAction(10);

    node.enter();
    pending->setTag(9);
    manager.addAction(
        CallFunc::create([&]() {
            manager.addAction(pending, &node);
            ZC_CHECK_EQ(manager.getActionByTag(9, &node), pending);
            ZC_CHECK_EQ(manager.getNumberOfRunningActionsInTarget(&node),
                        static_cast<mstd::size_t>(2));
            ZC_CHECK_EQ(manager.getNumberOfRunningActionsInTargetByTag(&node, 9),
                        static_cast<mstd::size_t>(1));
            manager.removeActionByTag(9, &node);
            ZC_CHECK_EQ(manager.getActionByTag(9, &node), nullptr);
        }),
        &node);

    manager.update(0.f);
    ZC_CHECK_EQ(manager.getRunningActionCount(), static_cast<mstd::size_t>(0));

    pending->release();
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

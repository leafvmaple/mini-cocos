#include "ZCTestFramework.h"

#include "2d/ZCScene.h"
#include "base/ZCAction.h"
#include "base/ZCAutoreleasePool.h"
#include "base/ZCDirector.h"
#include "base/ZCEvent.h"
#include "base/ZCEventListener.h"
#include "ui/UIWidget.h"

using namespace zocos;

namespace {
class RecordingScene final : public Scene {
public:
    void onEnter() override {
        ++enterCount;
        Scene::onEnter();
    }

    void onExit() override {
        ++exitCount;
        Scene::onExit();
    }

    void onEnterTransitionDidFinish() override {
        ++enterTransitionFinishedCount;
        Scene::onEnterTransitionDidFinish();
    }

    void onExitTransitionDidStart() override {
        ++exitTransitionStartedCount;
        Scene::onExitTransitionDidStart();
    }

    void cleanup() override {
        ++cleanupCount;
        Scene::cleanup();
    }

    int enterCount = 0;
    int exitCount = 0;
    int enterTransitionFinishedCount = 0;
    int exitTransitionStartedCount = 0;
    int cleanupCount = 0;
};

class CountingAction final : public Action {
public:
    explicit CountingAction(int& steps) : _steps(steps) {}

    void step(float) override { ++_steps; }
    bool isDone() const override { return false; }

private:
    int& _steps;
};

class RemovingWidget final : public ui::Widget {
public:
    explicit RemovingWidget(bool& destroyed) : _destroyed(destroyed) {}
    ~RemovingWidget() override { _destroyed = true; }

private:
    bool& _destroyed;
};

class TrackingWidget final : public ui::Widget {
public:
    bool pressed() const { return isPressed(); }
    const mstd::vector<bool>& pressStates() const { return _pressStates; }

protected:
    void onPressStateChanged(bool pressed) override { _pressStates.push_back(pressed); }

private:
    mstd::vector<bool> _pressStates;
};
} // namespace

ZC_TEST(director_scene_stack_pauses_resumes_and_cleans_scenes) {
    Director& director = Director::getInstance();
    director.shutdown();

    auto* root = new RecordingScene();
    auto* middle = new RecordingScene();
    auto* top = new RecordingScene();
    auto* replacement = new RecordingScene();
    int scheduleCalls = 0;
    int actionSteps = 0;

    director.runWithScene(root);
    ZC_CHECK_EQ(root->enterTransitionFinishedCount, 1);
    root->schedule("tick", [&](float) { ++scheduleCalls; });
    auto* action = new CountingAction(actionSteps);
    root->runAction(action);
    action->release();

    director.getScheduler().update(0.1f);
    director.getActionManager().update(0.1f);
    ZC_CHECK_EQ(scheduleCalls, 1);
    ZC_CHECK_EQ(actionSteps, 1);

    director.pushScene(middle);
    director.pushScene(top);
    ZC_CHECK_EQ(director.getRunningScene(), top);
    ZC_CHECK_EQ(director.getSceneCount(), static_cast<mstd::size_t>(3));
    ZC_CHECK(!root->isRunning());
    ZC_CHECK_EQ(root->exitTransitionStartedCount, 1);
    ZC_CHECK_EQ(middle->enterTransitionFinishedCount, 1);
    ZC_CHECK_EQ(top->enterTransitionFinishedCount, 1);

    director.getScheduler().update(0.1f);
    director.getActionManager().update(0.1f);
    ZC_CHECK_EQ(scheduleCalls, 1);
    ZC_CHECK_EQ(actionSteps, 1);

    director.popToRootScene();
    ZC_CHECK_EQ(director.getRunningScene(), root);
    ZC_CHECK_EQ(director.getSceneCount(), static_cast<mstd::size_t>(1));
    ZC_CHECK_EQ(root->enterTransitionFinishedCount, 2);
    ZC_CHECK_EQ(middle->cleanupCount, 1);
    ZC_CHECK_EQ(top->cleanupCount, 1);

    director.getScheduler().update(0.1f);
    director.getActionManager().update(0.1f);
    ZC_CHECK_EQ(scheduleCalls, 2);
    ZC_CHECK_EQ(actionSteps, 2);

    director.pushScene(top);
    director.popScene();
    ZC_CHECK_EQ(director.getRunningScene(), root);
    ZC_CHECK_EQ(top->cleanupCount, 2);

    director.replaceScene(replacement);
    ZC_CHECK_EQ(director.getRunningScene(), replacement);
    ZC_CHECK_EQ(director.getSceneCount(), static_cast<mstd::size_t>(1));
    ZC_CHECK_EQ(root->cleanupCount, 1);
    ZC_CHECK_EQ(replacement->enterTransitionFinishedCount, 1);
    ZC_CHECK_EQ(director.getScheduler().getScheduledCount(), static_cast<mstd::size_t>(0));
    ZC_CHECK_EQ(director.getActionManager().getRunningActionCount(), static_cast<mstd::size_t>(0));

    director.shutdown();
    ZC_CHECK_EQ(director.getRunningScene(), nullptr);
    ZC_CHECK_EQ(director.getSceneCount(), static_cast<mstd::size_t>(0));
    ZC_CHECK_EQ(root->cleanupCount, 1);
    ZC_CHECK_EQ(replacement->cleanupCount, 1);
    ZC_CHECK_EQ(director.getScheduler().getScheduledCount(), static_cast<mstd::size_t>(0));
    ZC_CHECK_EQ(director.getActionManager().getRunningActionCount(), static_cast<mstd::size_t>(0));

    root->release();
    middle->release();
    top->release();
    replacement->release();
}

ZC_TEST(widget_survives_until_its_removal_callback_returns) {
    Director& director = Director::getInstance();
    director.shutdown();

    auto* scene = new RecordingScene();
    bool callbackCalled = false;
    bool aliveAfterRemoval = false;
    bool destroyed = false;
    auto* widget = new RemovingWidget(destroyed);
    widget->setAnchorPoint({0.f, 0.f});
    widget->setContentSize({100.f, 100.f});
    widget->addEventListener([&](ui::Widget& sender) {
        callbackCalled = true;
        sender.removeFromParent();
        aliveAfterRemoval = !destroyed;
    });
    scene->addChild(widget);
    widget->release();
    director.runWithScene(scene);

    Touch touch;
    touch.setTouchInfo(0, 10.f, 10.f);
    EventTouch touchBegan(EventTouch::EventCode::BEGAN, &touch);
    director.getEventDispatcher().dispatchEvent(touchBegan);

    EventTouch touchEnded(EventTouch::EventCode::ENDED, &touch);
    director.getEventDispatcher().dispatchEvent(touchEnded);

    ZC_CHECK(callbackCalled);
    ZC_CHECK(aliveAfterRemoval);
    ZC_CHECK(destroyed);

    director.shutdown();
    scene->release();
}

ZC_TEST(widget_touch_prefers_topmost_and_drag_out_cancels_click) {
    Director& director = Director::getInstance();
    director.shutdown();

    auto* scene = new RecordingScene();
    auto* bottom = new TrackingWidget();
    auto* top = new TrackingWidget();
    bottom->setAnchorPoint({0.f, 0.f});
    bottom->setContentSize({100.f, 100.f});
    top->setAnchorPoint({0.f, 0.f});
    top->setContentSize({100.f, 100.f});

    int bottomClicks = 0;
    int topClicks = 0;
    bottom->addEventListener([&](ui::Widget&) { ++bottomClicks; });
    top->addEventListener([&](ui::Widget&) { ++topClicks; });
    scene->addChild(bottom);
    scene->addChild(top);
    bottom->release();
    top->release();
    director.runWithScene(scene);

    Touch touch;
    touch.setTouchInfo(0, 10.f, 10.f);
    EventTouch began(EventTouch::EventCode::BEGAN, &touch);
    director.getEventDispatcher().dispatchEvent(began);
    ZC_CHECK(top->pressed());
    ZC_CHECK(!bottom->pressed());

    EventTouch ended(EventTouch::EventCode::ENDED, &touch);
    director.getEventDispatcher().dispatchEvent(ended);
    ZC_CHECK_EQ(topClicks, 1);
    ZC_CHECK_EQ(bottomClicks, 0);

    touch = Touch{};
    touch.setTouchInfo(0, 10.f, 10.f);
    EventTouch dragBegan(EventTouch::EventCode::BEGAN, &touch);
    director.getEventDispatcher().dispatchEvent(dragBegan);
    touch.setTouchInfo(0, 150.f, 150.f);
    EventTouch dragMoved(EventTouch::EventCode::MOVED, &touch);
    director.getEventDispatcher().dispatchEvent(dragMoved);
    ZC_CHECK(!top->pressed());
    EventTouch dragEnded(EventTouch::EventCode::ENDED, &touch);
    director.getEventDispatcher().dispatchEvent(dragEnded);
    ZC_CHECK_EQ(topClicks, 1);
    ZC_CHECK_EQ(bottomClicks, 0);

    touch = Touch{};
    touch.setTouchInfo(0, 10.f, 10.f);
    EventTouch cancelBegan(EventTouch::EventCode::BEGAN, &touch);
    director.getEventDispatcher().dispatchEvent(cancelBegan);
    EventTouch cancelled(EventTouch::EventCode::CANCELLED, &touch);
    director.getEventDispatcher().dispatchEvent(cancelled);
    ZC_CHECK(!top->pressed());
    ZC_CHECK_EQ(topClicks, 1);
    ZC_CHECK_EQ(bottomClicks, 0);

    const auto& pressStates = top->pressStates();
    ZC_CHECK_EQ(pressStates.size(), static_cast<mstd::size_t>(6));
    for (mstd::size_t i = 0; i < pressStates.size(); ++i) {
        ZC_CHECK_EQ(pressStates[i], i % 2 == 0);
    }

    bottom->setLocalZOrder(1);
    touch = Touch{};
    touch.setTouchInfo(0, 10.f, 10.f);
    EventTouch reorderedBegan(EventTouch::EventCode::BEGAN, &touch);
    director.getEventDispatcher().dispatchEvent(reorderedBegan);
    EventTouch reorderedEnded(EventTouch::EventCode::ENDED, &touch);
    director.getEventDispatcher().dispatchEvent(reorderedEnded);
    ZC_CHECK_EQ(topClicks, 1);
    ZC_CHECK_EQ(bottomClicks, 1);

    director.shutdown();
    scene->release();
}

ZC_TEST(widget_touch_places_parent_above_negative_z_child) {
    Director& director = Director::getInstance();
    director.shutdown();

    auto* scene = new RecordingScene();
    auto* parent = new TrackingWidget();
    auto* backChild = new TrackingWidget();
    parent->setAnchorPoint({0.f, 0.f});
    parent->setContentSize({100.f, 100.f});
    backChild->setAnchorPoint({0.f, 0.f});
    backChild->setContentSize({100.f, 100.f});

    int parentClicks = 0;
    int childClicks = 0;
    parent->addEventListener([&](ui::Widget&) { ++parentClicks; });
    backChild->addEventListener([&](ui::Widget&) { ++childClicks; });
    parent->addChild(backChild, -1);
    scene->addChild(parent);
    backChild->release();
    parent->release();
    director.runWithScene(scene);

    Touch touch;
    touch.setTouchInfo(0, 10.f, 10.f);
    EventTouch began(EventTouch::EventCode::BEGAN, &touch);
    director.getEventDispatcher().dispatchEvent(began);
    EventTouch ended(EventTouch::EventCode::ENDED, &touch);
    director.getEventDispatcher().dispatchEvent(ended);

    ZC_CHECK_EQ(parentClicks, 1);
    ZC_CHECK_EQ(childClicks, 0);

    director.shutdown();
    scene->release();
}

ZC_TEST(director_maps_left_mouse_drag_to_touch_sequence) {
    Director& director = Director::getInstance();
    director.shutdown();
    AutoreleasePool pool("mouse touch mapping test");
    mstd::vector<EventTouch::EventCode> eventCodes;
    Vec2 moveDelta{};

    auto* listener = EventListenerTouchOneByOne::create();
    listener->onTouchBegan = [&](Touch& touch, EventTouch& event) {
        eventCodes.push_back(event.getEventCode());
        ZC_CHECK_NEAR(touch.getLocation().x, 10.f, 1e-4);
        ZC_CHECK_NEAR(touch.getStartLocation().y, 20.f, 1e-4);
        return true;
    };
    listener->onTouchMoved = [&](Touch& touch, EventTouch& event) {
        eventCodes.push_back(event.getEventCode());
        moveDelta = touch.getDelta();
    };
    listener->onTouchEnded = [&](Touch& touch, EventTouch& event) {
        eventCodes.push_back(event.getEventCode());
        ZC_CHECK_NEAR(touch.getLocation().x, 18.f, 1e-4);
        ZC_CHECK_NEAR(touch.getLocation().y, 29.f, 1e-4);
    };
    director.getEventDispatcher().addEventListenerWithFixedPriority(listener, -1);

    ViewDelegate& viewDelegate = director;
    viewDelegate.onViewMouseButtonEvent(
        static_cast<int>(EventMouse::MouseButton::BUTTON_RIGHT), 0, true, 1.f, 2.f);
    viewDelegate.onViewMouseButtonEvent(
        static_cast<int>(EventMouse::MouseButton::BUTTON_RIGHT), 0, false, 1.f, 2.f);
    ZC_CHECK(eventCodes.empty());

    viewDelegate.onViewMouseButtonEvent(
        static_cast<int>(EventMouse::MouseButton::BUTTON_LEFT), 0, true, 10.f, 20.f);
    viewDelegate.onViewMouseMoveEvent(15.f, 25.f, 5.f, 5.f);
    viewDelegate.onViewMouseButtonEvent(
        static_cast<int>(EventMouse::MouseButton::BUTTON_LEFT), 0, false, 18.f, 29.f);

    ZC_CHECK_EQ(eventCodes.size(), static_cast<mstd::size_t>(3));
    ZC_CHECK_EQ(eventCodes[0], EventTouch::EventCode::BEGAN);
    ZC_CHECK_EQ(eventCodes[1], EventTouch::EventCode::MOVED);
    ZC_CHECK_EQ(eventCodes[2], EventTouch::EventCode::ENDED);
    ZC_CHECK_NEAR(moveDelta.x, 5.f, 1e-4);
    ZC_CHECK_NEAR(moveDelta.y, 5.f, 1e-4);

    director.getEventDispatcher().removeAllEventListeners();
}

ZC_TEST(director_applies_frame_callback_scene_changes_at_a_safe_point) {
    Director& director = Director::getInstance();
    director.shutdown();
    ZC_CHECK(director.init(320, 180, "headless"));

    auto* root = new RecordingScene();
    auto* next = new RecordingScene();
    bool callbackRan = false;
    bool rootWasStillRunningInsideCallback = false;
    director.runWithScene(root);
    root->scheduleOnce("push_scene", [&](float) {
        director.pushScene(next);
        callbackRan = true;
        rootWasStillRunningInsideCallback = director.getRunningScene() == root;
    });

    ZC_CHECK(director.mainLoop());
    ZC_CHECK(callbackRan);
    ZC_CHECK(rootWasStillRunningInsideCallback);
    ZC_CHECK_EQ(director.getRunningScene(), next);
    ZC_CHECK_EQ(director.getSceneCount(), static_cast<mstd::size_t>(2));
    ZC_CHECK_EQ(root->exitTransitionStartedCount, 1);
    ZC_CHECK_EQ(next->enterTransitionFinishedCount, 1);

    director.shutdown();
    root->release();
    next->release();
}

ZC_TEST(director_finishes_incoming_lifecycle_after_fade_transition) {
    Director& director = Director::getInstance();
    director.shutdown();
    ZC_CHECK(director.init(320, 180, "headless"));

    auto* outgoing = new RecordingScene();
    auto* incoming = new RecordingScene();
    auto* incomingChild = new RecordingScene();
    incoming->addChild(incomingChild);
    director.runWithScene(outgoing);
    director.replaceScene(incoming, 0.18f);

    ZC_CHECK(director.mainLoop());
    ZC_CHECK_EQ(director.getRunningScene(), outgoing);
    ZC_CHECK(director.isSceneTransitioning());

    ZC_CHECK(director.mainLoop());
    ZC_CHECK_EQ(director.getRunningScene(), incoming);
    ZC_CHECK_EQ(outgoing->exitTransitionStartedCount, 1);
    ZC_CHECK_EQ(incoming->enterCount, 1);
    ZC_CHECK_EQ(incoming->enterTransitionFinishedCount, 0);
    ZC_CHECK_EQ(incomingChild->enterCount, 1);
    ZC_CHECK_EQ(incomingChild->enterTransitionFinishedCount, 0);

    ZC_CHECK(director.mainLoop());
    ZC_CHECK(director.mainLoop());
    ZC_CHECK(!director.isSceneTransitioning());
    ZC_CHECK_EQ(incoming->enterTransitionFinishedCount, 1);
    ZC_CHECK_EQ(incomingChild->enterTransitionFinishedCount, 1);

    director.shutdown();
    outgoing->release();
    incoming->release();
    incomingChild->release();
}

#include "ZCTestFramework.h"

#include "2d/ZCScene.h"
#include "base/ZCAction.h"
#include "base/ZCDirector.h"
#include "base/ZCEvent.h"
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

    EventMouse mouseDown(EventMouse::MouseEventType::MOUSE_DOWN);
    mouseDown.setMouseButton(EventMouse::MouseButton::BUTTON_LEFT);
    mouseDown.setPosition(10.f, 10.f);
    director.getEventDispatcher().dispatchEvent(mouseDown);

    EventMouse mouseUp(EventMouse::MouseEventType::MOUSE_UP);
    mouseUp.setMouseButton(EventMouse::MouseButton::BUTTON_LEFT);
    mouseUp.setPosition(10.f, 10.f);
    director.getEventDispatcher().dispatchEvent(mouseUp);

    ZC_CHECK(callbackCalled);
    ZC_CHECK(aliveAfterRemoval);
    ZC_CHECK(destroyed);

    director.shutdown();
    scene->release();
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

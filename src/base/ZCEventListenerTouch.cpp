#include "base/ZCEventListener.h"

#include "base/ZCStd.h"

namespace zocos {

EventListenerTouchOneByOne* EventListenerTouchOneByOne::create() {
    auto* listener = new (mstd::nothrow) EventListenerTouchOneByOne();
    if (listener && listener->init()) {
        listener->autorelease();
        return listener;
    }
    delete listener;
    return nullptr;
}

bool EventListenerTouchOneByOne::init() { return true; }

bool EventListenerTouchOneByOne::hasCallbacks() const {
    return static_cast<bool>(onTouchBegan);
}

bool EventListenerTouchOneByOne::dispatchEvent(Event& event) {
    if (event.getType() != Event::Type::Touch) {
        return false;
    }

    auto& touchEvent = static_cast<EventTouch&>(event);
    if (touchEvent.getTouches().empty() || !touchEvent.getTouches().front()) {
        return false;
    }

    Touch& touch = *touchEvent.getTouches().front();
    switch (touchEvent.getEventCode()) {
    case EventTouch::EventCode::BEGAN:
        return onTouchBegan && onTouchBegan(touch, touchEvent);
    case EventTouch::EventCode::MOVED:
        if (onTouchMoved) {
            onTouchMoved(touch, touchEvent);
            return true;
        }
        break;
    case EventTouch::EventCode::ENDED:
        if (onTouchEnded) {
            onTouchEnded(touch, touchEvent);
            return true;
        }
        break;
    case EventTouch::EventCode::CANCELLED:
        if (onTouchCancelled) {
            onTouchCancelled(touch, touchEvent);
            return true;
        }
        break;
    }
    return false;
}

bool EventListenerTouchOneByOne::hasClaimedTouch(int touchId) const {
    return mstd::find(_claimedTouchIds.begin(), _claimedTouchIds.end(), touchId) !=
           _claimedTouchIds.end();
}

void EventListenerTouchOneByOne::claimTouch(int touchId) {
    if (!hasClaimedTouch(touchId)) {
        _claimedTouchIds.push_back(touchId);
    }
}

void EventListenerTouchOneByOne::unclaimTouch(int touchId) {
    auto it = mstd::find(_claimedTouchIds.begin(), _claimedTouchIds.end(), touchId);
    if (it != _claimedTouchIds.end()) {
        _claimedTouchIds.erase(it);
    }
}

EventListenerTouchOneByOne::EventListenerTouchOneByOne()
    : EventListener(Type::TouchOneByOne) {}

} // namespace zocos

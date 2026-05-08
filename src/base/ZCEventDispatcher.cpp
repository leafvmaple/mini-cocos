#include "base/ZCEventDispatcher.h"

#include "2d/ZCNode.h"

#include <algorithm>
#include <utility>

namespace zocos {

namespace {

bool hasCallbacks(const EventListener* listener) {
    if (!listener) {
        return false;
    }

    if (listener->getType() == EventListener::Type::Keyboard) {
        const auto* keyboard = static_cast<const EventListenerKeyboard*>(listener);
        return static_cast<bool>(keyboard->onKeyPressed) || static_cast<bool>(keyboard->onKeyReleased);
    }

    const auto* mouse = static_cast<const EventListenerMouse*>(listener);
    return static_cast<bool>(mouse->onMouseDown) || static_cast<bool>(mouse->onMouseUp)
        || static_cast<bool>(mouse->onMouseMove) || static_cast<bool>(mouse->onMouseScroll);
}

} // namespace

EventDispatcher::ListenerID EventDispatcher::addEventListener(EventListener* listener, Node* target,
                                                              int priority) {
    if (!target || !listener || !hasCallbacks(listener)) {
        return 0;
    }

    ListenerEntry entry;
    entry.id = _nextListenerId++;
    entry.target = target;
    entry.listener = listener;
    entry.priority = priority;
    entry.order = _nextOrder++;
    entry.listener->retain();

    addListener(std::move(entry));
    return entry.id;
}

void EventDispatcher::removeListener(ListenerID id) {
    if (_dispatching) {
        for (auto& listener : _listeners) {
            if (listener.id == id) {
                listener.removed = true;
            }
        }
        for (auto& listener : _pendingListeners) {
            if (listener.id == id) {
                listener.removed = true;
            }
        }
        return;
    }

    for (auto it = _listeners.begin(); it != _listeners.end();) {
        if (it->id == id) {
            releaseListenerEntry(*it);
            it = _listeners.erase(it);
        } else {
            ++it;
        }
    }

    for (auto it = _pendingListeners.begin(); it != _pendingListeners.end();) {
        if (it->id == id) {
            releaseListenerEntry(*it);
            it = _pendingListeners.erase(it);
        } else {
            ++it;
        }
    }
}

void EventDispatcher::removeListenersForTarget(Node* target) {
    if (_dispatching) {
        for (auto& listener : _listeners) {
            if (listener.target == target) {
                listener.removed = true;
            }
        }
        for (auto& listener : _pendingListeners) {
            if (listener.target == target) {
                listener.removed = true;
            }
        }
        return;
    }

    for (auto it = _listeners.begin(); it != _listeners.end();) {
        if (it->target == target) {
            releaseListenerEntry(*it);
            it = _listeners.erase(it);
        } else {
            ++it;
        }
    }

    for (auto it = _pendingListeners.begin(); it != _pendingListeners.end();) {
        if (it->target == target) {
            releaseListenerEntry(*it);
            it = _pendingListeners.erase(it);
        } else {
            ++it;
        }
    }
}

void EventDispatcher::removeAllListeners() {
    if (_dispatching) {
        for (auto& listener : _listeners) {
            listener.removed = true;
        }
        for (auto& listener : _pendingListeners) {
            listener.removed = true;
        }
        return;
    }

    for (auto& listener : _listeners) {
        releaseListenerEntry(listener);
    }
    for (auto& listener : _pendingListeners) {
        releaseListenerEntry(listener);
    }

    _listeners.clear();
    _pendingListeners.clear();
}

void EventDispatcher::dispatchEvent(Event& event) {
    mergePending();
    if (_listeners.empty()) {
        return;
    }

    sortListenersIfNeeded();

    event.resetForDispatch();
    _dispatching = true;
    for (auto& listener : _listeners) {
        if (listener.removed || !listener.target || !listener.listener) {
            continue;
        }
        if (!listener.target->isRunning() || listener.target->isPaused()) {
            continue;
        }
        if (!listener.listener->isEnabled()) {
            continue;
        }

        bool invoked = false;

        if (listener.listener->getType() == EventListener::Type::Keyboard
            && event.getType() == Event::Type::Keyboard) {
            auto* keyboard = static_cast<EventListenerKeyboard*>(listener.listener);
            auto& keyEvent = static_cast<EventKeyboard&>(event);
            if (keyEvent.isPressed()) {
                if (keyboard->onKeyPressed) {
                    event.setCurrentTarget(listener.target);
                    keyboard->onKeyPressed(keyEvent);
                    invoked = true;
                }
            } else {
                if (keyboard->onKeyReleased) {
                    event.setCurrentTarget(listener.target);
                    keyboard->onKeyReleased(keyEvent);
                    invoked = true;
                }
            }
        } else if (listener.listener->getType() == EventListener::Type::Mouse) {
            auto* mouse = static_cast<EventListenerMouse*>(listener.listener);
            switch (event.getType()) {
            case Event::Type::MouseButton: {
                auto& mouseButtonEvent = static_cast<EventMouseButton&>(event);
                if (mouseButtonEvent.isPressed()) {
                    if (mouse->onMouseDown) {
                        event.setCurrentTarget(listener.target);
                        mouse->onMouseDown(mouseButtonEvent);
                        invoked = true;
                    }
                } else {
                    if (mouse->onMouseUp) {
                        event.setCurrentTarget(listener.target);
                        mouse->onMouseUp(mouseButtonEvent);
                        invoked = true;
                    }
                }
                break;
            }
            case Event::Type::MouseMove: {
                if (mouse->onMouseMove) {
                    event.setCurrentTarget(listener.target);
                    mouse->onMouseMove(static_cast<EventMouseMove&>(event));
                    invoked = true;
                }
                break;
            }
            case Event::Type::MouseScroll: {
                if (mouse->onMouseScroll) {
                    event.setCurrentTarget(listener.target);
                    mouse->onMouseScroll(static_cast<EventMouseScroll&>(event));
                    invoked = true;
                }
                break;
            }
            case Event::Type::Keyboard:
                break;
            }
        }

        if (invoked && event.isStopped()) {
            break;
        }
    }
    _dispatching = false;
    event.setCurrentTarget(nullptr);

    for (auto it = _listeners.begin(); it != _listeners.end();) {
        if (it->removed || it->target == nullptr || it->listener == nullptr) {
            releaseListenerEntry(*it);
            it = _listeners.erase(it);
        } else {
            ++it;
        }
    }

    mergePending();
    sortListenersIfNeeded();
}

std::size_t EventDispatcher::getListenerCount() const {
    std::size_t count = 0;
    for (const auto& listener : _listeners) {
        if (!listener.removed && listener.target && listener.listener) {
            ++count;
        }
    }
    for (const auto& listener : _pendingListeners) {
        if (!listener.removed && listener.target && listener.listener) {
            ++count;
        }
    }
    return count;
}

void EventDispatcher::addListener(ListenerEntry entry) {
    if (_dispatching) {
        _pendingListeners.push_back(std::move(entry));
        return;
    }
    _listeners.push_back(std::move(entry));
    _dirtyOrder = true;
}

void EventDispatcher::mergePending() {
    if (_pendingListeners.empty()) {
        return;
    }

    for (auto& listener : _pendingListeners) {
        if (!listener.removed && listener.target && listener.listener) {
            _listeners.push_back(std::move(listener));
            _dirtyOrder = true;
        } else {
            releaseListenerEntry(listener);
        }
    }
    _pendingListeners.clear();
}

void EventDispatcher::sortListenersIfNeeded() {
    if (!_dirtyOrder) {
        return;
    }

    std::sort(_listeners.begin(), _listeners.end(), [](const ListenerEntry& a, const ListenerEntry& b) {
        if (a.priority != b.priority) {
            return a.priority < b.priority;
        }
        return a.order < b.order;
    });
    _dirtyOrder = false;
}

void EventDispatcher::releaseListenerEntry(ListenerEntry& entry) {
    if (entry.listener) {
        entry.listener->release();
        entry.listener = nullptr;
    }
}

} // namespace zocos

#include "base/ZCEventDispatcher.h"

#include "2d/ZCNode.h"

#include <algorithm>
#include <utility>

namespace zocos {

EventDispatcher::ListenerID EventDispatcher::addKeyboardListener(
    Node* target, KeyboardCallback onPressed, KeyboardCallback onReleased, int priority) {
    if (!target || (!onPressed && !onReleased)) {
        return 0;
    }

    ListenerEntry entry;
    entry.id = _nextListenerId++;
    entry.type = Event::Type::Keyboard;
    entry.target = target;
    entry.priority = priority;
    entry.order = _nextOrder++;
    entry.callback = [onPressed = std::move(onPressed), onReleased = std::move(onReleased)](Event& e) {
        auto& keyEvent = static_cast<EventKeyboard&>(e);
        if (keyEvent.isPressed()) {
            if (onPressed) onPressed(keyEvent);
        } else {
            if (onReleased) onReleased(keyEvent);
        }
    };
    addListener(std::move(entry));
    return entry.id;
}

EventDispatcher::ListenerID EventDispatcher::addMouseButtonListener(
    Node* target, MouseButtonCallback onPressed, MouseButtonCallback onReleased, int priority) {
    if (!target || (!onPressed && !onReleased)) {
        return 0;
    }

    ListenerEntry entry;
    entry.id = _nextListenerId++;
    entry.type = Event::Type::MouseButton;
    entry.target = target;
    entry.priority = priority;
    entry.order = _nextOrder++;
    entry.callback = [onPressed = std::move(onPressed), onReleased = std::move(onReleased)](Event& e) {
        auto& mouseEvent = static_cast<EventMouseButton&>(e);
        if (mouseEvent.isPressed()) {
            if (onPressed) onPressed(mouseEvent);
        } else {
            if (onReleased) onReleased(mouseEvent);
        }
    };
    addListener(std::move(entry));
    return entry.id;
}

EventDispatcher::ListenerID EventDispatcher::addMouseMoveListener(Node* target, MouseMoveCallback onMoved,
                                                                  int priority) {
    if (!target || !onMoved) {
        return 0;
    }

    ListenerEntry entry;
    entry.id = _nextListenerId++;
    entry.type = Event::Type::MouseMove;
    entry.target = target;
    entry.priority = priority;
    entry.order = _nextOrder++;
    entry.callback = [onMoved = std::move(onMoved)](Event& e) {
        auto& moveEvent = static_cast<EventMouseMove&>(e);
        onMoved(moveEvent);
    };
    addListener(std::move(entry));
    return entry.id;
}

EventDispatcher::ListenerID EventDispatcher::addMouseScrollListener(Node* target,
                                                                    MouseScrollCallback onScrolled,
                                                                    int priority) {
    if (!target || !onScrolled) {
        return 0;
    }

    ListenerEntry entry;
    entry.id = _nextListenerId++;
    entry.type = Event::Type::MouseScroll;
    entry.target = target;
    entry.priority = priority;
    entry.order = _nextOrder++;
    entry.callback = [onScrolled = std::move(onScrolled)](Event& e) {
        auto& scrollEvent = static_cast<EventMouseScroll&>(e);
        onScrolled(scrollEvent);
    };
    addListener(std::move(entry));
    return entry.id;
}

void EventDispatcher::removeListener(ListenerID id) {
    if (id == 0) {
        return;
    }

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

    _listeners.erase(std::remove_if(_listeners.begin(), _listeners.end(),
                                    [id](const ListenerEntry& listener) { return listener.id == id; }),
                     _listeners.end());
    _pendingListeners.erase(
        std::remove_if(_pendingListeners.begin(), _pendingListeners.end(),
                       [id](const ListenerEntry& listener) { return listener.id == id; }),
        _pendingListeners.end());
}

void EventDispatcher::removeListenersForTarget(Node* target) {
    if (!target) {
        return;
    }

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

    _listeners.erase(std::remove_if(_listeners.begin(), _listeners.end(),
                                    [target](const ListenerEntry& listener) {
                                        return listener.target == target;
                                    }),
                     _listeners.end());

    _pendingListeners.erase(
        std::remove_if(_pendingListeners.begin(), _pendingListeners.end(),
                       [target](const ListenerEntry& listener) { return listener.target == target; }),
        _pendingListeners.end());
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
        if (listener.removed || listener.type != event.getType() || !listener.callback || !listener.target) {
            continue;
        }
        if (!listener.target->isRunning() || listener.target->isPaused()) {
            continue;
        }

        event.setCurrentTarget(listener.target);
        listener.callback(event);
        if (event.isStopped()) {
            break;
        }
    }
    _dispatching = false;
    event.setCurrentTarget(nullptr);

    _listeners.erase(std::remove_if(_listeners.begin(), _listeners.end(),
                                    [](const ListenerEntry& listener) {
                                        return listener.removed || listener.target == nullptr;
                                    }),
                     _listeners.end());

    mergePending();
    sortListenersIfNeeded();
}

std::size_t EventDispatcher::getListenerCount() const {
    std::size_t count = 0;
    for (const auto& listener : _listeners) {
        if (!listener.removed && listener.target) {
            ++count;
        }
    }
    for (const auto& listener : _pendingListeners) {
        if (!listener.removed && listener.target) {
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
        if (!listener.removed) {
            _listeners.push_back(std::move(listener));
            _dirtyOrder = true;
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

} // namespace zocos

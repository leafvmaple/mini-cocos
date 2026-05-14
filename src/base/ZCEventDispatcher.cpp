#include "base/ZCEventDispatcher.h"

#include "2d/ZCNode.h"

#include <algorithm>
#include <utility>

namespace zocos {

int EventDispatcher::getListenerID(EventListener::Type type) { return static_cast<int>(type); }

int EventDispatcher::getListenerID(Event::Type type) {
    switch (type) {
    case Event::Type::Keyboard:
        return getListenerID(EventListener::Type::Keyboard);
    case Event::Type::Mouse:
        return getListenerID(EventListener::Type::Mouse);
    default:
        break;
    }
    return -1;
}

EventDispatcher::ListenerHandle
EventDispatcher::addEventListenerWithNodePriority(EventListener* listener, Node* node) {
    if (!node || !listener || !listener->hasCallbacks()) {
        return 0;
    }

    ListenerEntry entry;
    entry.handle = _nextListenerHandle++;
    entry.target = node;
    entry.listener = listener;
    entry.priority = 0;
    entry.order = _nextOrder++;
    entry.listener->retain();

    const ListenerHandle handle = entry.handle;
    addEventListenerInternal(std::move(entry));
    return handle;
}

EventDispatcher::ListenerHandle
EventDispatcher::addEventListenerWithFixedPriority(EventListener* listener, int fixedPriority) {
    if (!listener || !listener->hasCallbacks() || fixedPriority == 0) {
        return 0;
    }

    ListenerEntry entry;
    entry.handle = _nextListenerHandle++;
    entry.target = nullptr;
    entry.listener = listener;
    entry.priority = fixedPriority;
    entry.order = _nextOrder++;
    entry.listener->retain();

    const ListenerHandle handle = entry.handle;
    addEventListenerInternal(std::move(entry));
    return handle;
}

void EventDispatcher::removeEventListener(ListenerHandle handle) {
    removeEventListenersIf([handle](const ListenerEntry& entry) { return entry.handle == handle; });
}

void EventDispatcher::removeEventListenersForTarget(Node* target) {
    removeEventListenersIf([target](const ListenerEntry& entry) { return entry.target == target; });
}

void EventDispatcher::removeEventListenersIf(const ListenerCondition& condition) {
    auto markInVector = [&condition](std::vector<ListenerEntry>& entries) {
        for (auto& entry : entries) {
            if (condition(entry)) {
                entry.removed = true;
            }
        }
    };

    auto eraseInVector = [this, &condition](std::vector<ListenerEntry>& entries) {
        bool changed = false;
        for (auto it = entries.begin(); it != entries.end();) {
            if (condition(*it)) {
                releaseListenerEntry(*it);
                it = entries.erase(it);
                changed = true;
            } else {
                ++it;
            }
        }

        return changed;
    };

    if (isDispatching()) {
        for (auto& [_, listeners] : _listenerMap) {
            markInVector(listeners._fixedListeners);
            markInVector(listeners._nodeListeners);
        }

        markInVector(_toAddedListeners);
        return;
    }

    for (auto it = _listenerMap.begin(); it != _listenerMap.end();) {
        auto& listeners = it->second;
        if (eraseInVector(listeners._fixedListeners))
            listeners._dirtyFixedPriority = true;
        if (eraseInVector(listeners._nodeListeners))
            listeners._dirtyNodePriority = true;

        if (listeners.empty()) {
            it = _listenerMap.erase(it);
        } else {
            ++it;
        }
    }

    eraseInVector(_toAddedListeners);
}

void EventDispatcher::removeAllEventListeners() {
    if (isDispatching()) {
        for (auto& [_, listeners] : _listenerMap) {
            for (auto& listener : listeners._fixedListeners) {
                listener.removed = true;
            }
            for (auto& listener : listeners._nodeListeners) {
                listener.removed = true;
            }
        }

        for (auto& listener : _toAddedListeners) {
            listener.removed = true;
        }
        return;
    }

    for (auto& [_, listeners] : _listenerMap) {
        for (auto& listener : listeners._fixedListeners) {
            releaseListenerEntry(listener);
        }
        for (auto& listener : listeners._nodeListeners) {
            releaseListenerEntry(listener);
        }
    }

    for (auto& listener : _toAddedListeners) {
        releaseListenerEntry(listener);
    }

    _listenerMap.clear();
    _toAddedListeners.clear();
}

void EventDispatcher::dispatchEvent(Event& event) {
    if (!isDispatching()) {
        updateListeners();
    }

    const int eventTypeKey = getListenerID(event.getType());
    if (eventTypeKey < 0) {
        return;
    }

    auto iter = _listenerMap.find(eventTypeKey);
    if (iter == _listenerMap.end() || iter->second.empty()) {
        return;
    }

    auto& listeners = iter->second;
    sortEventListeners(listeners);

    event.resetForDispatch();
    ++_inDispatch;

    bool shouldStopPropagation = false;

    if (!listeners._fixedListeners.empty()) {
        shouldStopPropagation = dispatchEventToListeners(event, listeners._fixedListeners, 0,
                                                         listeners._gt0Index, false);
    }

    if (!shouldStopPropagation && !listeners._nodeListeners.empty()) {
        shouldStopPropagation = dispatchEventToListeners(event, listeners._nodeListeners, 0,
                                                         listeners._nodeListeners.size(), true);
    }

    if (!shouldStopPropagation && !listeners._fixedListeners.empty()) {
        shouldStopPropagation =
            dispatchEventToListeners(event, listeners._fixedListeners, listeners._gt0Index,
                                     listeners._fixedListeners.size(), false);
    }

    --_inDispatch;
    event.setCurrentTarget(nullptr);

    if (!isDispatching()) {
        updateListeners();
    }
}

std::size_t EventDispatcher::getListenerCount() const {
    std::size_t count = 0;
    for (const auto& [_, listeners] : _listenerMap) {
        for (const auto& listener : listeners._fixedListeners) {
            if (!listener.removed && listener.listener) {
                ++count;
            }
        }

        for (const auto& listener : listeners._nodeListeners) {
            if (!listener.removed && listener.listener && listener.target) {
                ++count;
            }
        }
    }

    for (const auto& listener : _toAddedListeners) {
        if (!listener.removed && listener.listener) {
            if (listener.priority == 0) {
                if (listener.target) {
                    ++count;
                }
            } else {
                ++count;
            }
        }
    }

    return count;
}

void EventDispatcher::addEventListenerInternal(ListenerEntry entry) {
    if (isDispatching()) {
        _toAddedListeners.push_back(std::move(entry));
        return;
    }

    forceAddEventListener(std::move(entry));
}

void EventDispatcher::forceAddEventListener(ListenerEntry entry) {
    if (!entry.listener) {
        return;
    }

    auto& listeners = _listenerMap[getListenerID(entry.listener->getType())];
    if (entry.priority == 0) {
        listeners._nodeListeners.push_back(std::move(entry));
        listeners._dirtyNodePriority = true;
    } else {
        listeners._fixedListeners.push_back(std::move(entry));
        listeners._dirtyFixedPriority = true;
    }
}

void EventDispatcher::updateListeners() {
    if (isDispatching()) {
        return;
    }

    if (!_toAddedListeners.empty()) {
        for (auto& listener : _toAddedListeners) {
            const bool sceneGraphValid = (listener.priority != 0) || (listener.target != nullptr);
            if (!listener.removed && listener.listener && sceneGraphValid) {
                forceAddEventListener(std::move(listener));
            } else {
                releaseListenerEntry(listener);
            }
        }

        _toAddedListeners.clear();
    }

    cleanToRemovedListeners();
}

void EventDispatcher::sortEventListeners(EventListenerVector& listeners) {
    if (listeners._dirtyFixedPriority) {
        std::stable_sort(listeners._fixedListeners.begin(), listeners._fixedListeners.end(),
                         [](const ListenerEntry& a, const ListenerEntry& b) {
                             if (a.priority != b.priority) {
                                 return a.priority < b.priority;
                             }
                             return a.order < b.order;
                         });

        std::size_t index = 0;
        while (index < listeners._fixedListeners.size() &&
               listeners._fixedListeners[index].priority < 0) {
            ++index;
        }

        listeners._gt0Index = index;
        listeners._dirtyFixedPriority = false;
    }

    if (listeners._dirtyNodePriority) {
        // Placeholder: until node-priority traversal is implemented, preserve registration order.
        std::stable_sort(
            listeners._nodeListeners.begin(), listeners._nodeListeners.end(),
            [](const ListenerEntry& a, const ListenerEntry& b) { return a.order < b.order; });
        listeners._dirtyNodePriority = false;
    }
}

bool EventDispatcher::dispatchEventToListeners(Event& event, std::vector<ListenerEntry>& listeners,
                                               std::size_t begin, std::size_t end,
                                               bool sceneGraphPriority) {
    const std::size_t size = listeners.size();
    if (begin >= size) {
        return false;
    }

    const std::size_t clampedEnd = std::min(end, size);
    for (std::size_t i = begin; i < clampedEnd; ++i) {
        auto& listener = listeners[i];
        if (listener.removed || !listener.listener) {
            continue;
        }

        if (sceneGraphPriority) {
            if (!listener.target) {
                continue;
            }
            if (!listener.target->isRunning() || listener.target->isPaused()) {
                continue;
            }
        }

        if (!listener.listener->isEnabled()) {
            continue;
        }

        Node* previousTarget = event.getCurrentTarget();
        event.setCurrentTarget(sceneGraphPriority ? listener.target : nullptr);
        const bool invoked = listener.listener->dispatchEvent(event);
        if (!invoked) {
            event.setCurrentTarget(previousTarget);
        }

        if (invoked && event.isStopped()) {
            return true;
        }
    }

    return false;
}

bool EventDispatcher::cleanRemovedListenersInVector(std::vector<ListenerEntry>& listeners) {
    bool changed = false;
    for (auto it = listeners.begin(); it != listeners.end();) {
        if (it->removed || !it->listener || (it->priority == 0 && !it->target)) {
            releaseListenerEntry(*it);
            it = listeners.erase(it);
            changed = true;
        } else {
            ++it;
        }
    }
    return changed;
}

void EventDispatcher::cleanToRemovedListeners() {
    for (auto it = _listenerMap.begin(); it != _listenerMap.end();) {
        auto& listeners = it->second;

        if (cleanRemovedListenersInVector(listeners._fixedListeners)) {
            listeners._dirtyFixedPriority = true;
        }

        if (cleanRemovedListenersInVector(listeners._nodeListeners)) {
            listeners._dirtyNodePriority = true;
        }

        if (listeners.empty()) {
            it = _listenerMap.erase(it);
        } else {
            ++it;
        }
    }
}

void EventDispatcher::releaseListenerEntry(ListenerEntry& entry) {
    if (entry.listener) {
        entry.listener->release();
        entry.listener = nullptr;
    }
}

} // namespace zocos

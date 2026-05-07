#pragma once

#include "base/ZCEvent.h"

#include <cstddef>
#include <cstdint>
#include <functional>
#include <vector>

namespace zocos {

class Node;

class EventDispatcher {
public:
    using ListenerID = std::uint64_t;
    using KeyboardCallback = std::function<void(EventKeyboard&)>;
    using MouseButtonCallback = std::function<void(EventMouseButton&)>;
    using MouseMoveCallback = std::function<void(EventMouseMove&)>;
    using MouseScrollCallback = std::function<void(EventMouseScroll&)>;

    ListenerID addKeyboardListener(Node* target, KeyboardCallback onPressed,
                                   KeyboardCallback onReleased = {}, int priority = 0);

    ListenerID addMouseButtonListener(Node* target, MouseButtonCallback onPressed,
                                      MouseButtonCallback onReleased = {}, int priority = 0);

    ListenerID addMouseMoveListener(Node* target, MouseMoveCallback onMoved, int priority = 0);
    ListenerID addMouseScrollListener(Node* target, MouseScrollCallback onScrolled, int priority = 0);

    void removeListener(ListenerID id);
    void removeListenersForTarget(Node* target);
    void removeAllListeners();

    void dispatchEvent(Event& event);

    std::size_t getListenerCount() const;

private:
    struct ListenerEntry {
        ListenerID id = 0;
        Event::Type type = Event::Type::Keyboard;
        Node* target = nullptr;
        int priority = 0;
        std::size_t order = 0;
        bool removed = false;
        std::function<void(Event&)> callback;
    };

    void addListener(ListenerEntry entry);
    void mergePending();
    void sortListenersIfNeeded();

    ListenerID _nextListenerId = 1;
    std::size_t _nextOrder = 0;
    bool _dispatching = false;
    bool _dirtyOrder = false;
    std::vector<ListenerEntry> _listeners;
    std::vector<ListenerEntry> _pendingListeners;
};

} // namespace zocos

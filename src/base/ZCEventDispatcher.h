#pragma once

#include "base/ZCEvent.h"
#include "base/ZCEventListener.h"

#include <cstddef>
#include <cstdint>
#include <vector>

namespace zocos {

class Node;

class EventDispatcher {
public:
    using ListenerID = std::uint64_t;

    ListenerID addEventListener(EventListener* listener, Node* target, int priority = 0);

    void removeListener(ListenerID id);
    void removeListenersForTarget(Node* target);
    void removeAllListeners();

    void dispatchEvent(Event& event);

    std::size_t getListenerCount() const;

private:
    struct ListenerEntry {
        ListenerID id = 0;
        Node* target = nullptr;
        EventListener* listener = nullptr;
        int priority = 0;
        std::size_t order = 0;
        bool removed = false;
    };

    void addListener(ListenerEntry entry);
    void mergePending();
    void sortListenersIfNeeded();
    void releaseListenerEntry(ListenerEntry& entry);

    ListenerID _nextListenerId = 1;
    std::size_t _nextOrder = 0;
    bool _dispatching = false;
    bool _dirtyOrder = false;
    std::vector<ListenerEntry> _listeners;
    std::vector<ListenerEntry> _pendingListeners;
};

} // namespace zocos

#pragma once

#include "base/ZCEvent.h"
#include "base/ZCEventListener.h"

#include <cstddef>
#include <cstdint>
#include <functional>
#include <unordered_map>
#include <vector>

namespace zocos {

class Node;

class EventDispatcher {
public:
    using ListenerHandle = std::uint64_t;

    ListenerHandle addEventListenerWithNodePriority(EventListener* listener, Node* node);
    ListenerHandle addEventListenerWithFixedPriority(EventListener* listener, int fixedPriority);
    void removeEventListener(ListenerHandle handle);
    void removeEventListenersForTarget(Node* target);
    void removeAllEventListeners();

    void dispatchEvent(Event& event);

    std::size_t getListenerCount() const;

private:
    struct ListenerEntry {
        ListenerHandle handle = 0;
        Node* target = nullptr;
        EventListener* listener = nullptr;
        int priority = 0;
        bool removed = false;
    };

    using ListenerVector = std::vector<ListenerEntry>;

    struct EventListenerVector {
        ListenerVector _fixedListeners;
        ListenerVector _nodeListeners;
        std::size_t _gt0Index = 0;
        bool _dirtyFixedPriority = false;
        bool _dirtyNodePriority = false;

        bool empty() const { return _fixedListeners.empty() && _nodeListeners.empty(); }
    };

    using ListenerCondition = std::function<bool(const ListenerEntry&)>;
    using ListenerEvent = std::function<bool(ListenerEntry&)>;

    void addEventListenerInternal(ListenerEntry entry);
    void forceAddEventListener(ListenerEntry entry);
    void removeEventListenersIf(const ListenerCondition& condition);
    void updateListeners();
    void sortEventListeners(EventListenerVector& listeners);
    void visitTarget(Node* node);
    bool dispatchEventToListeners(EventListenerVector& listeners, const ListenerEvent& onEvent);
    bool cleanRemovedListenersInVector(ListenerVector& listeners);
    void cleanToRemovedListeners();
    void releaseListenerEntry(ListenerEntry& entry);

    static int getListenerID(EventListener::Type type);
    static int getListenerID(Event::Type type);
    bool isDispatching() const { return _inDispatch > 0; }

    ListenerHandle _nextListenerHandle = 1;
    int _inDispatch = 0;
    std::unordered_map<int, EventListenerVector> _listenerMap;
    std::unordered_map<Node*, std::size_t> _nodePriorityMap;
    std::size_t _nodePriorityIndex = 0;
    ListenerVector _toAddedListeners;
};

} // namespace zocos

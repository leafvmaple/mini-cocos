#pragma once

#include <cstddef>
#include "base/ZCStd.h"

namespace zocos {

class Action;
class Node;

class ActionManager {
public:
    void addAction(Action* action, Node* target);

    void removeAction(Action* action);
    void removeAllActionsFromTarget(Node* target);
    void removeAllActions();

    void update(float dt);

    mstd::size_t getRunningActionCount() const;

private:
    struct Entry {
        Node* target = nullptr;
        Action* action = nullptr;
        bool removed = false;
    };

    void addEntry(Entry entry);
    void mergePendingEntries();
    void compactEntries();
    void releaseEntry(Entry& entry);

    bool _updating = false;
    bool _compacting = false;
    mstd::vector<Entry> _entries;
    mstd::vector<Entry> _pendingEntries;
};

} // namespace zocos

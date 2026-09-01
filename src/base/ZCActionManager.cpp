#include "base/ZCActionManager.h"

#include "2d/ZCNode.h"
#include "base/ZCAction.h"

#include "base/ZCStd.h"

namespace zocos {

void ActionManager::addAction(Action* action, Node* target) {
    Entry entry;
    entry.target = target;
    entry.action = action;
    entry.removed = false;

    entry.target->retain();
    entry.action->retain();
    entry.action->startWithTarget(entry.target);

    addEntry(mstd::move(entry));
}

void ActionManager::removeAction(Action* action) {
    for (auto& entry : _entries) {
        if (entry.action == action) {
            entry.removed = true;
        }
    }
    for (auto& entry : _pendingEntries) {
        if (entry.action == action) {
            entry.removed = true;
        }
    }

    if (!_updating && !_compacting) {
        compactEntries();
    }
}

void ActionManager::removeActionByTag(int tag, Node* target) {
    if (tag == Action::InvalidTag || !target) {
        return;
    }

    bool found = false;
    for (auto& entry : _entries) {
        if (!entry.removed && entry.target == target && entry.action &&
            entry.action->getTag() == tag) {
            entry.removed = true;
            found = true;
            break;
        }
    }
    if (!found) {
        for (auto& entry : _pendingEntries) {
            if (!entry.removed && entry.target == target && entry.action &&
                entry.action->getTag() == tag) {
                entry.removed = true;
                break;
            }
        }
    }

    if (!_updating && !_compacting) {
        compactEntries();
    }
}

void ActionManager::removeAllActionsByTag(int tag, Node* target) {
    if (tag == Action::InvalidTag || !target) {
        return;
    }

    for (auto& entry : _entries) {
        if (!entry.removed && entry.target == target && entry.action &&
            entry.action->getTag() == tag) {
            entry.removed = true;
        }
    }
    for (auto& entry : _pendingEntries) {
        if (!entry.removed && entry.target == target && entry.action &&
            entry.action->getTag() == tag) {
            entry.removed = true;
        }
    }

    if (!_updating && !_compacting) {
        compactEntries();
    }
}

void ActionManager::removeAllActionsFromTarget(Node* target) {
    for (auto& entry : _entries) {
        if (entry.target == target) {
            entry.removed = true;
        }
    }
    for (auto& entry : _pendingEntries) {
        if (entry.target == target) {
            entry.removed = true;
        }
    }

    if (!_updating && !_compacting) {
        compactEntries();
    }
}

void ActionManager::removeAllActions() {
    for (auto& entry : _entries) {
        entry.removed = true;
    }
    for (auto& entry : _pendingEntries) {
        entry.removed = true;
    }

    if (!_updating && !_compacting) {
        compactEntries();
    }
}

Action* ActionManager::getActionByTag(int tag, const Node* target) const {
    if (tag == Action::InvalidTag || !target) {
        return nullptr;
    }

    for (const auto& entry : _entries) {
        if (!entry.removed && entry.target == target && entry.action &&
            entry.action->getTag() == tag) {
            return entry.action;
        }
    }
    for (const auto& entry : _pendingEntries) {
        if (!entry.removed && entry.target == target && entry.action &&
            entry.action->getTag() == tag) {
            return entry.action;
        }
    }
    return nullptr;
}

mstd::size_t ActionManager::getNumberOfRunningActionsInTarget(const Node* target) const {
    if (!target) {
        return 0;
    }

    mstd::size_t count = 0;
    for (const auto& entry : _entries) {
        if (!entry.removed && entry.target == target && entry.action) {
            ++count;
        }
    }
    for (const auto& entry : _pendingEntries) {
        if (!entry.removed && entry.target == target && entry.action) {
            ++count;
        }
    }
    return count;
}

mstd::size_t ActionManager::getNumberOfRunningActionsInTargetByTag(const Node* target,
                                                                    int tag) const {
    if (!target || tag == Action::InvalidTag) {
        return 0;
    }

    mstd::size_t count = 0;
    for (const auto& entry : _entries) {
        if (!entry.removed && entry.target == target && entry.action &&
            entry.action->getTag() == tag) {
            ++count;
        }
    }
    for (const auto& entry : _pendingEntries) {
        if (!entry.removed && entry.target == target && entry.action &&
            entry.action->getTag() == tag) {
            ++count;
        }
    }
    return count;
}

void ActionManager::update(float dt) {
    mergePendingEntries();
    if (_entries.empty()) {
        return;
    }

    _updating = true;
    for (auto& entry : _entries) {
        if (entry.removed || !entry.target || !entry.action) {
            continue;
        }
        if (!entry.target->isRunning() || entry.target->isPaused()) {
            continue;
        }

        entry.action->step(dt);
        if (entry.action->isDone()) {
            entry.action->stop();
            entry.removed = true;
        }
    }
    _updating = false;

    compactEntries();
    mergePendingEntries();
}

mstd::size_t ActionManager::getRunningActionCount() const {
    mstd::size_t count = 0;
    for (const auto& entry : _entries) {
        if (!entry.removed && entry.target && entry.action) {
            ++count;
        }
    }
    for (const auto& entry : _pendingEntries) {
        if (!entry.removed && entry.target && entry.action) {
            ++count;
        }
    }
    return count;
}

void ActionManager::addEntry(Entry entry) {
    if (_updating) {
        _pendingEntries.push_back(mstd::move(entry));
        return;
    }
    _entries.push_back(mstd::move(entry));
}

void ActionManager::mergePendingEntries() {
    if (_pendingEntries.empty()) {
        return;
    }

    for (auto& pending : _pendingEntries) {
        if (!pending.removed && pending.target && pending.action) {
            _entries.push_back(mstd::move(pending));
        } else {
            releaseEntry(pending);
        }
    }
    _pendingEntries.clear();
}

void ActionManager::compactEntries() {
    if (_compacting) {
        return;
    }

    _compacting = true;
    for (auto it = _entries.begin(); it != _entries.end();) {
        if (it->removed || !it->target || !it->action) {
            releaseEntry(*it);
            it = _entries.erase(it);
        } else {
            ++it;
        }
    }

    for (auto it = _pendingEntries.begin(); it != _pendingEntries.end();) {
        if (it->removed || !it->target || !it->action) {
            releaseEntry(*it);
            it = _pendingEntries.erase(it);
        } else {
            ++it;
        }
    }
    _compacting = false;
}

void ActionManager::releaseEntry(Entry& entry) {
    if (entry.action) {
        Action* action = entry.action;
        entry.action = nullptr;
        action->stop();
        action->release();
    }
    if (entry.target) {
        Node* target = entry.target;
        entry.target = nullptr;
        target->release();
    }
}

} // namespace zocos

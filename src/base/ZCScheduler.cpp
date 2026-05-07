#include "base/ZCScheduler.h"

#include "2d/ZCNode.h"

#include <algorithm>
#include <utility>

namespace zocos {

void Scheduler::schedule(Node* target, const std::string& key, Callback callback, float interval, int repeat,
                         float delay, int priority) {
    if (!target || key.empty() || !callback) {
        return;
    }

    const float safeInterval = interval < 0.f ? 0.f : interval;
    const float safeDelay = delay < 0.f ? 0.f : delay;
    const int safeRepeat = repeat < RepeatForever ? RepeatForever : repeat;

    auto refreshEntry = [&](Entry& entry) {
        entry.callback = std::move(callback);
        entry.interval = safeInterval;
        entry.delay = safeDelay;
        entry.elapsed = 0.f;
        entry.repeat = safeRepeat;
        entry.priority = priority;
        entry.timesExecuted = 0;
        entry.cancelled = false;
    };

    for (auto& entry : _entries) {
        if (entry.target == target && entry.key == key) {
            refreshEntry(entry);
            _dirtyOrder = true;
            return;
        }
    }

    for (auto& entry : _pendingEntries) {
        if (entry.target == target && entry.key == key) {
            refreshEntry(entry);
            return;
        }
    }

    Entry entry;
    entry.target = target;
    entry.key = key;
    entry.callback = std::move(callback);
    entry.interval = safeInterval;
    entry.delay = safeDelay;
    entry.elapsed = 0.f;
    entry.repeat = safeRepeat;
    entry.priority = priority;
    entry.order = _nextOrder++;
    entry.timesExecuted = 0;
    entry.cancelled = false;

    if (_updating) {
        _pendingEntries.push_back(std::move(entry));
        return;
    }

    _entries.push_back(std::move(entry));
    _dirtyOrder = true;
}

void Scheduler::scheduleOnce(Node* target, const std::string& key, Callback callback, float delay,
                             int priority) {
    schedule(target, key, std::move(callback), 0.f, 0, delay, priority);
}

void Scheduler::unschedule(Node* target, const std::string& key) {
    if (!target || key.empty()) {
        return;
    }

    if (_updating) {
        for (auto& entry : _entries) {
            if (entry.target == target && entry.key == key) {
                entry.cancelled = true;
            }
        }
        for (auto& entry : _pendingEntries) {
            if (entry.target == target && entry.key == key) {
                entry.cancelled = true;
            }
        }
        return;
    }

    _entries.erase(std::remove_if(_entries.begin(), _entries.end(),
                                  [target, &key](const Entry& entry) {
                                      return entry.target == target && entry.key == key;
                                  }),
                   _entries.end());
    _pendingEntries.erase(std::remove_if(_pendingEntries.begin(), _pendingEntries.end(),
                                         [target, &key](const Entry& entry) {
                                             return entry.target == target && entry.key == key;
                                         }),
                          _pendingEntries.end());
}

void Scheduler::unscheduleAllForTarget(Node* target) {
    if (!target) {
        return;
    }

    if (_updating) {
        for (auto& entry : _entries) {
            if (entry.target == target) {
                entry.cancelled = true;
            }
        }
        for (auto& entry : _pendingEntries) {
            if (entry.target == target) {
                entry.cancelled = true;
            }
        }
        return;
    }

    _entries.erase(std::remove_if(_entries.begin(), _entries.end(),
                                  [target](const Entry& entry) { return entry.target == target; }),
                   _entries.end());
    _pendingEntries.erase(std::remove_if(_pendingEntries.begin(), _pendingEntries.end(),
                                         [target](const Entry& entry) { return entry.target == target; }),
                          _pendingEntries.end());
}

void Scheduler::mergePendingEntries() {
    if (_pendingEntries.empty()) {
        return;
    }

    for (auto& entry : _pendingEntries) {
        if (!entry.cancelled) {
            _entries.push_back(std::move(entry));
            _dirtyOrder = true;
        }
    }
    _pendingEntries.clear();
}

void Scheduler::sortEntriesIfNeeded() {
    if (!_dirtyOrder) {
        return;
    }

    std::sort(_entries.begin(), _entries.end(), [](const Entry& a, const Entry& b) {
        if (a.priority != b.priority) {
            return a.priority < b.priority;
        }
        return a.order < b.order;
    });
    _dirtyOrder = false;
}

void Scheduler::update(float dt) {
    mergePendingEntries();

    if (_entries.empty()) {
        return;
    }

    sortEntriesIfNeeded();

    _updating = true;
    for (auto& entry : _entries) {
        if (entry.cancelled || !entry.callback || !entry.target || !entry.target->isRunning() ||
            entry.target->isPaused()) {
            continue;
        }

        bool shouldFire = false;
        if (entry.delay > 0.f) {
            entry.delay -= dt;
            if (entry.delay <= 0.f) {
                shouldFire = true;
                entry.elapsed = 0.f;
            }
        } else if (entry.interval <= 0.f) {
            shouldFire = true;
        } else {
            entry.elapsed += dt;
            if (entry.elapsed >= entry.interval) {
                entry.elapsed -= entry.interval;
                shouldFire = true;
            }
        }

        if (!shouldFire) {
            continue;
        }

        entry.callback(dt);
        if (entry.cancelled) {
            continue;
        }

        ++entry.timesExecuted;
        if (entry.repeat != RepeatForever && entry.timesExecuted > entry.repeat) {
            entry.cancelled = true;
        }
    }
    _updating = false;

    _entries.erase(std::remove_if(_entries.begin(), _entries.end(),
                                  [](const Entry& entry) {
                                      return entry.cancelled || !entry.target || !entry.target->isRunning();
                                  }),
                   _entries.end());

    mergePendingEntries();
    sortEntriesIfNeeded();
}

std::size_t Scheduler::getScheduledCount() const {
    std::size_t count = 0;
    for (const auto& entry : _entries) {
        if (!entry.cancelled && entry.target) {
            ++count;
        }
    }
    for (const auto& entry : _pendingEntries) {
        if (!entry.cancelled && entry.target) {
            ++count;
        }
    }
    return count;
}

} // namespace zocos

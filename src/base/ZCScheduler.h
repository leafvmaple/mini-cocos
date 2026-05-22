#pragma once

#include <cstddef>
#include "base/ZCStd.h"

namespace zocos {

class Node;

class Scheduler {
public:
    using Callback = mstd::function<void(float)>;
    static constexpr int RepeatForever = -1;

    void schedule(Node* target, const mstd::string& key, Callback callback, float interval = 0.f,
                  int repeat = RepeatForever, float delay = 0.f, int priority = 0);
    void scheduleOnce(Node* target, const mstd::string& key, Callback callback, float delay = 0.f,
                      int priority = 0);
    void unschedule(Node* target, const mstd::string& key);
    void unscheduleAllForTarget(Node* target);
    void update(float dt);

    mstd::size_t getScheduledCount() const;

private:
    struct Entry {
        Node* target = nullptr;
        mstd::string key;
        Callback callback;
        float interval = 0.f;
        float delay = 0.f;
        float elapsed = 0.f;
        int repeat = RepeatForever;
        int priority = 0;
        mstd::size_t order = 0;
        int timesExecuted = 0;
        bool cancelled = false;
    };

    void mergePendingEntries();
    void sortEntriesIfNeeded();

    bool _updating = false;
    bool _dirtyOrder = false;
    mstd::size_t _nextOrder = 0;
    mstd::vector<Entry> _entries;
    mstd::vector<Entry> _pendingEntries;
};

} // namespace zocos

#pragma once

#include <cstddef>
#include <functional>
#include <string>
#include <vector>

namespace zocos {

class Node;

class Scheduler {
public:
    using Callback = std::function<void(float)>;
    static constexpr int RepeatForever = -1;

    void schedule(Node* target, const std::string& key, Callback callback, float interval = 0.f,
                  int repeat = RepeatForever, float delay = 0.f, int priority = 0);
    void scheduleOnce(Node* target, const std::string& key, Callback callback, float delay = 0.f,
                      int priority = 0);
    void unschedule(Node* target, const std::string& key);
    void unscheduleAllForTarget(Node* target);
    void update(float dt);

    std::size_t getScheduledCount() const;

private:
    struct Entry {
        Node* target = nullptr;
        std::string key;
        Callback callback;
        float interval = 0.f;
        float delay = 0.f;
        float elapsed = 0.f;
        int repeat = RepeatForever;
        int priority = 0;
        std::size_t order = 0;
        int timesExecuted = 0;
        bool cancelled = false;
    };

    void mergePendingEntries();
    void sortEntriesIfNeeded();

    bool _updating = false;
    bool _dirtyOrder = false;
    std::size_t _nextOrder = 0;
    std::vector<Entry> _entries;
    std::vector<Entry> _pendingEntries;
};

} // namespace zocos

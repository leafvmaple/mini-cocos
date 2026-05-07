#pragma once

#include "base/ZCRef.h"

#include <cstddef>
#include <string>
#include <vector>

namespace zocos {

class PoolManager;

class AutoreleasePool {
public:
    explicit AutoreleasePool(std::string name = "autorelease pool");
    ~AutoreleasePool();

    void addObject(Ref* object);
    void clear();

    const std::string& getName() const { return _name; }
    std::size_t size() const { return _managedObjects.size(); }

private:
    friend class PoolManager;

    AutoreleasePool(std::string name, bool managedByPoolManager);

    std::string _name;
    bool _managedByPoolManager = true;
    std::vector<Ref*> _managedObjects;
};

class PoolManager {
public:
    static PoolManager& getInstance();

    AutoreleasePool* getCurrentPool() const;
    void clearRootPool();

    void push(AutoreleasePool* pool);
    void pop(AutoreleasePool* pool);

private:
    PoolManager();
    ~PoolManager();

    AutoreleasePool _rootPool;
    std::vector<AutoreleasePool*> _poolStack;
};

} // namespace zocos

#pragma once

#include "base/ZCRef.h"

#include <cstddef>
#include "base/ZCStd.h"

namespace zocos {

class PoolManager;

class AutoreleasePool {
public:
    explicit AutoreleasePool(mstd::string name = "autorelease pool");
    ~AutoreleasePool();

    void addObject(Ref* object);
    void clear();

    const mstd::string& getName() const { return _name; }
    mstd::size_t size() const { return _managedObjects.size(); }

private:
    friend class PoolManager;

    AutoreleasePool(mstd::string name, bool managedByPoolManager);

    mstd::string _name;
    bool _managedByPoolManager = true;
    mstd::vector<Ref*> _managedObjects;
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
    mstd::vector<AutoreleasePool*> _poolStack;
};

} // namespace zocos

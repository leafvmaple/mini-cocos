#include "base/ZCAutoreleasePool.h"

#include <cassert>
#include "base/ZCStd.h"

namespace zocos {

AutoreleasePool::AutoreleasePool(mstd::string name)
    : AutoreleasePool(mstd::move(name), true) {}

AutoreleasePool::AutoreleasePool(mstd::string name, bool managedByPoolManager)
    : _name(mstd::move(name)), _managedByPoolManager(managedByPoolManager) {
    if (_managedByPoolManager) {
        PoolManager::getInstance().push(this);
    }
}

AutoreleasePool::~AutoreleasePool() {
    clear();
    if (_managedByPoolManager) {
        PoolManager::getInstance().pop(this);
    }
}

void AutoreleasePool::addObject(Ref* object) {
    if (!object) {
        return;
    }
    _managedObjects.push_back(object);
}

void AutoreleasePool::clear() {
    auto releasing = mstd::move(_managedObjects);
    _managedObjects.clear();
    for (auto* object : releasing) {
        object->release();
    }
}

PoolManager& PoolManager::getInstance() {
    static PoolManager instance;
    return instance;
}

PoolManager::PoolManager() : _rootPool("root autorelease pool", false) {
    _poolStack.push_back(&_rootPool);
}

PoolManager::~PoolManager() {
    while (_poolStack.size() > 1) {
        _poolStack.pop_back();
    }
    _rootPool.clear();
}

AutoreleasePool* PoolManager::getCurrentPool() const {
    if (_poolStack.empty()) {
        return nullptr;
    }
    return _poolStack.back();
}

void PoolManager::clearRootPool() {
    _rootPool.clear();
}

void PoolManager::push(AutoreleasePool* pool) {
    if (!pool) {
        return;
    }
    _poolStack.push_back(pool);
}

void PoolManager::pop(AutoreleasePool* pool) {
    if (_poolStack.empty() || _poolStack.back() != pool) {
        assert(false && "Autorelease pool stack corrupted.");
        return;
    }
    _poolStack.pop_back();
}

} // namespace zocos

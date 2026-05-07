#include "base/ZCRef.h"
#include "base/ZCAutoreleasePool.h"

#include <cassert>

namespace zocos {

void Ref::retain() {
    ++_referenceCount;
}

void Ref::release() {
    assert(_referenceCount > 0);
    --_referenceCount;
    if (_referenceCount == 0) {
        delete this;
    }
}

Ref* Ref::autorelease() {
    auto* pool = PoolManager::getInstance().getCurrentPool();
    assert(pool && "No autorelease pool available.");
    if (pool) {
        pool->addObject(this);
    }
    return this;
}

} // namespace zocos

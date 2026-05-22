#include "base/ZCRef.h"
#include "base/ZCAutoreleasePool.h"

#include <cassert>

namespace zocos {

mstd::size_t Ref::s_liveCount = 0;

Ref::Ref() {
    ++s_liveCount;
}

Ref::~Ref() {
    assert(s_liveCount > 0);
    --s_liveCount;
}

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

void Ref::autorelease() {
    auto* pool = PoolManager::getInstance().getCurrentPool();
    assert(pool && "No autorelease pool available.");
    if (pool) {
        pool->addObject(this);
    }
}

mstd::size_t Ref::getLiveCount() {
    return s_liveCount;
}

} // namespace zocos

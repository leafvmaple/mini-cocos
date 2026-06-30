#include "ZCTestFramework.h"

#include "base/ZCAutoreleasePool.h"
#include "base/ZCRef.h"
#include "base/ZCStd.h"

using namespace zocos;

namespace {
// Ref's constructor/destructor are protected; a trivial subclass exposes them.
struct Probe : public Ref {
    Probe() = default;
};
} // namespace

ZC_TEST(ref_retain_release_lifecycle) {
    const mstd::size_t before = Ref::getLiveCount();

    auto* p = new Probe();
    ZC_CHECK_EQ(Ref::getLiveCount(), before + 1);
    ZC_CHECK_EQ(p->getReferenceCount(), 1u);

    p->retain();
    ZC_CHECK_EQ(p->getReferenceCount(), 2u);

    p->release();
    ZC_CHECK_EQ(p->getReferenceCount(), 1u);
    ZC_CHECK_EQ(Ref::getLiveCount(), before + 1);

    p->release(); // refcount hits 0 -> delete
    ZC_CHECK_EQ(Ref::getLiveCount(), before);
}

ZC_TEST(ref_autorelease_pool_drains_on_scope_exit) {
    const mstd::size_t before = Ref::getLiveCount();
    {
        AutoreleasePool pool("test pool");
        auto* p = new Probe();
        p->autorelease();
        ZC_CHECK_EQ(pool.size(), static_cast<mstd::size_t>(1));
        ZC_CHECK_EQ(Ref::getLiveCount(), before + 1);
    }
    // Pool destructor releases the autoreleased object, which frees it.
    ZC_CHECK_EQ(Ref::getLiveCount(), before);
}

ZC_TEST(ref_retained_object_survives_pool) {
    const mstd::size_t before = Ref::getLiveCount();
    Probe* survivor = nullptr;
    {
        AutoreleasePool pool("test pool");
        survivor = new Probe();
        survivor->autorelease();
        survivor->retain(); // refcount 2: one for the pool, one for us
    }
    // Pool released its reference; ours keeps it alive.
    ZC_CHECK_EQ(Ref::getLiveCount(), before + 1);
    ZC_CHECK_EQ(survivor->getReferenceCount(), 1u);
    survivor->release();
    ZC_CHECK_EQ(Ref::getLiveCount(), before);
}

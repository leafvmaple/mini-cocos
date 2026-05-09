#pragma once

#include <cstddef>

namespace zocos {

class Ref {
public:
    void retain();
    void release();
    void autorelease();
    static std::size_t getLiveCount();

    unsigned int getReferenceCount() const { return _referenceCount; }

protected:
    Ref();
    virtual ~Ref();

private:
    static std::size_t s_liveCount;
    unsigned int _referenceCount = 1;
};

} // namespace zocos

#pragma once

namespace zocos {

class Ref {
public:
    void retain();
    void release();
    Ref* autorelease();

    unsigned int getReferenceCount() const { return _referenceCount; }

protected:
    Ref() = default;
    virtual ~Ref() = default;

private:
    unsigned int _referenceCount = 1;
};

} // namespace zocos

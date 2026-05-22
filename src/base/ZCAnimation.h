#pragma once

#include "base/ZCRef.h"
#include "math/ZCMath.h"

#include "base/ZCStd.h"

namespace zocos {

class Animation : public Ref {
public:
    static Animation* create(const mstd::vector<Rect>& frames, float delayPerFrame = 0.1f);

    const mstd::vector<Rect>& getFrames() const { return _frames; }
    float getDelayPerFrame() const { return _delayPerFrame; }
    float getDuration() const;

protected:
    Animation(const mstd::vector<Rect>& frames, float delayPerFrame);

private:
    mstd::vector<Rect> _frames;
    float _delayPerFrame = 0.1f;
};

} // namespace zocos

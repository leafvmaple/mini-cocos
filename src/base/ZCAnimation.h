#pragma once

#include "base/ZCRef.h"
#include "math/ZCMath.h"

#include <vector>

namespace zocos {

class Animation : public Ref {
public:
    static Animation* create(const std::vector<Rect>& frames, float delayPerFrame = 0.1f);

    const std::vector<Rect>& getFrames() const { return _frames; }
    float getDelayPerFrame() const { return _delayPerFrame; }
    float getDuration() const;

protected:
    Animation(const std::vector<Rect>& frames, float delayPerFrame);

private:
    std::vector<Rect> _frames;
    float _delayPerFrame = 0.1f;
};

} // namespace zocos

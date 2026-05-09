#include "base/ZCAnimation.h"

#include <algorithm>
#include <new>

namespace zocos {

Animation::Animation(const std::vector<Rect>& frames, float delayPerFrame)
    : _frames(frames), _delayPerFrame(std::max(0.f, delayPerFrame)) {
}

Animation* Animation::create(const std::vector<Rect>& frames, float delayPerFrame) {
    if (frames.empty()) {
        return nullptr;
    }

    auto* animation = new (std::nothrow) Animation(frames, delayPerFrame);
    if (animation) {
        animation->autorelease();
        return animation;
    }

    delete animation;
    return nullptr;
}

float Animation::getDuration() const {
    if (_frames.empty()) {
        return 0.f;
    }

    return _delayPerFrame * static_cast<float>(_frames.size());
}

} // namespace zocos

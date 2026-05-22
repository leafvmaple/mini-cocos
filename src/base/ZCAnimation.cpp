#include "base/ZCAnimation.h"

#include "base/ZCStd.h"

namespace zocos {

Animation::Animation(const mstd::vector<Rect>& frames, float delayPerFrame)
    : _frames(frames), _delayPerFrame(mstd::max(0.f, delayPerFrame)) {
}

Animation* Animation::create(const mstd::vector<Rect>& frames, float delayPerFrame) {
    if (frames.empty()) {
        return nullptr;
    }

    auto* animation = new (mstd::nothrow) Animation(frames, delayPerFrame);
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

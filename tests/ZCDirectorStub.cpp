#include "base/ZCDirector.h"

#include "base/ZCFontAtlasCache.h"
#include "base/ZCFontCache.h"
#include "base/ZCTextureCache.h"

namespace zocos {

// Node's lifecycle hooks use the Director's core managers. Tests need those
// managers but deliberately never initialize a window or render device.
Director& Director::getInstance() {
    static Director* director = new Director();
    return *director;
}

void Director::onViewResized(int, int) {}

bool Director::onViewKeyEvent(int, int, int, bool, bool) { return false; }

void Director::onViewMouseButtonEvent(int, int, bool, float, float) {}

void Director::onViewMouseMoveEvent(float, float, float, float) {}

void Director::onViewMouseScrollEvent(float, float, float, float) {}

// The headless Director never creates this cache. Its destructor is still
// referenced by the constructor's cleanup path generated for unique_ptr.
FontAtlasCache::~FontAtlasCache() = default;

} // namespace zocos

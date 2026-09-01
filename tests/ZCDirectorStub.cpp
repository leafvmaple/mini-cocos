#include "base/ZCDirector.h"

#include "base/ZCFontAtlasCache.h"
#include "base/ZCFontCache.h"
#include "base/ZCPlatformFactory.h"
#include "base/ZCTextureCache.h"

namespace zocos {

// Headless platform and cache implementations let the tests exercise the real
// Director scene-stack code without creating a window or render device.
mstd::unique_ptr<View> createDefaultView() { return {}; }

mstd::unique_ptr<RenderDevice> createDefaultRenderDevice(View&) { return {}; }

FontAtlasCache::FontAtlasCache(Director& director) : _director(director) {}
FontAtlasCache::~FontAtlasCache() = default;
void FontAtlasCache::removeAllFontAtlas() {}

void FontCache::removeAllFonts() {}

void TextureCache::removeAllTextures(Director&) {}

} // namespace zocos

#include "2d/ZCSprite.h"

#include "base/ZCDirector.h"
#include "base/ZCRenderer.h"
#include "base/ZCTextureCache.h"

#include "base/ZCStd.h"

namespace zocos {

namespace {

Rect sanitizeRect(const Rect& rect, const Size& textureSize) {
    Rect clamped = rect;

    if (clamped.width < 0.f) {
        clamped.x += clamped.width;
        clamped.width = -clamped.width;
    }
    if (clamped.height < 0.f) {
        clamped.y += clamped.height;
        clamped.height = -clamped.height;
    }

    clamped.x = mstd::clamp(clamped.x, 0.f, textureSize.width);
    clamped.y = mstd::clamp(clamped.y, 0.f, textureSize.height);

    const float maxX = mstd::clamp(clamped.x + clamped.width, 0.f, textureSize.width);
    const float maxY = mstd::clamp(clamped.y + clamped.height, 0.f, textureSize.height);
    clamped.width = mstd::max(0.f, maxX - clamped.x);
    clamped.height = mstd::max(0.f, maxY - clamped.y);
    return clamped;
}

Rect toUvRectTopLeft(const Rect& pixelRect, const Size& textureSize) {
    if (textureSize.width <= 0.f || textureSize.height <= 0.f) {
        return Rect{0.f, 0.f, 1.f, 1.f};
    }

    const Rect clamped = sanitizeRect(pixelRect, textureSize);
    const float invW = 1.f / textureSize.width;
    const float invH = 1.f / textureSize.height;

    Rect uv;
    uv.x = clamped.x * invW;
    uv.y = (textureSize.height - (clamped.y + clamped.height)) * invH;
    uv.width = clamped.width * invW;
    uv.height = clamped.height * invH;
    return uv;
}

} // namespace

Sprite::Sprite(Director& director) : _director(director) {
}

Sprite* Sprite::create(Director& director) {
    auto* sprite = new (mstd::nothrow) Sprite(director);
    if (sprite && sprite->init()) {
        sprite->autorelease();
        return sprite;
    }
    delete sprite;
    return nullptr;
}

Sprite* Sprite::createWithFile(Director& director, const char* path) {
    auto* sprite = create(director);
    if (!sprite) {
        return nullptr;
    }
    if (!sprite->initWithFile(path)) {
        return nullptr;
    }
    return sprite;
}

bool Sprite::init() {
    if (!Node::init()) {
        return false;
    }
    setContentSize({128.f, 128.f});
    return true;
}

Sprite::~Sprite() {
    releaseTexture();
}

bool Sprite::initWithFile(const char* path) {
    TextureHandle texture;
    Size textureSize;
    if (!_director.getTextureCache().acquireFromFile(_director, path, texture, textureSize)) {
        return false;
    }

    releaseTexture();
    _texture = texture;
    _texturePixelSize = textureSize;
    _textureRect = {0.f, 0.f, textureSize.width, textureSize.height};
    _hasTextureRect = true;
    setContentSize(textureSize);
    _ready = true;
    return true;
}

void Sprite::initWithCheckerboard() {
    TextureHandle texture;
    Size textureSize;
    if (!_director.getTextureCache().acquireCheckerboard(_director, 64, texture, textureSize)) {
        _ready = false;
        return;
    }

    releaseTexture();
    _texture = texture;
    _texturePixelSize = textureSize;
    _textureRect = {0.f, 0.f, textureSize.width, textureSize.height};
    _hasTextureRect = true;
    setContentSize(textureSize);
    _ready = true;
}

void Sprite::setTextureRect(const Rect& rect, bool resetSize) {
    if (!_texture.isValid() || _texturePixelSize.width <= 0.f || _texturePixelSize.height <= 0.f) {
        return;
    }

    _textureRect = sanitizeRect(rect, _texturePixelSize);
    _hasTextureRect = true;
    if (resetSize) {
        setContentSize({_textureRect.width, _textureRect.height});
    }
}

void Sprite::releaseTexture() {
    if (_texture.isValid()) {
        _director.getTextureCache().release(_director, _texture);
        _texture = {};
    }

    _texturePixelSize = {};
    _textureRect = {};
    _hasTextureRect = false;
    _ready = false;
}

void Sprite::draw(Renderer& renderer, const Mat4& world) {
    if (!_ready || !_texture.isValid()) {
        return;
    }

    Rect uvRect{0.f, 0.f, 1.f, 1.f};
    if (_hasTextureRect) {
        uvRect = toUvRectTopLeft(_textureRect, _texturePixelSize);
    }

    const RenderSortKey sortKey = makeRenderSortKey(0, 0, _texture.value);
    renderer.addDrawSprite(world, _contentSize, _texture, uvRect, getOpacity(), sortKey);
}

} // namespace zocos

#pragma once

#include "base/ZCRenderCommand.h"
#include "2d/ZCNode.h"

namespace zocos {

class Director;
class Renderer;

class Sprite : public Node {
public:
    static Sprite* create(Director& director);
    static Sprite* createWithFile(Director& director, const char* path);

    ~Sprite() override;

    bool init() override;

    bool initWithFile(const char* path);
    void initWithCheckerboard();
    void setTextureRect(const Rect& rect, bool resetSize = true);
    const Rect& getTextureRect() const { return _textureRect; }
    const Size& getTextureSize() const { return _texturePixelSize; }

    void draw(Renderer& renderer, const Mat4& world) override;

protected:
    explicit Sprite(Director& director);

private:
    void releaseTexture();

    Director& _director;
    TextureHandle _texture{};
    Size _texturePixelSize{};
    Rect _textureRect{};
    bool _hasTextureRect = false;
    bool _ready = false;
};

} // namespace zocos

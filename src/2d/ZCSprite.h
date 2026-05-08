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

    void draw(Renderer& renderer, const Mat4& world) override;

protected:
    explicit Sprite(Director& director);

private:
    Director& _director;
    TextureHandle _texture{};
    bool _ready = false;
};

} // namespace zocos

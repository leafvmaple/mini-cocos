#pragma once

#include "2d/ZCNode.h"
#include "platform/opengl_loader.h"

namespace zocos {

class Director;

class Sprite : public Node {
public:
    static Sprite* create(Director& director);
    static Sprite* createWithFile(Director& director, const char* path);

    ~Sprite() override;

    bool init() override;

    bool initWithFile(const char* path);
    void initWithCheckerboard();

    void draw(const Mat4& world) override;

protected:
    explicit Sprite(Director& director);

private:
    Director& _director;
    GLuint _texture = 0;
    GLuint _vao = 0;
    GLuint _vbo = 0;
    bool _ready = false;
};

} // namespace zocos

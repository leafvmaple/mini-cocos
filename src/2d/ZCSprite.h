#pragma once

#include "2d/ZCNode.h"
#include "platform/opengl_loader.h"

namespace zocos {

class ZCDirector;

class ZCSprite : public ZCNode {
public:
    explicit ZCSprite(ZCDirector& director);
    ~ZCSprite() override;

    bool initWithFile(const char* path);
    void initWithCheckerboard();

    void draw(const ZCMat4& world) override;

private:
    ZCDirector& _director;
    GLuint _texture = 0;
    GLuint _vao = 0;
    GLuint _vbo = 0;
    bool _ready = false;
};

} // namespace zocos

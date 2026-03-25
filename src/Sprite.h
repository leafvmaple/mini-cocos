#pragma once

#include "Node.h"
#include "opengl_loader.h"

namespace mini {

class Director;

class Sprite : public Node {
public:
  explicit Sprite(Director& director);
  ~Sprite() override;

  bool initWithFile(const char* path);
  void initWithCheckerboard();

  void draw(const Mat4& world) override;

private:
  Director& _director;
  GLuint _texture = 0;
  GLuint _vao = 0;
  GLuint _vbo = 0;
  bool _ready = false;
};

} // namespace mini

#include "Sprite.h"
#include "Director.h"

#define STB_IMAGE_IMPLEMENTATION
#include <stb_image.h>

#include <cstdio>
#include <vector>

namespace mini {

namespace {

const char* kVs = R"(#version 330 core
layout (location = 0) in vec2 aPos;
layout (location = 1) in vec2 aUv;
uniform mat4 uMvp;
out vec2 vUv;
void main() {
  vUv = aUv;
  gl_Position = uMvp * vec4(aPos, 0.0, 1.0);
}
)";

const char* kFs = R"(#version 330 core
in vec2 vUv;
uniform sampler2D uTex;
out vec4 FragColor;
void main() {
  FragColor = texture(uTex, vUv);
}
)";

GLuint compileShader(GLenum type, const char* src) {
  const GLuint s = glCreateShader(type);
  glShaderSource(s, 1, &src, nullptr);
  glCompileShader(s);
  GLint ok = 0;
  glGetShaderiv(s, GL_COMPILE_STATUS, &ok);
  if (!ok) {
    char buf[512];
    glGetShaderInfoLog(s, sizeof(buf), nullptr, buf);
    std::fprintf(stderr, "Shader compile error: %s\n", buf);
    glDeleteShader(s);
    return 0;
  }
  return s;
}

GLuint linkProgram(GLuint vs, GLuint fs) {
  const GLuint p = glCreateProgram();
  glAttachShader(p, vs);
  glAttachShader(p, fs);
  glLinkProgram(p);
  GLint ok = 0;
  glGetProgramiv(p, GL_LINK_STATUS, &ok);
  if (!ok) {
    char buf[512];
    glGetProgramInfoLog(p, sizeof(buf), nullptr, buf);
    std::fprintf(stderr, "Program link error: %s\n", buf);
    glDeleteProgram(p);
    return 0;
  }
  return p;
}

} // namespace

Sprite::Sprite(Director& director) : _director(director) {
  setContentSize({128.f, 128.f});
}

Sprite::~Sprite() {
  if (_vbo) glDeleteBuffers(1, &_vbo);
  if (_vao) glDeleteVertexArrays(1, &_vao);
  if (_texture) glDeleteTextures(1, &_texture);
}

bool Sprite::initWithFile(const char* path) {
  int w = 0, h = 0, ch = 0;
  unsigned char* data = stbi_load(path, &w, &h, &ch, 4);
  if (!data) {
    std::fprintf(stderr, "stbi_load failed: %s\n", path);
    return false;
  }
  if (_texture) glDeleteTextures(1, &_texture);
  glGenTextures(1, &_texture);
  glBindTexture(GL_TEXTURE_2D, _texture);
  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
  glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, w, h, 0, GL_RGBA, GL_UNSIGNED_BYTE, data);
  stbi_image_free(data);
  setContentSize({static_cast<float>(w), static_cast<float>(h)});
  _ready = true;
  return true;
}

void Sprite::initWithCheckerboard() {
  constexpr int N = 64;
  std::vector<unsigned char> px(static_cast<size_t>(N * N * 4));
  for (int y = 0; y < N; ++y) {
    for (int x = 0; x < N; ++x) {
      const bool c = ((x / 8) + (y / 8)) % 2 == 0;
      const unsigned char v = c ? 240 : 60;
      size_t i = static_cast<size_t>((y * N + x) * 4);
      px[i + 0] = v;
      px[i + 1] = static_cast<unsigned char>(255 - v);
      px[i + 2] = 160;
      px[i + 3] = 255;
    }
  }
  if (_texture) glDeleteTextures(1, &_texture);
  glGenTextures(1, &_texture);
  glBindTexture(GL_TEXTURE_2D, _texture);
  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
  glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, N, N, 0, GL_RGBA, GL_UNSIGNED_BYTE, px.data());
  setContentSize({static_cast<float>(N), static_cast<float>(N)});
  _ready = true;
}

void Sprite::draw(const Mat4& world) {
  if (!_ready) return;

  static GLuint s_program = 0;
  static GLint s_locMvp = -1;
  static GLint s_locTex = -1;
  if (!s_program) {
    const GLuint vs = compileShader(GL_VERTEX_SHADER, kVs);
    const GLuint fs = compileShader(GL_FRAGMENT_SHADER, kFs);
    if (!vs || !fs) return;
    s_program = linkProgram(vs, fs);
    glDeleteShader(vs);
    glDeleteShader(fs);
    if (!s_program) return;
    s_locMvp = glGetUniformLocation(s_program, "uMvp");
    s_locTex = glGetUniformLocation(s_program, "uTex");
  }

  const float w = _contentSize.width;
  const float h = _contentSize.height;
  const float verts[] = {0.f, 0.f, 0.f, 0.f, w, 0.f, 1.f, 0.f, w, h, 1.f, 1.f,
                         0.f, 0.f, 0.f, 0.f, w, h, 1.f, 1.f, 0.f, h, 0.f, 1.f};

  if (!_vao) {
    glGenVertexArrays(1, &_vao);
    glGenBuffers(1, &_vbo);
  }
  glBindVertexArray(_vao);
  glBindBuffer(GL_ARRAY_BUFFER, _vbo);
  glBufferData(GL_ARRAY_BUFFER, sizeof(verts), verts, GL_STATIC_DRAW);
  glEnableVertexAttribArray(0);
  glVertexAttribPointer(0, 2, GL_FLOAT, GL_FALSE, 4 * sizeof(float), reinterpret_cast<void*>(0));
  glEnableVertexAttribArray(1);
  glVertexAttribPointer(1, 2, GL_FLOAT, GL_FALSE, 4 * sizeof(float), reinterpret_cast<void*>(8));

  const Mat4 mvp = _director.projectionMatrix() * world;
  glUseProgram(s_program);
  glUniformMatrix4fv(s_locMvp, 1, GL_TRUE, mvp.data());
  glActiveTexture(GL_TEXTURE0);
  glBindTexture(GL_TEXTURE_2D, _texture);
  glUniform1i(s_locTex, 0);
  glDrawArrays(GL_TRIANGLES, 0, 6);
  glBindVertexArray(0);
}

} // namespace mini

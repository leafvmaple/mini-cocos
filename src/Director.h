#pragma once

#include "MiniMath.h"
#include "Node.h"
#include <memory>

struct GLFWwindow;

namespace mini {

class Director {
public:
  static Director& getInstance();

  bool init(int width, int height, const char* title);
  void shutdown();

  void runWithScene(std::unique_ptr<Scene> scene);
  void mainLoop();

  const Mat4& projectionMatrix() const { return _projection; }
  GLFWwindow* getWindow() const { return _window; }

  int getFramebufferWidth() const { return _fbWidth; }
  int getFramebufferHeight() const { return _fbHeight; }

  void onFramebufferResize(int w, int h);

private:
  Director() = default;

  void updateProjection();

  GLFWwindow* _window = nullptr;
  std::unique_ptr<Scene> _runningScene;
  Mat4 _projection = Mat4::identity();
  int _fbWidth = 0;
  int _fbHeight = 0;
  double _lastTime = 0.0;
};

} // namespace mini

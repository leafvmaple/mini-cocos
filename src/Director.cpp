#include "Director.h"
#include "opengl_loader.h"

#define GLFW_INCLUDE_NONE
#include <GLFW/glfw3.h>

namespace mini {

static void framebufferSizeCallback(GLFWwindow* win, int w, int h) {
  auto* self = static_cast<Director*>(glfwGetWindowUserPointer(win));
  if (self) self->onFramebufferResize(w, h);
}

void Director::onFramebufferResize(int w, int h) {
  _fbWidth = w;
  _fbHeight = h;
  glViewport(0, 0, w, h);
  updateProjection();
}

Director& Director::getInstance() {
  static Director inst;
  return inst;
}

bool Director::init(int width, int height, const char* title) {
  if (!glfwInit()) return false;
  glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
  glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 3);
  glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);
#ifdef __APPLE__
  glfwWindowHint(GLFW_OPENGL_FORWARD_COMPAT, GL_TRUE);
#endif
  _window = glfwCreateWindow(width, height, title, nullptr, nullptr);
  if (!_window) {
    glfwTerminate();
    return false;
  }
  glfwMakeContextCurrent(_window);
  glfwSetWindowUserPointer(_window, this);
  glfwSetFramebufferSizeCallback(_window, framebufferSizeCallback);

  if (!loadOpenGL(reinterpret_cast<void* (*)(const char*)>(glfwGetProcAddress))) {
    shutdown();
    return false;
  }

  glfwGetFramebufferSize(_window, &_fbWidth, &_fbHeight);
  glViewport(0, 0, _fbWidth, _fbHeight);
  glEnable(GL_BLEND);
  glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
  updateProjection();
  _lastTime = glfwGetTime();
  return true;
}

void Director::shutdown() {
  _runningScene.reset();
  if (_window) {
    glfwDestroyWindow(_window);
    _window = nullptr;
  }
  glfwTerminate();
}

void Director::updateProjection() {
  if (_fbWidth <= 0 || _fbHeight <= 0) return;
  _projection = Mat4::ortho(0.f, static_cast<float>(_fbWidth), 0.f, static_cast<float>(_fbHeight), -1.f, 1.f);
}

void Director::runWithScene(std::unique_ptr<Scene> scene) {
  _runningScene = std::move(scene);
}

void Director::mainLoop() {
  while (_window && !glfwWindowShouldClose(_window)) {
    const double now = glfwGetTime();
    const float dt = static_cast<float>(now - _lastTime);
    _lastTime = now;

    glfwPollEvents();
    if (_runningScene) _runningScene->updateTree(dt);

    glClearColor(0.12f, 0.12f, 0.15f, 1.f);
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

    if (_runningScene) _runningScene->visitScene();

    glfwSwapBuffers(_window);
  }
}

} // namespace mini

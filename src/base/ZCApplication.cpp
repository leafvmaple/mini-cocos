#include "base/ZCApplication.h"
#include "base/ZCDirector.h"

#define GLFW_INCLUDE_NONE
#include <GLFW/glfw3.h>

namespace zocos {

int Application::run() {
    Director& director = Director::getInstance();
    while (director.getWindow() && !glfwWindowShouldClose(director.getWindow())) {
        director.mainLoop();
    }
    return 0;
}

} // namespace zocos
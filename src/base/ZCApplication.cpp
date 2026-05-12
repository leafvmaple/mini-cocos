#include "base/ZCApplication.h"
#include "base/ZCDirector.h"

namespace zocos {

int Application::run() {
    Director& director = Director::getInstance();
    while (director.mainLoop()) {
    }
    return 0;
}

} // namespace zocos
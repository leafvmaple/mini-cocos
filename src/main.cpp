#include "base/ZCApplication.h"
#include "base/ZCDirector.h"
#include "scripting/ZCLuaEngine.h"
#include "base/ZCRef.h"

#include <cassert>
#include <cstdio>

using namespace zocos;

int main(int argc, char** argv) {
    LuaEngine& luaEngine = LuaEngine::getInstance();
    if (!luaEngine.init()) {
        std::fprintf(stderr, "LuaEngine::init failed\n");
        return 1;
    }

    if (!luaEngine.executeScriptFile("scripts/main.lua", argc, argv)) {
        std::fprintf(stderr, "Failed to execute scripts/main.lua\n");
        Director::getInstance().shutdown();
        luaEngine.shutdown();
        return 1;
    }

    Application app;
    const int exitCode = app.run();

    Director::getInstance().shutdown();
    luaEngine.shutdown();

    assert(Ref::getLiveCount() == 0 && "Ref-managed objects leaked.");

    return exitCode;
}

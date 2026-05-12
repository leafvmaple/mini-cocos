#include "scripting/ZCLuaEngine.h"

#include "platform/ZCFileUtils.h"
#include "scripting/ZCLuaManual.h"

extern "C" {
#include <lauxlib.h>
#include <lua.h>
#include <lualib.h>
}

#include <cstdio>
#include <string>
#include <vector>

namespace zocos {

namespace {

void setScriptArgs(lua_State* state, const char* scriptPath, int argc, char** argv) {
    lua_newtable(state);

    if (scriptPath) {
        lua_pushstring(state, scriptPath);
        lua_seti(state, -2, 0);
    }

    for (int i = 1; i < argc; ++i) {
        const char* value = (argv && argv[i]) ? argv[i] : "";
        lua_pushstring(state, value);
        lua_seti(state, -2, i);
    }

    lua_setglobal(state, "arg");
}

void appendPackagePath(lua_State* state, const std::string& pattern) {
    if (pattern.empty()) {
        return;
    }

    lua_getglobal(state, "package");
    if (!lua_istable(state, -1)) {
        lua_pop(state, 1);
        return;
    }

    lua_getfield(state, -1, "path");
    const char* currentPath = lua_tostring(state, -1);

    std::string mergedPath = currentPath ? currentPath : "";
    if (mergedPath.find(pattern) == std::string::npos) {
        if (!mergedPath.empty()) {
            mergedPath.push_back(';');
        }
        mergedPath += pattern;
    }

    lua_pop(state, 1);
    lua_pushlstring(state, mergedPath.c_str(), mergedPath.size());
    lua_setfield(state, -2, "path");
    lua_pop(state, 1);
}

} // namespace

LuaEngine& LuaEngine::getInstance() {
    static LuaEngine engine;
    return engine;
}

bool LuaEngine::init() {
    if (_state) {
        return true;
    }

    _state = luaL_newstate();
    if (!_state) {
        return false;
    }

    luaL_openlibs(_state);
    register_all_zocos(_state);
    return true;
}

void LuaEngine::shutdown() {
    if (_state) {
        lua_close(_state);
        _state = nullptr;
    }
}

bool LuaEngine::executeScriptFile(const char* scriptPath, int argc, char** argv) {
    if (!_state || !scriptPath || !*scriptPath) {
        return false;
    }

    const std::string scriptDir = FileUtils::getInstance().parentPath(scriptPath);
    if (!scriptDir.empty()) {
        appendPackagePath(_state, scriptDir + "/?.lua");
    }

    setScriptArgs(_state, scriptPath, argc, argv);

    std::vector<unsigned char> chunkBytes;
    if (!FileUtils::getInstance().getDataFromFile(scriptPath, chunkBytes)) {
        std::fprintf(stderr, "Lua read error: %s\n", scriptPath);
        return false;
    }

    if (luaL_loadbufferx(_state, reinterpret_cast<const char*>(chunkBytes.data()),
                         chunkBytes.size(), scriptPath, nullptr)
        != LUA_OK) {
        const char* message = lua_tostring(_state, -1);
        std::fprintf(stderr, "Lua load error: %s\n", message ? message : "(unknown)");
        lua_pop(_state, 1);
        return false;
    }

    if (lua_pcall(_state, 0, 0, 0) != LUA_OK) {
        const char* message = lua_tostring(_state, -1);
        std::fprintf(stderr, "Lua runtime error: %s\n", message ? message : "(unknown)");
        lua_pop(_state, 1);
        return false;
    }

    return true;
}

} // namespace zocos

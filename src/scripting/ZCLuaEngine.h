#pragma once

struct lua_State;

namespace zocos {

class LuaEngine {
public:
    static LuaEngine& getInstance();

    bool init();
    void shutdown();

    bool executeScriptFile(const char* scriptPath, int argc = 0, char** argv = nullptr);

    lua_State* getLuaState() const { return _state; }

private:
    LuaEngine() = default;

    lua_State* _state = nullptr;
};

} // namespace zocos

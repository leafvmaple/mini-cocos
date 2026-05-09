#include "scripting/ZCLuaManual.h"

#include "base/ZCAction.h"
#include "base/ZCDirector.h"
#include "2d/ZCLabel.h"
#include "2d/ZCNode.h"
#include "2d/ZCScene.h"
#include "2d/ZCSprite.h"

extern "C" {
#include <lauxlib.h>
#include <lua.h>
}

#include <cstdio>
#include <string>
#include <unordered_map>
#include <vector>

namespace zocos {

namespace {

constexpr const char* kDirectorMeta = "zocos.Director";
constexpr const char* kNodeMeta = "zocos.Node";
constexpr const char* kActionMeta = "zocos.Action";

struct LuaDirectorRef {
    Director* director = nullptr;
};

struct LuaNodeRef {
    Node* node = nullptr;
};

struct LuaActionRef {
    Action* action = nullptr;
};

using LuaNodeScheduleRefs = std::unordered_map<Node*, std::unordered_map<std::string, int>>;

LuaNodeScheduleRefs& getLuaNodeScheduleRefs() {
    static LuaNodeScheduleRefs refs;
    return refs;
}

int findLuaScheduleRef(Node* node, const std::string& key) {
    auto& refs = getLuaNodeScheduleRefs();
    const auto nodeIt = refs.find(node);
    if (nodeIt == refs.end()) {
        return LUA_NOREF;
    }

    const auto keyIt = nodeIt->second.find(key);
    if (keyIt == nodeIt->second.end()) {
        return LUA_NOREF;
    }

    return keyIt->second;
}

void setLuaScheduleRef(lua_State* tolua_S, Node* node, const std::string& key, int ref) {
    auto& refs = getLuaNodeScheduleRefs();
    auto& perNodeRefs = refs[node];

    const auto it = perNodeRefs.find(key);
    if (it != perNodeRefs.end()) {
        luaL_unref(tolua_S, LUA_REGISTRYINDEX, it->second);
    }

    perNodeRefs[key] = ref;
}

void clearLuaScheduleRef(lua_State* tolua_S, Node* node, const std::string& key,
                         int expectedRef = LUA_NOREF) {
    auto& refs = getLuaNodeScheduleRefs();
    const auto nodeIt = refs.find(node);
    if (nodeIt == refs.end()) {
        return;
    }

    const auto keyIt = nodeIt->second.find(key);
    if (keyIt == nodeIt->second.end()) {
        return;
    }

    if (expectedRef != LUA_NOREF && keyIt->second != expectedRef) {
        return;
    }

    luaL_unref(tolua_S, LUA_REGISTRYINDEX, keyIt->second);
    nodeIt->second.erase(keyIt);
    if (nodeIt->second.empty()) {
        refs.erase(nodeIt);
    }
}

void clearAllLuaScheduleRefs(lua_State* tolua_S, Node* node) {
    auto& refs = getLuaNodeScheduleRefs();
    const auto nodeIt = refs.find(node);
    if (nodeIt == refs.end()) {
        return;
    }

    for (const auto& kv : nodeIt->second) {
        luaL_unref(tolua_S, LUA_REGISTRYINDEX, kv.second);
    }

    refs.erase(nodeIt);
}

int classArgBase(lua_State* tolua_S) {
    if (lua_gettop(tolua_S) >= 1 && lua_istable(tolua_S, 1)) {
        return 2;
    }
    return 1;
}

Director* checkDirector(lua_State* tolua_S, int index) {
    auto* ref = static_cast<LuaDirectorRef*>(luaL_checkudata(tolua_S, index, kDirectorMeta));
    luaL_argcheck(tolua_S, ref != nullptr && ref->director != nullptr, index, "Director expected");
    return ref->director;
}

Node* checkNode(lua_State* tolua_S, int index) {
    auto* ref = static_cast<LuaNodeRef*>(luaL_checkudata(tolua_S, index, kNodeMeta));
    luaL_argcheck(tolua_S, ref != nullptr && ref->node != nullptr, index, "Node expected");
    return ref->node;
}

Action* checkAction(lua_State* tolua_S, int index) {
    auto* ref = static_cast<LuaActionRef*>(luaL_checkudata(tolua_S, index, kActionMeta));
    luaL_argcheck(tolua_S, ref != nullptr && ref->action != nullptr, index, "Action expected");
    return ref->action;
}

void pushDirector(lua_State* tolua_S, Director* director) {
    auto* ref = static_cast<LuaDirectorRef*>(lua_newuserdata(tolua_S, sizeof(LuaDirectorRef)));
    ref->director = director;
    luaL_getmetatable(tolua_S, kDirectorMeta);
    lua_setmetatable(tolua_S, -2);
}

void pushNode(lua_State* tolua_S, Node* node) {
    if (!node) {
        lua_pushnil(tolua_S);
        return;
    }

    auto* ref = static_cast<LuaNodeRef*>(lua_newuserdata(tolua_S, sizeof(LuaNodeRef)));
    ref->node = node;
    luaL_getmetatable(tolua_S, kNodeMeta);
    lua_setmetatable(tolua_S, -2);
}

void pushAction(lua_State* tolua_S, Action* action) {
    if (!action) {
        lua_pushnil(tolua_S);
        return;
    }

    auto* ref = static_cast<LuaActionRef*>(lua_newuserdata(tolua_S, sizeof(LuaActionRef)));
    ref->action = action;
    luaL_getmetatable(tolua_S, kActionMeta);
    lua_setmetatable(tolua_S, -2);
}

std::vector<Action*> checkActionArray(lua_State* tolua_S, int index) {
    const int absIndex = lua_absindex(tolua_S, index);
    luaL_checktype(tolua_S, absIndex, LUA_TTABLE);

    const lua_Integer length = luaL_len(tolua_S, absIndex);
    std::vector<Action*> actions;
    actions.reserve(static_cast<std::size_t>(length));

    for (lua_Integer i = 1; i <= length; ++i) {
        lua_geti(tolua_S, absIndex, i);
        actions.push_back(checkAction(tolua_S, -1));
        lua_pop(tolua_S, 1);
    }

    return actions;
}

int lua_zocos_Director_getInstance(lua_State* tolua_S) {
    pushDirector(tolua_S, &Director::getInstance());
    return 1;
}

int lua_zocos_Director_init(lua_State* tolua_S) {
    Director* director = checkDirector(tolua_S, 1);
    const int width = static_cast<int>(luaL_checkinteger(tolua_S, 2));
    const int height = static_cast<int>(luaL_checkinteger(tolua_S, 3));
    const char* title = luaL_checkstring(tolua_S, 4);
    lua_pushboolean(tolua_S, director->init(width, height, title));
    return 1;
}

int lua_zocos_Director_runWithScene(lua_State* tolua_S) {
    Director* director = checkDirector(tolua_S, 1);
    Node* node = checkNode(tolua_S, 2);
    auto* scene = dynamic_cast<Scene*>(node);
    if (!scene) {
        return luaL_error(tolua_S, "Director:runWithScene expects a Scene");
    }

    director->runWithScene(scene);
    return 0;
}

int lua_zocos_Director_shutdown(lua_State* tolua_S) {
    Director* director = checkDirector(tolua_S, 1);
    director->shutdown();
    return 0;
}

int lua_zocos_Director_getFramebufferWidth(lua_State* tolua_S) {
    Director* director = checkDirector(tolua_S, 1);
    lua_pushinteger(tolua_S, director->getFramebufferWidth());
    return 1;
}

int lua_zocos_Director_getFramebufferHeight(lua_State* tolua_S) {
    Director* director = checkDirector(tolua_S, 1);
    lua_pushinteger(tolua_S, director->getFramebufferHeight());
    return 1;
}

int lua_zocos_Scene_create(lua_State* tolua_S) {
    (void)tolua_S;
    pushNode(tolua_S, Scene::create());
    return 1;
}

int lua_zocos_Sprite_create(lua_State* tolua_S) {
    (void)tolua_S;
    pushNode(tolua_S, Sprite::create(Director::getInstance()));
    return 1;
}

int lua_zocos_Sprite_createWithFile(lua_State* tolua_S) {
    const int base = classArgBase(tolua_S);
    const char* path = luaL_checkstring(tolua_S, base);
    pushNode(tolua_S, Sprite::createWithFile(Director::getInstance(), path));
    return 1;
}

int lua_zocos_Label_create(lua_State* tolua_S) {
    const int base = classArgBase(tolua_S);
    const char* text = luaL_optstring(tolua_S, base, "");
    pushNode(tolua_S, Label::create(Director::getInstance(), text));
    return 1;
}

int lua_zocos_MoveTo_create(lua_State* tolua_S) {
    const int base = classArgBase(tolua_S);
    const float duration = static_cast<float>(luaL_checknumber(tolua_S, base + 0));
    const float x = static_cast<float>(luaL_checknumber(tolua_S, base + 1));
    const float y = static_cast<float>(luaL_checknumber(tolua_S, base + 2));
    pushAction(tolua_S, MoveTo::create(duration, Vec2{x, y}));
    return 1;
}

int lua_zocos_MoveBy_create(lua_State* tolua_S) {
    const int base = classArgBase(tolua_S);
    const float duration = static_cast<float>(luaL_checknumber(tolua_S, base + 0));
    const float x = static_cast<float>(luaL_checknumber(tolua_S, base + 1));
    const float y = static_cast<float>(luaL_checknumber(tolua_S, base + 2));
    pushAction(tolua_S, MoveBy::create(duration, Vec2{x, y}));
    return 1;
}

int lua_zocos_RotateBy_create(lua_State* tolua_S) {
    const int base = classArgBase(tolua_S);
    const float duration = static_cast<float>(luaL_checknumber(tolua_S, base + 0));
    const float delta = static_cast<float>(luaL_checknumber(tolua_S, base + 1));
    pushAction(tolua_S, RotateBy::create(duration, delta));
    return 1;
}

int lua_zocos_RotateTo_create(lua_State* tolua_S) {
    const int base = classArgBase(tolua_S);
    const float duration = static_cast<float>(luaL_checknumber(tolua_S, base + 0));
    const float value = static_cast<float>(luaL_checknumber(tolua_S, base + 1));
    pushAction(tolua_S, RotateTo::create(duration, value));
    return 1;
}

int lua_zocos_ScaleTo_create(lua_State* tolua_S) {
    const int base = classArgBase(tolua_S);
    const float duration = static_cast<float>(luaL_checknumber(tolua_S, base + 0));
    const float sx = static_cast<float>(luaL_checknumber(tolua_S, base + 1));
    const float sy = static_cast<float>(luaL_optnumber(tolua_S, base + 2, sx));
    pushAction(tolua_S, ScaleTo::create(duration, Vec2{sx, sy}));
    return 1;
}

int lua_zocos_FadeTo_create(lua_State* tolua_S) {
    const int base = classArgBase(tolua_S);
    const float duration = static_cast<float>(luaL_checknumber(tolua_S, base + 0));
    const float opacity = static_cast<float>(luaL_checknumber(tolua_S, base + 1));
    pushAction(tolua_S, FadeTo::create(duration, opacity));
    return 1;
}

int lua_zocos_DelayTime_create(lua_State* tolua_S) {
    const int base = classArgBase(tolua_S);
    const float duration = static_cast<float>(luaL_checknumber(tolua_S, base));
    pushAction(tolua_S, DelayTime::create(duration));
    return 1;
}

int lua_zocos_Sequence_create(lua_State* tolua_S) {
    const int base = classArgBase(tolua_S);
    pushAction(tolua_S, Sequence::create(checkActionArray(tolua_S, base)));
    return 1;
}

int lua_zocos_Spawn_create(lua_State* tolua_S) {
    const int base = classArgBase(tolua_S);
    pushAction(tolua_S, Spawn::create(checkActionArray(tolua_S, base)));
    return 1;
}

int lua_zocos_Repeat_create(lua_State* tolua_S) {
    const int base = classArgBase(tolua_S);
    Action* inner = checkAction(tolua_S, base + 0);
    const int times = static_cast<int>(luaL_checkinteger(tolua_S, base + 1));
    pushAction(tolua_S, Repeat::create(inner, times));
    return 1;
}

int lua_zocos_RepeatForever_create(lua_State* tolua_S) {
    const int base = classArgBase(tolua_S);
    Action* inner = checkAction(tolua_S, base);
    pushAction(tolua_S, RepeatForever::create(inner));
    return 1;
}

int lua_zocos_Node_setPosition(lua_State* tolua_S) {
    Node* node = checkNode(tolua_S, 1);
    const float x = static_cast<float>(luaL_checknumber(tolua_S, 2));
    const float y = static_cast<float>(luaL_checknumber(tolua_S, 3));
    node->setPosition(x, y);
    return 0;
}

int lua_zocos_Node_setScale(lua_State* tolua_S) {
    Node* node = checkNode(tolua_S, 1);
    const float sx = static_cast<float>(luaL_checknumber(tolua_S, 2));
    const float sy = static_cast<float>(luaL_optnumber(tolua_S, 3, sx));
    node->setScale(Vec2{sx, sy});
    return 0;
}

int lua_zocos_Node_setRotation(lua_State* tolua_S) {
    Node* node = checkNode(tolua_S, 1);
    const float degrees = static_cast<float>(luaL_checknumber(tolua_S, 2));
    node->setRotation(degrees);
    return 0;
}

int lua_zocos_Node_setAnchorPoint(lua_State* tolua_S) {
    Node* node = checkNode(tolua_S, 1);
    const float x = static_cast<float>(luaL_checknumber(tolua_S, 2));
    const float y = static_cast<float>(luaL_checknumber(tolua_S, 3));
    node->setAnchorPoint(Vec2{x, y});
    return 0;
}

int lua_zocos_Node_setOpacity(lua_State* tolua_S) {
    Node* node = checkNode(tolua_S, 1);
    const float opacity = static_cast<float>(luaL_checknumber(tolua_S, 2));
    node->setOpacity(opacity);
    return 0;
}

int lua_zocos_Node_addChild(lua_State* tolua_S) {
    Node* node = checkNode(tolua_S, 1);
    Node* child = checkNode(tolua_S, 2);
    node->addChild(child);
    return 0;
}

int lua_zocos_Node_runAction(lua_State* tolua_S) {
    Node* node = checkNode(tolua_S, 1);
    Action* action = checkAction(tolua_S, 2);
    pushAction(tolua_S, node->runAction(action));
    return 1;
}

int lua_zocos_Node_schedule(lua_State* tolua_S) {
    Node* node = checkNode(tolua_S, 1);
    const char* key = luaL_checkstring(tolua_S, 2);
    luaL_argcheck(tolua_S, key != nullptr && key[0] != '\0', 2, "non-empty key expected");
    luaL_checktype(tolua_S, 3, LUA_TFUNCTION);

    const float interval = static_cast<float>(luaL_optnumber(tolua_S, 4, 0.0));
    const int repeat = static_cast<int>(luaL_optinteger(tolua_S, 5, Node::RepeatForever));
    const float delay = static_cast<float>(luaL_optnumber(tolua_S, 6, 0.0));
    const int priority = static_cast<int>(luaL_optinteger(tolua_S, 7, 0));

    lua_pushvalue(tolua_S, 3);
    const int callbackRef = luaL_ref(tolua_S, LUA_REGISTRYINDEX);
    const std::string keyString(key);
    setLuaScheduleRef(tolua_S, node, keyString, callbackRef);

    node->schedule(keyString, [tolua_S, node, keyString](float dt) {
        const int ref = findLuaScheduleRef(node, keyString);
        if (ref == LUA_NOREF) {
            return;
        }

        lua_rawgeti(tolua_S, LUA_REGISTRYINDEX, ref);
        lua_pushnumber(tolua_S, static_cast<lua_Number>(dt));
        if (lua_pcall(tolua_S, 1, 0, 0) != LUA_OK) {
            const char* message = lua_tostring(tolua_S, -1);
            std::fprintf(stderr, "Lua schedule callback error (%s): %s\n", keyString.c_str(),
                         message ? message : "(unknown)");
            lua_pop(tolua_S, 1);
        }
    }, interval, repeat, delay, priority);
    return 0;
}

int lua_zocos_Node_scheduleOnce(lua_State* tolua_S) {
    Node* node = checkNode(tolua_S, 1);
    const char* key = luaL_checkstring(tolua_S, 2);
    luaL_argcheck(tolua_S, key != nullptr && key[0] != '\0', 2, "non-empty key expected");
    luaL_checktype(tolua_S, 3, LUA_TFUNCTION);

    const float delay = static_cast<float>(luaL_optnumber(tolua_S, 4, 0.0));
    const int priority = static_cast<int>(luaL_optinteger(tolua_S, 5, 0));

    lua_pushvalue(tolua_S, 3);
    const int callbackRef = luaL_ref(tolua_S, LUA_REGISTRYINDEX);
    const std::string keyString(key);
    setLuaScheduleRef(tolua_S, node, keyString, callbackRef);

    node->scheduleOnce(keyString, [tolua_S, node, keyString, callbackRef](float dt) {
        const int ref = findLuaScheduleRef(node, keyString);
        if (ref == LUA_NOREF) {
            return;
        }

        lua_rawgeti(tolua_S, LUA_REGISTRYINDEX, ref);
        lua_pushnumber(tolua_S, static_cast<lua_Number>(dt));
        if (lua_pcall(tolua_S, 1, 0, 0) != LUA_OK) {
            const char* message = lua_tostring(tolua_S, -1);
            std::fprintf(stderr, "Lua scheduleOnce callback error (%s): %s\n", keyString.c_str(),
                         message ? message : "(unknown)");
            lua_pop(tolua_S, 1);
        }

        clearLuaScheduleRef(tolua_S, node, keyString, callbackRef);
    }, delay, priority);
    return 0;
}

int lua_zocos_Node_unschedule(lua_State* tolua_S) {
    Node* node = checkNode(tolua_S, 1);
    const char* key = luaL_checkstring(tolua_S, 2);
    const std::string keyString = key ? key : "";
    node->unschedule(keyString);
    clearLuaScheduleRef(tolua_S, node, keyString);
    return 0;
}

int lua_zocos_Node_unscheduleAllCallbacks(lua_State* tolua_S) {
    Node* node = checkNode(tolua_S, 1);
    node->unscheduleAllCallbacks();
    clearAllLuaScheduleRefs(tolua_S, node);
    return 0;
}

int lua_zocos_Node_initWithFile(lua_State* tolua_S) {
    auto* sprite = dynamic_cast<Sprite*>(checkNode(tolua_S, 1));
    if (!sprite) {
        return luaL_error(tolua_S, "initWithFile is only valid for Sprite");
    }

    const char* path = luaL_checkstring(tolua_S, 2);
    lua_pushboolean(tolua_S, sprite->initWithFile(path));
    return 1;
}

int lua_zocos_Node_initWithCheckerboard(lua_State* tolua_S) {
    auto* sprite = dynamic_cast<Sprite*>(checkNode(tolua_S, 1));
    if (!sprite) {
        return luaL_error(tolua_S, "initWithCheckerboard is only valid for Sprite");
    }

    sprite->initWithCheckerboard();
    return 0;
}

int lua_zocos_Node_setString(lua_State* tolua_S) {
    auto* label = dynamic_cast<Label*>(checkNode(tolua_S, 1));
    if (!label) {
        return luaL_error(tolua_S, "setString is only valid for Label");
    }

    const char* text = luaL_checkstring(tolua_S, 2);
    label->setString(text);
    return 0;
}

int lua_zocos_Action_setTag(lua_State* tolua_S) {
    Action* action = checkAction(tolua_S, 1);
    const int tag = static_cast<int>(luaL_checkinteger(tolua_S, 2));
    action->setTag(tag);
    return 0;
}

int lua_zocos_Action_getTag(lua_State* tolua_S) {
    Action* action = checkAction(tolua_S, 1);
    lua_pushinteger(tolua_S, action->getTag());
    return 1;
}

void registerMetatable(lua_State* tolua_S, const char* metatableName, const luaL_Reg* methods) {
    luaL_newmetatable(tolua_S, metatableName);
    lua_pushvalue(tolua_S, -1);
    lua_setfield(tolua_S, -2, "__index");
    luaL_setfuncs(tolua_S, methods, 0);
    lua_pop(tolua_S, 1);
}

void tolua_beginmodule(lua_State* tolua_S, const char* moduleName) {
    if (!moduleName || moduleName[0] == '\0') {
        return;
    }

    lua_getfield(tolua_S, -1, moduleName);
    if (!lua_istable(tolua_S, -1)) {
        lua_pop(tolua_S, 1);
        lua_newtable(tolua_S);
        lua_pushvalue(tolua_S, -1);
        lua_setfield(tolua_S, -3, moduleName);
    }
}

void tolua_function(lua_State* tolua_S, const char* funcName, lua_CFunction func) {
    lua_pushcfunction(tolua_S, func);
    lua_setfield(tolua_S, -2, funcName);
}

void tolua_endmodule(lua_State* tolua_S) {
    lua_pop(tolua_S, 1);
}

void stashCurrentModuleInRegistry(lua_State* tolua_S, const char* moduleName) {
    lua_pushstring(tolua_S, moduleName);
    lua_pushvalue(tolua_S, -2);
    lua_rawset(tolua_S, LUA_REGISTRYINDEX);
}

} // namespace

int register_all_zocos_manual(lua_State* tolua_S) {
    static const luaL_Reg directorMethods[] = {
        {"init", lua_zocos_Director_init},
        {"runWithScene", lua_zocos_Director_runWithScene},
        {"shutdown", lua_zocos_Director_shutdown},
        {"getFramebufferWidth", lua_zocos_Director_getFramebufferWidth},
        {"getFramebufferHeight", lua_zocos_Director_getFramebufferHeight},
        {nullptr, nullptr},
    };

    static const luaL_Reg nodeMethods[] = {
        {"setPosition", lua_zocos_Node_setPosition},
        {"setScale", lua_zocos_Node_setScale},
        {"setRotation", lua_zocos_Node_setRotation},
        {"setAnchorPoint", lua_zocos_Node_setAnchorPoint},
        {"setOpacity", lua_zocos_Node_setOpacity},
        {"addChild", lua_zocos_Node_addChild},
        {"runAction", lua_zocos_Node_runAction},
        {"schedule", lua_zocos_Node_schedule},
        {"scheduleOnce", lua_zocos_Node_scheduleOnce},
        {"unschedule", lua_zocos_Node_unschedule},
        {"unscheduleAllCallbacks", lua_zocos_Node_unscheduleAllCallbacks},
        {"initWithFile", lua_zocos_Node_initWithFile},
        {"initWithCheckerboard", lua_zocos_Node_initWithCheckerboard},
        {"setString", lua_zocos_Node_setString},
        {nullptr, nullptr},
    };

    static const luaL_Reg actionMethods[] = {
        {"setTag", lua_zocos_Action_setTag},
        {"getTag", lua_zocos_Action_getTag},
        {nullptr, nullptr},
    };

    registerMetatable(tolua_S, kDirectorMeta, directorMethods);
    registerMetatable(tolua_S, kNodeMeta, nodeMethods);
    registerMetatable(tolua_S, kActionMeta, actionMethods);

    lua_newtable(tolua_S);

    tolua_beginmodule(tolua_S, "Director");
    stashCurrentModuleInRegistry(tolua_S, "Director");
    tolua_function(tolua_S, "getInstance", lua_zocos_Director_getInstance);
    tolua_endmodule(tolua_S);

    tolua_beginmodule(tolua_S, "Scene");
    stashCurrentModuleInRegistry(tolua_S, "Scene");
    tolua_function(tolua_S, "create", lua_zocos_Scene_create);
    tolua_endmodule(tolua_S);

    tolua_beginmodule(tolua_S, "Sprite");
    stashCurrentModuleInRegistry(tolua_S, "Sprite");
    tolua_function(tolua_S, "create", lua_zocos_Sprite_create);
    tolua_endmodule(tolua_S);

    tolua_beginmodule(tolua_S, "Label");
    stashCurrentModuleInRegistry(tolua_S, "Label");
    tolua_function(tolua_S, "create", lua_zocos_Label_create);
    tolua_endmodule(tolua_S);

    tolua_beginmodule(tolua_S, "MoveTo");
    stashCurrentModuleInRegistry(tolua_S, "MoveTo");
    tolua_function(tolua_S, "create", lua_zocos_MoveTo_create);
    tolua_endmodule(tolua_S);

    tolua_beginmodule(tolua_S, "MoveBy");
    stashCurrentModuleInRegistry(tolua_S, "MoveBy");
    tolua_function(tolua_S, "create", lua_zocos_MoveBy_create);
    tolua_endmodule(tolua_S);

    tolua_beginmodule(tolua_S, "RotateBy");
    stashCurrentModuleInRegistry(tolua_S, "RotateBy");
    tolua_function(tolua_S, "create", lua_zocos_RotateBy_create);
    tolua_endmodule(tolua_S);

    tolua_beginmodule(tolua_S, "RotateTo");
    stashCurrentModuleInRegistry(tolua_S, "RotateTo");
    tolua_function(tolua_S, "create", lua_zocos_RotateTo_create);
    tolua_endmodule(tolua_S);

    tolua_beginmodule(tolua_S, "ScaleTo");
    stashCurrentModuleInRegistry(tolua_S, "ScaleTo");
    tolua_function(tolua_S, "create", lua_zocos_ScaleTo_create);
    tolua_endmodule(tolua_S);

    tolua_beginmodule(tolua_S, "FadeTo");
    stashCurrentModuleInRegistry(tolua_S, "FadeTo");
    tolua_function(tolua_S, "create", lua_zocos_FadeTo_create);
    tolua_endmodule(tolua_S);

    tolua_beginmodule(tolua_S, "DelayTime");
    stashCurrentModuleInRegistry(tolua_S, "DelayTime");
    tolua_function(tolua_S, "create", lua_zocos_DelayTime_create);
    tolua_endmodule(tolua_S);

    tolua_beginmodule(tolua_S, "Sequence");
    stashCurrentModuleInRegistry(tolua_S, "Sequence");
    tolua_function(tolua_S, "create", lua_zocos_Sequence_create);
    tolua_endmodule(tolua_S);

    tolua_beginmodule(tolua_S, "Spawn");
    stashCurrentModuleInRegistry(tolua_S, "Spawn");
    tolua_function(tolua_S, "create", lua_zocos_Spawn_create);
    tolua_endmodule(tolua_S);

    tolua_beginmodule(tolua_S, "Repeat");
    stashCurrentModuleInRegistry(tolua_S, "Repeat");
    tolua_function(tolua_S, "create", lua_zocos_Repeat_create);
    tolua_endmodule(tolua_S);

    tolua_beginmodule(tolua_S, "RepeatForever");
    stashCurrentModuleInRegistry(tolua_S, "RepeatForever");
    tolua_function(tolua_S, "create", lua_zocos_RepeatForever_create);
    tolua_endmodule(tolua_S);

    lua_pushstring(tolua_S, "zocos-lua");
    lua_setfield(tolua_S, -2, "VERSION");

    lua_pushvalue(tolua_S, -1);
    lua_setglobal(tolua_S, "zocos");
    lua_pushvalue(tolua_S, -1);
    lua_setglobal(tolua_S, "cc");

    lua_pop(tolua_S, 1);
    return 1;
}

int register_all_zocos_manual_deprecated(lua_State* tolua_S) {
    lua_pushstring(tolua_S, "Sprite");
    lua_rawget(tolua_S, LUA_REGISTRYINDEX);
    if (lua_istable(tolua_S, -1)) {
        lua_pushstring(tolua_S, "createWithFile");
        lua_pushcfunction(tolua_S, lua_zocos_Sprite_createWithFile);
        lua_rawset(tolua_S, -3);
    }
    lua_pop(tolua_S, 1);

    lua_pushstring(tolua_S, "Sequence");
    lua_rawget(tolua_S, LUA_REGISTRYINDEX);
    if (lua_istable(tolua_S, -1)) {
        lua_pushstring(tolua_S, "createWithActions");
        lua_pushcfunction(tolua_S, lua_zocos_Sequence_create);
        lua_rawset(tolua_S, -3);
    }
    lua_pop(tolua_S, 1);

    lua_pushstring(tolua_S, "Spawn");
    lua_rawget(tolua_S, LUA_REGISTRYINDEX);
    if (lua_istable(tolua_S, -1)) {
        lua_pushstring(tolua_S, "createWithActions");
        lua_pushcfunction(tolua_S, lua_zocos_Spawn_create);
        lua_rawset(tolua_S, -3);
    }
    lua_pop(tolua_S, 1);

    return 1;
}

int register_all_zocos(lua_State* tolua_S) {
    register_all_zocos_manual(tolua_S);
    register_all_zocos_manual_deprecated(tolua_S);
    return 1;
}

} // namespace zocos



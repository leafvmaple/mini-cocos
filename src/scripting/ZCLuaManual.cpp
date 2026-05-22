#include "scripting/ZCLuaManual.h"

#include "base/ZCAction.h"
#include "base/ZCActionInterval.h"
#include "base/ZCDirector.h"
#include "base/ZCFontAtlas.h"
#include "2d/ZCLabel.h"
#include "2d/ZCNode.h"
#include "2d/ZCScene.h"
#include "2d/ZCSprite.h"
#include "ui/UIButton.h"
#include "ui/UIWidget.h"

extern "C" {
#include <lauxlib.h>
#include <lua.h>
}

#include <cstdio>
#include <cstring>
#include "base/ZCStd.h"

namespace zocos {

namespace {

constexpr const char* kDirectorMeta = "zocos.Director";
constexpr const char* kNodeMeta = "zocos.Node";
constexpr const char* kSceneMeta = "zocos.Scene";
constexpr const char* kSpriteMeta = "zocos.Sprite";
constexpr const char* kLabelMeta = "zocos.Label";
constexpr const char* kWidgetMeta = "zocos.Widget";
constexpr const char* kButtonMeta = "zocos.Button";
constexpr const char* kActionMeta = "zocos.Action";
constexpr const char* kAnimationMeta = "zocos.Animation";
constexpr const char* kFontAtlasMeta = "zocos.FontAtlas";

bool hasMetatable(lua_State* tolua_S, int index, const char* metatableName) {
    const int absIndex = lua_absindex(tolua_S, index);
    if (!lua_getmetatable(tolua_S, absIndex)) {
        return false;
    }

    luaL_getmetatable(tolua_S, metatableName);
    const bool same = lua_rawequal(tolua_S, -1, -2) != 0;
    lua_pop(tolua_S, 2);
    return same;
}

bool isCompatibleMetatable(lua_State* tolua_S, int index, const char* expectedMetatable) {
    if (hasMetatable(tolua_S, index, expectedMetatable)) {
        return true;
    }

    if (std::strcmp(expectedMetatable, kNodeMeta) == 0) {
        return hasMetatable(tolua_S, index, kSceneMeta) ||
               hasMetatable(tolua_S, index, kSpriteMeta) ||
               hasMetatable(tolua_S, index, kLabelMeta) ||
               hasMetatable(tolua_S, index, kWidgetMeta) ||
               hasMetatable(tolua_S, index, kButtonMeta);
    }

    if (std::strcmp(expectedMetatable, kWidgetMeta) == 0) {
        return hasMetatable(tolua_S, index, kButtonMeta);
    }

    return false;
}

const char* nodeMetatableForObject(Node* node) {
    if (!node) {
        return kNodeMeta;
    }

    if (dynamic_cast<ui::Button*>(node)) {
        return kButtonMeta;
    }
    if (dynamic_cast<ui::Widget*>(node)) {
        return kWidgetMeta;
    }
    if (dynamic_cast<Label*>(node)) {
        return kLabelMeta;
    }
    if (dynamic_cast<Sprite*>(node)) {
        return kSpriteMeta;
    }
    if (dynamic_cast<Scene*>(node)) {
        return kSceneMeta;
    }

    return kNodeMeta;
}

template <typename T>
T* luaval_to_object(lua_State* tolua_S, int index, const char* metatableName,
                    const char* expectedMessage) {
    const int absIndex = lua_absindex(tolua_S, index);
    luaL_checktype(tolua_S, absIndex, LUA_TUSERDATA);
    luaL_argcheck(tolua_S, isCompatibleMetatable(tolua_S, absIndex, metatableName), absIndex,
                  expectedMessage);

    auto* userData = static_cast<T**>(lua_touserdata(tolua_S, absIndex));
    luaL_argcheck(tolua_S, userData != nullptr && *userData != nullptr, index, expectedMessage);
    return *userData;
}

template <typename T>
void object_to_luaval(lua_State* tolua_S, const char* metatableName, T* object) {
    if (!object) {
        lua_pushnil(tolua_S);
        return;
    }

    auto* userData = static_cast<T**>(lua_newuserdata(tolua_S, sizeof(T*)));
    *userData = object;
    luaL_getmetatable(tolua_S, metatableName);
    lua_setmetatable(tolua_S, -2);
}

void node_to_luaval(lua_State* tolua_S, Node* node) {
    object_to_luaval(tolua_S, nodeMetatableForObject(node), node);
}

using LuaNodeScheduleRefs = mstd::unordered_map<Node*, mstd::unordered_map<mstd::string, int>>;
using LuaWidgetEventRefs = mstd::unordered_map<ui::Widget*, int>;

LuaNodeScheduleRefs& getLuaNodeScheduleRefs() {
    static LuaNodeScheduleRefs refs;
    return refs;
}

LuaWidgetEventRefs& getLuaWidgetEventRefs() {
    static LuaWidgetEventRefs refs;
    return refs;
}

int findLuaScheduleRef(Node* node, const mstd::string& key) {
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

void setLuaScheduleRef(lua_State* tolua_S, Node* node, const mstd::string& key, int ref) {
    auto& refs = getLuaNodeScheduleRefs();
    auto& perNodeRefs = refs[node];

    const auto it = perNodeRefs.find(key);
    if (it != perNodeRefs.end()) {
        luaL_unref(tolua_S, LUA_REGISTRYINDEX, it->second);
    }

    perNodeRefs[key] = ref;
}

void clearLuaScheduleRef(lua_State* tolua_S, Node* node, const mstd::string& key,
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

int findLuaWidgetEventRef(ui::Widget* widget) {
    auto& refs = getLuaWidgetEventRefs();
    const auto it = refs.find(widget);
    if (it == refs.end()) {
        return LUA_NOREF;
    }
    return it->second;
}

void setLuaWidgetEventRef(lua_State* tolua_S, ui::Widget* widget, int ref) {
    if (!widget) {
        luaL_unref(tolua_S, LUA_REGISTRYINDEX, ref);
        return;
    }

    auto& refs = getLuaWidgetEventRefs();
    const auto it = refs.find(widget);
    if (it != refs.end()) {
        luaL_unref(tolua_S, LUA_REGISTRYINDEX, it->second);
        it->second = ref;
    } else {
        refs.emplace(widget, ref);
    }
}

void clearLuaWidgetEventRef(lua_State* tolua_S, ui::Widget* widget) {
    if (!widget) {
        return;
    }

    auto& refs = getLuaWidgetEventRefs();
    const auto it = refs.find(widget);
    if (it == refs.end()) {
        return;
    }

    luaL_unref(tolua_S, LUA_REGISTRYINDEX, it->second);
    refs.erase(it);
}

int classArgBase(lua_State* tolua_S) {
    if (lua_gettop(tolua_S) >= 1 && lua_istable(tolua_S, 1)) {
        return 2;
    }
    return 1;
}

int classArgCount(lua_State* tolua_S) { return lua_gettop(tolua_S) - classArgBase(tolua_S) + 1; }

int reportWrongArgCount(lua_State* tolua_S, const char* funcName, int argc, int expected) {
    return luaL_error(tolua_S, "%s has wrong number of arguments: %d, was expecting %d", funcName,
                      argc, expected);
}

int reportWrongArgCount(lua_State* tolua_S, const char* funcName, int argc, int minExpected,
                        int maxExpected) {
    return luaL_error(tolua_S, "%s has wrong number of arguments: %d, was expecting %d to %d",
                      funcName, argc, minExpected, maxExpected);
}

template <typename T>
mstd::vector<T*> luaval_to_object_array(lua_State* tolua_S, int index, const char* metatableName,
                                       const char* expectedMessage) {
    const int absIndex = lua_absindex(tolua_S, index);
    luaL_checktype(tolua_S, absIndex, LUA_TTABLE);

    const lua_Integer length = luaL_len(tolua_S, absIndex);
    mstd::vector<T*> objects;
    objects.reserve(static_cast<mstd::size_t>(length));

    for (lua_Integer i = 1; i <= length; ++i) {
        lua_geti(tolua_S, absIndex, i);
        objects.push_back(luaval_to_object<T>(tolua_S, -1, metatableName, expectedMessage));
        lua_pop(tolua_S, 1);
    }

    return objects;
}

Rect luaval_to_rect(lua_State* tolua_S, int index, const char* expectedMessage) {
    const int absIndex = lua_absindex(tolua_S, index);
    luaL_checktype(tolua_S, absIndex, LUA_TTABLE);

    lua_geti(tolua_S, absIndex, 1);
    const float x = static_cast<float>(luaL_checknumber(tolua_S, -1));
    lua_pop(tolua_S, 1);

    lua_geti(tolua_S, absIndex, 2);
    const float y = static_cast<float>(luaL_checknumber(tolua_S, -1));
    lua_pop(tolua_S, 1);

    lua_geti(tolua_S, absIndex, 3);
    const float width = static_cast<float>(luaL_checknumber(tolua_S, -1));
    lua_pop(tolua_S, 1);

    lua_geti(tolua_S, absIndex, 4);
    const float height = static_cast<float>(luaL_checknumber(tolua_S, -1));
    lua_pop(tolua_S, 1);

    luaL_argcheck(tolua_S, width >= 0.f && height >= 0.f, absIndex, expectedMessage);
    return Rect{x, y, width, height};
}

mstd::vector<Rect> luaval_to_rect_array(lua_State* tolua_S, int index, const char* expectedMessage) {
    const int absIndex = lua_absindex(tolua_S, index);
    luaL_checktype(tolua_S, absIndex, LUA_TTABLE);

    const lua_Integer length = luaL_len(tolua_S, absIndex);
    mstd::vector<Rect> rects;
    rects.reserve(static_cast<mstd::size_t>(length));

    for (lua_Integer i = 1; i <= length; ++i) {
        lua_geti(tolua_S, absIndex, i);
        rects.push_back(luaval_to_rect(tolua_S, -1, expectedMessage));
        lua_pop(tolua_S, 1);
    }

    return rects;
}

int lua_zocos_Director_getInstance(lua_State* tolua_S) {
    const int argc = classArgCount(tolua_S);
    if (argc == 0) {
        object_to_luaval(tolua_S, kDirectorMeta, &Director::getInstance());
        return 1;
    }

    return reportWrongArgCount(tolua_S, "cc.Director:getInstance", argc, 0);
}

int lua_zocos_Director_init(lua_State* tolua_S) {
    Director* cobj = luaval_to_object<Director>(tolua_S, 1, kDirectorMeta, "Director expected");
    const int argc = lua_gettop(tolua_S) - 1;
    if (argc == 3) {
        const int width = static_cast<int>(luaL_checkinteger(tolua_S, 2));
        const int height = static_cast<int>(luaL_checkinteger(tolua_S, 3));
        const char* title = luaL_checkstring(tolua_S, 4);
        lua_pushboolean(tolua_S, cobj->init(width, height, title));
        return 1;
    }

    return reportWrongArgCount(tolua_S, "cc.Director:init", argc, 3);
}

int lua_zocos_Director_runWithScene(lua_State* tolua_S) {
    Director* cobj = luaval_to_object<Director>(tolua_S, 1, kDirectorMeta, "Director expected");
    const int argc = lua_gettop(tolua_S) - 1;
    if (argc == 1) {
        Node* node = luaval_to_object<Node>(tolua_S, 2, kNodeMeta, "Node expected");
        auto* scene = dynamic_cast<Scene*>(node);
        if (!scene) {
            return luaL_error(tolua_S, "cc.Director:runWithScene expects Scene as first argument");
        }

        cobj->runWithScene(scene);
        return 0;
    }

    return reportWrongArgCount(tolua_S, "cc.Director:runWithScene", argc, 1);
}

int lua_zocos_Director_shutdown(lua_State* tolua_S) {
    Director* cobj = luaval_to_object<Director>(tolua_S, 1, kDirectorMeta, "Director expected");
    const int argc = lua_gettop(tolua_S) - 1;
    if (argc == 0) {
        cobj->shutdown();
        return 0;
    }

    return reportWrongArgCount(tolua_S, "cc.Director:shutdown", argc, 0);
}

int lua_zocos_Director_getFramebufferWidth(lua_State* tolua_S) {
    Director* cobj = luaval_to_object<Director>(tolua_S, 1, kDirectorMeta, "Director expected");
    const int argc = lua_gettop(tolua_S) - 1;
    if (argc == 0) {
        lua_pushinteger(tolua_S, cobj->getFramebufferWidth());
        return 1;
    }

    return reportWrongArgCount(tolua_S, "cc.Director:getFramebufferWidth", argc, 0);
}

int lua_zocos_Director_getFramebufferHeight(lua_State* tolua_S) {
    Director* cobj = luaval_to_object<Director>(tolua_S, 1, kDirectorMeta, "Director expected");
    const int argc = lua_gettop(tolua_S) - 1;
    if (argc == 0) {
        lua_pushinteger(tolua_S, cobj->getFramebufferHeight());
        return 1;
    }

    return reportWrongArgCount(tolua_S, "cc.Director:getFramebufferHeight", argc, 0);
}

int lua_zocos_Scene_create(lua_State* tolua_S) {
    const int argc = classArgCount(tolua_S);
    if (argc == 0) {
        node_to_luaval(tolua_S, Scene::create());
        return 1;
    }

    return reportWrongArgCount(tolua_S, "cc.Scene:create", argc, 0);
}

int lua_zocos_Sprite_create(lua_State* tolua_S) {
    const int argc = classArgCount(tolua_S);
    if (argc == 0) {
        node_to_luaval(tolua_S, Sprite::create(Director::getInstance()));
        return 1;
    }

    return reportWrongArgCount(tolua_S, "cc.Sprite:create", argc, 0);
}

int lua_zocos_Sprite_createWithFile(lua_State* tolua_S) {
    const int base = classArgBase(tolua_S);
    const int argc = classArgCount(tolua_S);
    if (argc == 1) {
        const char* path = luaL_checkstring(tolua_S, base);
        node_to_luaval(tolua_S, Sprite::createWithFile(Director::getInstance(), path));
        return 1;
    }

    return reportWrongArgCount(tolua_S, "cc.Sprite:createWithFile", argc, 1);
}

int lua_zocos_Label_create(lua_State* tolua_S) {
    const int base = classArgBase(tolua_S);
    const int argc = classArgCount(tolua_S);
    if (argc >= 0 && argc <= 3) {
        const char* text = luaL_optstring(tolua_S, base, "");
        const char* fontPath = argc >= 2 ? luaL_checkstring(tolua_S, base + 1) : "";
        const float fontSize =
            argc >= 3 ? static_cast<float>(luaL_checknumber(tolua_S, base + 2)) : 24.f;
        node_to_luaval(tolua_S,
                       Label::createWithTTF(Director::getInstance(), text, fontPath, fontSize));
        return 1;
    }

    return reportWrongArgCount(tolua_S, "cc.Label:create", argc, 0, 3);
}

int lua_zocos_Label_createWithTTF(lua_State* tolua_S) {
    const int base = classArgBase(tolua_S);
    const int argc = classArgCount(tolua_S);
    if (argc == 2 || argc == 3) {
        const char* text = luaL_checkstring(tolua_S, base);
        const char* fontPath = luaL_checkstring(tolua_S, base + 1);
        const float fontSize =
            argc >= 3 ? static_cast<float>(luaL_checknumber(tolua_S, base + 2)) : 24.f;
        node_to_luaval(tolua_S,
                       Label::createWithTTF(Director::getInstance(), text, fontPath, fontSize));
        return 1;
    }

    return reportWrongArgCount(tolua_S, "cc.Label:createWithTTF", argc, 2, 3);
}

int lua_zocos_Button_create(lua_State* tolua_S) {
    const int base = classArgBase(tolua_S);
    const int argc = classArgCount(tolua_S);
    if (argc >= 0 && argc <= 3) {
        const char* text = luaL_optstring(tolua_S, base, "");
        const char* fontPath = argc >= 2 ? luaL_checkstring(tolua_S, base + 1) : "";
        const float fontSize =
            argc >= 3 ? static_cast<float>(luaL_checknumber(tolua_S, base + 2)) : 24.f;

        auto* button = ui::Button::create(Director::getInstance(), text);
        if (button && argc >= 2) {
            button->setTitleFontName(fontPath);
        }
        if (button && argc >= 3) {
            button->setTitleFontSize(fontSize);
        }
        node_to_luaval(tolua_S, button);
        return 1;
    }

    return reportWrongArgCount(tolua_S, "cc.Button:create", argc, 0, 3);
}

int lua_zocos_FontAtlas_create(lua_State* tolua_S) {
    const int base = classArgBase(tolua_S);
    const int argc = classArgCount(tolua_S);
    if (argc == 1 || argc == 2) {
        const char* fontPath = luaL_checkstring(tolua_S, base);
        const float fontSize =
            argc >= 2 ? static_cast<float>(luaL_checknumber(tolua_S, base + 1)) : 24.f;
        object_to_luaval(tolua_S, kFontAtlasMeta,
                         FontAtlas::create(Director::getInstance(), fontPath, fontSize));
        return 1;
    }

    return reportWrongArgCount(tolua_S, "cc.FontAtlas:create", argc, 1, 2);
}

int lua_zocos_MoveTo_create(lua_State* tolua_S) {
    const int base = classArgBase(tolua_S);
    const int argc = classArgCount(tolua_S);
    if (argc == 3) {
        const float duration = static_cast<float>(luaL_checknumber(tolua_S, base + 0));
        const float x = static_cast<float>(luaL_checknumber(tolua_S, base + 1));
        const float y = static_cast<float>(luaL_checknumber(tolua_S, base + 2));
        object_to_luaval(tolua_S, kActionMeta, MoveTo::create(duration, Vec2{x, y}));
        return 1;
    }

    return reportWrongArgCount(tolua_S, "cc.MoveTo:create", argc, 3);
}

int lua_zocos_MoveBy_create(lua_State* tolua_S) {
    const int base = classArgBase(tolua_S);
    const int argc = classArgCount(tolua_S);
    if (argc == 3) {
        const float duration = static_cast<float>(luaL_checknumber(tolua_S, base + 0));
        const float x = static_cast<float>(luaL_checknumber(tolua_S, base + 1));
        const float y = static_cast<float>(luaL_checknumber(tolua_S, base + 2));
        object_to_luaval(tolua_S, kActionMeta, MoveBy::create(duration, Vec2{x, y}));
        return 1;
    }

    return reportWrongArgCount(tolua_S, "cc.MoveBy:create", argc, 3);
}

int lua_zocos_RotateBy_create(lua_State* tolua_S) {
    const int base = classArgBase(tolua_S);
    const int argc = classArgCount(tolua_S);
    if (argc == 2) {
        const float duration = static_cast<float>(luaL_checknumber(tolua_S, base + 0));
        const float delta = static_cast<float>(luaL_checknumber(tolua_S, base + 1));
        object_to_luaval(tolua_S, kActionMeta, RotateBy::create(duration, delta));
        return 1;
    }

    return reportWrongArgCount(tolua_S, "cc.RotateBy:create", argc, 2);
}

int lua_zocos_RotateTo_create(lua_State* tolua_S) {
    const int base = classArgBase(tolua_S);
    const int argc = classArgCount(tolua_S);
    if (argc == 2) {
        const float duration = static_cast<float>(luaL_checknumber(tolua_S, base + 0));
        const float value = static_cast<float>(luaL_checknumber(tolua_S, base + 1));
        object_to_luaval(tolua_S, kActionMeta, RotateTo::create(duration, value));
        return 1;
    }

    return reportWrongArgCount(tolua_S, "cc.RotateTo:create", argc, 2);
}

int lua_zocos_ScaleTo_create(lua_State* tolua_S) {
    const int base = classArgBase(tolua_S);
    const int argc = classArgCount(tolua_S);
    if (argc == 2 || argc == 3) {
        const float duration = static_cast<float>(luaL_checknumber(tolua_S, base + 0));
        const float sx = static_cast<float>(luaL_checknumber(tolua_S, base + 1));
        const float sy = static_cast<float>(luaL_optnumber(tolua_S, base + 2, sx));
        object_to_luaval(tolua_S, kActionMeta, ScaleTo::create(duration, Vec2{sx, sy}));
        return 1;
    }

    return reportWrongArgCount(tolua_S, "cc.ScaleTo:create", argc, 2, 3);
}

int lua_zocos_FadeTo_create(lua_State* tolua_S) {
    const int base = classArgBase(tolua_S);
    const int argc = classArgCount(tolua_S);
    if (argc == 2) {
        const float duration = static_cast<float>(luaL_checknumber(tolua_S, base + 0));
        const float opacity = static_cast<float>(luaL_checknumber(tolua_S, base + 1));
        object_to_luaval(tolua_S, kActionMeta, FadeTo::create(duration, opacity));
        return 1;
    }

    return reportWrongArgCount(tolua_S, "cc.FadeTo:create", argc, 2);
}

int lua_zocos_DelayTime_create(lua_State* tolua_S) {
    const int base = classArgBase(tolua_S);
    const int argc = classArgCount(tolua_S);
    if (argc == 1) {
        const float duration = static_cast<float>(luaL_checknumber(tolua_S, base));
        object_to_luaval(tolua_S, kActionMeta, DelayTime::create(duration));
        return 1;
    }

    return reportWrongArgCount(tolua_S, "cc.DelayTime:create", argc, 1);
}

int lua_zocos_Animation_create(lua_State* tolua_S) {
    const int base = classArgBase(tolua_S);
    const int argc = classArgCount(tolua_S);
    if (argc == 1 || argc == 2) {
        mstd::vector<Rect> frames =
            luaval_to_rect_array(tolua_S, base, "Animation frame must be {x, y, width, height}");
        const float delayPerFrame = static_cast<float>(luaL_optnumber(tolua_S, base + 1, 0.1));
        object_to_luaval(tolua_S, kAnimationMeta, Animation::create(frames, delayPerFrame));
        return 1;
    }

    return reportWrongArgCount(tolua_S, "cc.Animation:create", argc, 1, 2);
}

int lua_zocos_Animate_create(lua_State* tolua_S) {
    const int base = classArgBase(tolua_S);
    const int argc = classArgCount(tolua_S);
    if (argc == 1) {
        Animation* animation =
            luaval_to_object<Animation>(tolua_S, base, kAnimationMeta, "Animation expected");
        object_to_luaval(tolua_S, kActionMeta, Animate::create(animation));
        return 1;
    }

    return reportWrongArgCount(tolua_S, "cc.Animate:create", argc, 1);
}

int lua_zocos_Sequence_create(lua_State* tolua_S) {
    const int base = classArgBase(tolua_S);
    const int argc = classArgCount(tolua_S);
    if (argc == 1) {
        object_to_luaval(tolua_S, kActionMeta,
                         Sequence::create(luaval_to_object_array<Action>(tolua_S, base, kActionMeta,
                                                                         "Action expected")));
        return 1;
    }

    return reportWrongArgCount(tolua_S, "cc.Sequence:create", argc, 1);
}

int lua_zocos_Spawn_create(lua_State* tolua_S) {
    const int base = classArgBase(tolua_S);
    const int argc = classArgCount(tolua_S);
    if (argc == 1) {
        object_to_luaval(tolua_S, kActionMeta,
                         Spawn::create(luaval_to_object_array<Action>(tolua_S, base, kActionMeta,
                                                                      "Action expected")));
        return 1;
    }

    return reportWrongArgCount(tolua_S, "cc.Spawn:create", argc, 1);
}

int lua_zocos_Repeat_create(lua_State* tolua_S) {
    const int base = classArgBase(tolua_S);
    const int argc = classArgCount(tolua_S);
    if (argc == 2) {
        Action* inner = luaval_to_object<Action>(tolua_S, base + 0, kActionMeta, "Action expected");
        const int times = static_cast<int>(luaL_checkinteger(tolua_S, base + 1));
        object_to_luaval(tolua_S, kActionMeta, Repeat::create(inner, times));
        return 1;
    }

    return reportWrongArgCount(tolua_S, "cc.Repeat:create", argc, 2);
}

int lua_zocos_RepeatForever_create(lua_State* tolua_S) {
    const int base = classArgBase(tolua_S);
    const int argc = classArgCount(tolua_S);
    if (argc == 1) {
        Action* inner = luaval_to_object<Action>(tolua_S, base, kActionMeta, "Action expected");
        object_to_luaval(tolua_S, kActionMeta, RepeatForever::create(inner));
        return 1;
    }

    return reportWrongArgCount(tolua_S, "cc.RepeatForever:create", argc, 1);
}

int lua_zocos_Node_setPosition(lua_State* tolua_S) {
    Node* cobj = luaval_to_object<Node>(tolua_S, 1, kNodeMeta, "Node expected");
    const int argc = lua_gettop(tolua_S) - 1;
    if (argc == 2) {
        const float x = static_cast<float>(luaL_checknumber(tolua_S, 2));
        const float y = static_cast<float>(luaL_checknumber(tolua_S, 3));
        cobj->setPosition(x, y);
        return 0;
    }

    return reportWrongArgCount(tolua_S, "cc.Node:setPosition", argc, 2);
}

int lua_zocos_Node_setScale(lua_State* tolua_S) {
    Node* cobj = luaval_to_object<Node>(tolua_S, 1, kNodeMeta, "Node expected");
    const int argc = lua_gettop(tolua_S) - 1;
    if (argc == 1 || argc == 2) {
        const float sx = static_cast<float>(luaL_checknumber(tolua_S, 2));
        const float sy = static_cast<float>(luaL_optnumber(tolua_S, 3, sx));
        cobj->setScale(Vec2{sx, sy});
        return 0;
    }

    return reportWrongArgCount(tolua_S, "cc.Node:setScale", argc, 1, 2);
}

int lua_zocos_Node_setRotation(lua_State* tolua_S) {
    Node* cobj = luaval_to_object<Node>(tolua_S, 1, kNodeMeta, "Node expected");
    const int argc = lua_gettop(tolua_S) - 1;
    if (argc == 1) {
        const float degrees = static_cast<float>(luaL_checknumber(tolua_S, 2));
        cobj->setRotation(degrees);
        return 0;
    }

    return reportWrongArgCount(tolua_S, "cc.Node:setRotation", argc, 1);
}

int lua_zocos_Node_setAnchorPoint(lua_State* tolua_S) {
    Node* cobj = luaval_to_object<Node>(tolua_S, 1, kNodeMeta, "Node expected");
    const int argc = lua_gettop(tolua_S) - 1;
    if (argc == 2) {
        const float x = static_cast<float>(luaL_checknumber(tolua_S, 2));
        const float y = static_cast<float>(luaL_checknumber(tolua_S, 3));
        cobj->setAnchorPoint(Vec2{x, y});
        return 0;
    }

    return reportWrongArgCount(tolua_S, "cc.Node:setAnchorPoint", argc, 2);
}

int lua_zocos_Node_setOpacity(lua_State* tolua_S) {
    Node* cobj = luaval_to_object<Node>(tolua_S, 1, kNodeMeta, "Node expected");
    const int argc = lua_gettop(tolua_S) - 1;
    if (argc == 1) {
        const float opacity = static_cast<float>(luaL_checknumber(tolua_S, 2));
        cobj->setOpacity(opacity);
        return 0;
    }

    return reportWrongArgCount(tolua_S, "cc.Node:setOpacity", argc, 1);
}

int lua_zocos_Node_setContentSize(lua_State* tolua_S) {
    Node* cobj = luaval_to_object<Node>(tolua_S, 1, kNodeMeta, "Node expected");
    const int argc = lua_gettop(tolua_S) - 1;
    if (argc == 2) {
        const float width = static_cast<float>(luaL_checknumber(tolua_S, 2));
        const float height = static_cast<float>(luaL_checknumber(tolua_S, 3));
        cobj->setContentSize(Size{width, height});
        return 0;
    }

    return reportWrongArgCount(tolua_S, "cc.Node:setContentSize", argc, 2);
}

int lua_zocos_Node_addChild(lua_State* tolua_S) {
    Node* cobj = luaval_to_object<Node>(tolua_S, 1, kNodeMeta, "Node expected");
    const int argc = lua_gettop(tolua_S) - 1;
    if (argc == 1) {
        Node* child = luaval_to_object<Node>(tolua_S, 2, kNodeMeta, "Node expected");
        cobj->addChild(child);
        return 0;
    }

    return reportWrongArgCount(tolua_S, "cc.Node:addChild", argc, 1);
}

int lua_zocos_Node_runAction(lua_State* tolua_S) {
    Node* cobj = luaval_to_object<Node>(tolua_S, 1, kNodeMeta, "Node expected");
    const int argc = lua_gettop(tolua_S) - 1;
    if (argc == 1) {
        Action* action = luaval_to_object<Action>(tolua_S, 2, kActionMeta, "Action expected");
        object_to_luaval(tolua_S, kActionMeta, cobj->runAction(action));
        return 1;
    }

    return reportWrongArgCount(tolua_S, "cc.Node:runAction", argc, 1);
}

int lua_zocos_Node_schedule(lua_State* tolua_S) {
    Node* cobj = luaval_to_object<Node>(tolua_S, 1, kNodeMeta, "Node expected");
    const int argc = lua_gettop(tolua_S) - 1;
    if (argc >= 2 && argc <= 6) {
        const char* key = luaL_checkstring(tolua_S, 2);
        luaL_argcheck(tolua_S, key != nullptr && key[0] != '\0', 2, "non-empty key expected");
        luaL_checktype(tolua_S, 3, LUA_TFUNCTION);

        const float interval = static_cast<float>(luaL_optnumber(tolua_S, 4, 0.0));
        const int repeat = static_cast<int>(luaL_optinteger(tolua_S, 5, Node::RepeatForever));
        const float delay = static_cast<float>(luaL_optnumber(tolua_S, 6, 0.0));
        const int priority = static_cast<int>(luaL_optinteger(tolua_S, 7, 0));

        lua_pushvalue(tolua_S, 3);
        const int callbackRef = luaL_ref(tolua_S, LUA_REGISTRYINDEX);
        const mstd::string keyString(key);
        setLuaScheduleRef(tolua_S, cobj, keyString, callbackRef);

        cobj->schedule(
            keyString,
            [tolua_S, cobj, keyString](float dt) {
                const int ref = findLuaScheduleRef(cobj, keyString);
                if (ref == LUA_NOREF) {
                    return;
                }

                lua_rawgeti(tolua_S, LUA_REGISTRYINDEX, ref);
                lua_pushnumber(tolua_S, static_cast<lua_Number>(dt));
                if (lua_pcall(tolua_S, 1, 0, 0) != LUA_OK) {
                    const char* message = lua_tostring(tolua_S, -1);
                    std::fprintf(stderr, "Lua schedule callback error (%s): %s\n",
                                 keyString.c_str(), message ? message : "(unknown)");
                    lua_pop(tolua_S, 1);
                }
            },
            interval, repeat, delay, priority);
        return 0;
    }

    return reportWrongArgCount(tolua_S, "cc.Node:schedule", argc, 2, 6);
}

int lua_zocos_Node_scheduleOnce(lua_State* tolua_S) {
    Node* cobj = luaval_to_object<Node>(tolua_S, 1, kNodeMeta, "Node expected");
    const int argc = lua_gettop(tolua_S) - 1;
    if (argc >= 2 && argc <= 4) {
        const char* key = luaL_checkstring(tolua_S, 2);
        luaL_argcheck(tolua_S, key != nullptr && key[0] != '\0', 2, "non-empty key expected");
        luaL_checktype(tolua_S, 3, LUA_TFUNCTION);

        const float delay = static_cast<float>(luaL_optnumber(tolua_S, 4, 0.0));
        const int priority = static_cast<int>(luaL_optinteger(tolua_S, 5, 0));

        lua_pushvalue(tolua_S, 3);
        const int callbackRef = luaL_ref(tolua_S, LUA_REGISTRYINDEX);
        const mstd::string keyString(key);
        setLuaScheduleRef(tolua_S, cobj, keyString, callbackRef);

        cobj->scheduleOnce(
            keyString,
            [tolua_S, cobj, keyString, callbackRef](float dt) {
                const int ref = findLuaScheduleRef(cobj, keyString);
                if (ref == LUA_NOREF) {
                    return;
                }

                lua_rawgeti(tolua_S, LUA_REGISTRYINDEX, ref);
                lua_pushnumber(tolua_S, static_cast<lua_Number>(dt));
                if (lua_pcall(tolua_S, 1, 0, 0) != LUA_OK) {
                    const char* message = lua_tostring(tolua_S, -1);
                    std::fprintf(stderr, "Lua scheduleOnce callback error (%s): %s\n",
                                 keyString.c_str(), message ? message : "(unknown)");
                    lua_pop(tolua_S, 1);
                }

                clearLuaScheduleRef(tolua_S, cobj, keyString, callbackRef);
            },
            delay, priority);
        return 0;
    }

    return reportWrongArgCount(tolua_S, "cc.Node:scheduleOnce", argc, 2, 4);
}

int lua_zocos_Node_unschedule(lua_State* tolua_S) {
    Node* cobj = luaval_to_object<Node>(tolua_S, 1, kNodeMeta, "Node expected");
    const int argc = lua_gettop(tolua_S) - 1;
    if (argc == 1) {
        const char* key = luaL_checkstring(tolua_S, 2);
        const mstd::string keyString = key ? key : "";
        cobj->unschedule(keyString);
        clearLuaScheduleRef(tolua_S, cobj, keyString);
        return 0;
    }

    return reportWrongArgCount(tolua_S, "cc.Node:unschedule", argc, 1);
}

int lua_zocos_Node_unscheduleAllCallbacks(lua_State* tolua_S) {
    Node* cobj = luaval_to_object<Node>(tolua_S, 1, kNodeMeta, "Node expected");
    const int argc = lua_gettop(tolua_S) - 1;
    if (argc == 0) {
        cobj->unscheduleAllCallbacks();
        clearAllLuaScheduleRefs(tolua_S, cobj);
        return 0;
    }

    return reportWrongArgCount(tolua_S, "cc.Node:unscheduleAllCallbacks", argc, 0);
}

int lua_zocos_Sprite_initWithFile(lua_State* tolua_S) {
    Sprite* cobj = luaval_to_object<Sprite>(tolua_S, 1, kSpriteMeta, "Sprite expected");
    const int argc = lua_gettop(tolua_S) - 1;
    if (argc == 1) {
        const char* path = luaL_checkstring(tolua_S, 2);
        lua_pushboolean(tolua_S, cobj->initWithFile(path));
        return 1;
    }

    return reportWrongArgCount(tolua_S, "cc.Sprite:initWithFile", argc, 1);
}

int lua_zocos_Sprite_initWithCheckerboard(lua_State* tolua_S) {
    Sprite* cobj = luaval_to_object<Sprite>(tolua_S, 1, kSpriteMeta, "Sprite expected");
    const int argc = lua_gettop(tolua_S) - 1;
    if (argc == 0) {
        cobj->initWithCheckerboard();
        return 0;
    }

    return reportWrongArgCount(tolua_S, "cc.Sprite:initWithCheckerboard", argc, 0);
}

int lua_zocos_Sprite_setTextureRect(lua_State* tolua_S) {
    Sprite* cobj = luaval_to_object<Sprite>(tolua_S, 1, kSpriteMeta, "Sprite expected");
    const int argc = lua_gettop(tolua_S) - 1;
    if (argc == 4 || argc == 5) {
        const float x = static_cast<float>(luaL_checknumber(tolua_S, 2));
        const float y = static_cast<float>(luaL_checknumber(tolua_S, 3));
        const float width = static_cast<float>(luaL_checknumber(tolua_S, 4));
        const float height = static_cast<float>(luaL_checknumber(tolua_S, 5));
        const bool resetSize = lua_gettop(tolua_S) >= 6 ? lua_toboolean(tolua_S, 6) != 0 : true;
        cobj->setTextureRect(Rect{x, y, width, height}, resetSize);
        return 0;
    }

    return reportWrongArgCount(tolua_S, "cc.Sprite:setTextureRect", argc, 4, 5);
}

int lua_zocos_Label_setString(lua_State* tolua_S) {
    Label* cobj = luaval_to_object<Label>(tolua_S, 1, kLabelMeta, "Label expected");
    const int argc = lua_gettop(tolua_S) - 1;
    if (argc == 1) {
        const char* text = luaL_checkstring(tolua_S, 2);
        cobj->setString(text);
        return 0;
    }

    return reportWrongArgCount(tolua_S, "cc.Label:setString", argc, 1);
}

int lua_zocos_Label_setFontAtlas(lua_State* tolua_S) {
    Label* cobj = luaval_to_object<Label>(tolua_S, 1, kLabelMeta, "Label expected");
    const int argc = lua_gettop(tolua_S) - 1;
    if (argc == 1) {
        auto* fontAtlas =
            luaval_to_object<FontAtlas>(tolua_S, 2, kFontAtlasMeta, "FontAtlas expected");
        lua_pushboolean(tolua_S, cobj->setFontAtlas(fontAtlas));
        return 1;
    }

    return reportWrongArgCount(tolua_S, "cc.Label:setFontAtlas", argc, 1);
}

int lua_zocos_Label_setFontSize(lua_State* tolua_S) {
    Label* cobj = luaval_to_object<Label>(tolua_S, 1, kLabelMeta, "Label expected");
    const int argc = lua_gettop(tolua_S) - 1;
    if (argc == 1) {
        const float fontSize = static_cast<float>(luaL_checknumber(tolua_S, 2));
        cobj->setFontSize(fontSize);
        return 0;
    }

    return reportWrongArgCount(tolua_S, "cc.Label:setFontSize", argc, 1);
}

int lua_zocos_Button_setString(lua_State* tolua_S) {
    ui::Button* cobj = luaval_to_object<ui::Button>(tolua_S, 1, kButtonMeta, "Button expected");
    const int argc = lua_gettop(tolua_S) - 1;
    if (argc == 1) {
        const char* text = luaL_checkstring(tolua_S, 2);
        cobj->setString(text);
        return 0;
    }

    return reportWrongArgCount(tolua_S, "cc.Button:setString", argc, 1);
}

int lua_zocos_Button_setTitleFontName(lua_State* tolua_S) {
    ui::Button* cobj = luaval_to_object<ui::Button>(tolua_S, 1, kButtonMeta, "Button expected");
    const int argc = lua_gettop(tolua_S) - 1;
    if (argc == 1) {
        const char* fontPath = luaL_checkstring(tolua_S, 2);
        lua_pushboolean(tolua_S, cobj->setTitleFontName(fontPath));
        return 1;
    }

    return reportWrongArgCount(tolua_S, "cc.Button:setTitleFontName", argc, 1);
}

int lua_zocos_Button_setFontAtlas(lua_State* tolua_S) {
    ui::Button* cobj = luaval_to_object<ui::Button>(tolua_S, 1, kButtonMeta, "Button expected");
    const int argc = lua_gettop(tolua_S) - 1;
    if (argc == 1) {
        auto* fontAtlas =
            luaval_to_object<FontAtlas>(tolua_S, 2, kFontAtlasMeta, "FontAtlas expected");
        lua_pushboolean(tolua_S, cobj->setFontAtlas(fontAtlas));
        return 1;
    }

    return reportWrongArgCount(tolua_S, "cc.Button:setFontAtlas", argc, 1);
}

int lua_zocos_Button_setFontSize(lua_State* tolua_S) {
    ui::Button* cobj = luaval_to_object<ui::Button>(tolua_S, 1, kButtonMeta, "Button expected");
    const int argc = lua_gettop(tolua_S) - 1;
    if (argc == 1) {
        const float fontSize = static_cast<float>(luaL_checknumber(tolua_S, 2));
        cobj->setTitleFontSize(fontSize);
        return 0;
    }

    return reportWrongArgCount(tolua_S, "cc.Button:setFontSize", argc, 1);
}

int lua_zocos_Button_setTitleFontSize(lua_State* tolua_S) {
    ui::Button* cobj = luaval_to_object<ui::Button>(tolua_S, 1, kButtonMeta, "Button expected");
    const int argc = lua_gettop(tolua_S) - 1;
    if (argc == 1) {
        const float fontSize = static_cast<float>(luaL_checknumber(tolua_S, 2));
        cobj->setTitleFontSize(fontSize);
        return 0;
    }

    return reportWrongArgCount(tolua_S, "cc.Button:setTitleFontSize", argc, 1);
}

int lua_zocos_Widget_addEventListener(lua_State* tolua_S) {
    ui::Widget* cobj = luaval_to_object<ui::Widget>(tolua_S, 1, kWidgetMeta, "Widget expected");

    const int argc = lua_gettop(tolua_S) - 1;
    if (argc == 1) {
        if (lua_isnil(tolua_S, 2)) {
            clearLuaWidgetEventRef(tolua_S, cobj);
            cobj->addEventListener({});
            return 0;
        }

        luaL_checktype(tolua_S, 2, LUA_TFUNCTION);
        lua_pushvalue(tolua_S, 2);
        const int callbackRef = luaL_ref(tolua_S, LUA_REGISTRYINDEX);
        setLuaWidgetEventRef(tolua_S, cobj, callbackRef);

        cobj->addEventListener([tolua_S, cobj](ui::Widget& sender) {
            const int ref = findLuaWidgetEventRef(cobj);
            if (ref == LUA_NOREF) {
                return;
            }

            lua_rawgeti(tolua_S, LUA_REGISTRYINDEX, ref);
            node_to_luaval(tolua_S, static_cast<Node*>(&sender));
            if (lua_pcall(tolua_S, 1, 0, 0) != LUA_OK) {
                const char* message = lua_tostring(tolua_S, -1);
                std::fprintf(stderr, "Lua widget event callback error: %s\n",
                             message ? message : "(unknown)");
                lua_pop(tolua_S, 1);
            }
        });
        return 0;
    }

    return reportWrongArgCount(tolua_S, "cc.Widget:addEventListener", argc, 1);
}

int lua_zocos_Action_setTag(lua_State* tolua_S) {
    Action* cobj = luaval_to_object<Action>(tolua_S, 1, kActionMeta, "Action expected");
    const int argc = lua_gettop(tolua_S) - 1;
    if (argc == 1) {
        const int tag = static_cast<int>(luaL_checkinteger(tolua_S, 2));
        cobj->setTag(tag);
        return 0;
    }

    return reportWrongArgCount(tolua_S, "cc.Action:setTag", argc, 1);
}

int lua_zocos_Action_getTag(lua_State* tolua_S) {
    Action* cobj = luaval_to_object<Action>(tolua_S, 1, kActionMeta, "Action expected");
    const int argc = lua_gettop(tolua_S) - 1;
    if (argc == 0) {
        lua_pushinteger(tolua_S, cobj->getTag());
        return 1;
    }

    return reportWrongArgCount(tolua_S, "cc.Action:getTag", argc, 0);
}

void registerMetatable(lua_State* tolua_S, const char* metatableName, const luaL_Reg* methods,
                       const char* baseMetatableName = nullptr) {
    luaL_newmetatable(tolua_S, metatableName);

    lua_newtable(tolua_S);
    if (methods) {
        luaL_setfuncs(tolua_S, methods, 0);
    }

    if (baseMetatableName && baseMetatableName[0] != '\0') {
        luaL_getmetatable(tolua_S, baseMetatableName);
        if (lua_istable(tolua_S, -1)) {
            lua_getfield(tolua_S, -1, "__index");
            if (lua_istable(tolua_S, -1)) {
                lua_newtable(tolua_S);
                lua_pushvalue(tolua_S, -2);
                lua_setfield(tolua_S, -2, "__index");
                lua_setmetatable(tolua_S, -4);
            }
            lua_pop(tolua_S, 1);
        }
        lua_pop(tolua_S, 1);
    }

    lua_pushvalue(tolua_S, -1);
    lua_setfield(tolua_S, -3, "__index");
    lua_pop(tolua_S, 2);
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

void tolua_endmodule(lua_State* tolua_S) { lua_pop(tolua_S, 1); }

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
        {"setContentSize", lua_zocos_Node_setContentSize},
        {"addChild", lua_zocos_Node_addChild},
        {"runAction", lua_zocos_Node_runAction},
        {"schedule", lua_zocos_Node_schedule},
        {"scheduleOnce", lua_zocos_Node_scheduleOnce},
        {"unschedule", lua_zocos_Node_unschedule},
        {"unscheduleAllCallbacks", lua_zocos_Node_unscheduleAllCallbacks},
        {nullptr, nullptr},
    };

    static const luaL_Reg sceneMethods[] = {
        {nullptr, nullptr},
    };

    static const luaL_Reg spriteMethods[] = {
        {"initWithFile", lua_zocos_Sprite_initWithFile},
        {"initWithCheckerboard", lua_zocos_Sprite_initWithCheckerboard},
        {"setTextureRect", lua_zocos_Sprite_setTextureRect},
        {nullptr, nullptr},
    };

    static const luaL_Reg labelMethods[] = {
        {"setString", lua_zocos_Label_setString},
        {"setFontAtlas", lua_zocos_Label_setFontAtlas},
        {"setFontSize", lua_zocos_Label_setFontSize},
        {nullptr, nullptr},
    };

    static const luaL_Reg widgetMethods[] = {
        {"addEventListener", lua_zocos_Widget_addEventListener},
        {nullptr, nullptr},
    };

    static const luaL_Reg buttonMethods[] = {
        {"setString", lua_zocos_Button_setString},
        {"setTitleFontName", lua_zocos_Button_setTitleFontName},
        {"setFontAtlas", lua_zocos_Button_setFontAtlas},
        {"setFontSize", lua_zocos_Button_setFontSize},
        {"setTitleFontSize", lua_zocos_Button_setTitleFontSize},
        {nullptr, nullptr},
    };

    static const luaL_Reg actionMethods[] = {
        {"setTag", lua_zocos_Action_setTag},
        {"getTag", lua_zocos_Action_getTag},
        {nullptr, nullptr},
    };

    static const luaL_Reg animationMethods[] = {
        {nullptr, nullptr},
    };

    static const luaL_Reg fontAtlasMethods[] = {
        {nullptr, nullptr},
    };

    registerMetatable(tolua_S, kDirectorMeta, directorMethods);
    registerMetatable(tolua_S, kNodeMeta, nodeMethods);
    registerMetatable(tolua_S, kSceneMeta, sceneMethods, kNodeMeta);
    registerMetatable(tolua_S, kSpriteMeta, spriteMethods, kNodeMeta);
    registerMetatable(tolua_S, kLabelMeta, labelMethods, kNodeMeta);
    registerMetatable(tolua_S, kWidgetMeta, widgetMethods, kNodeMeta);
    registerMetatable(tolua_S, kButtonMeta, buttonMethods, kWidgetMeta);
    registerMetatable(tolua_S, kActionMeta, actionMethods);
    registerMetatable(tolua_S, kAnimationMeta, animationMethods);
    registerMetatable(tolua_S, kFontAtlasMeta, fontAtlasMethods);

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
    tolua_function(tolua_S, "createWithTTF", lua_zocos_Label_createWithTTF);
    tolua_endmodule(tolua_S);

    tolua_beginmodule(tolua_S, "Button");
    stashCurrentModuleInRegistry(tolua_S, "Button");
    tolua_function(tolua_S, "create", lua_zocos_Button_create);
    tolua_endmodule(tolua_S);

    tolua_beginmodule(tolua_S, "FontAtlas");
    stashCurrentModuleInRegistry(tolua_S, "FontAtlas");
    tolua_function(tolua_S, "create", lua_zocos_FontAtlas_create);
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

    tolua_beginmodule(tolua_S, "Animation");
    stashCurrentModuleInRegistry(tolua_S, "Animation");
    tolua_function(tolua_S, "create", lua_zocos_Animation_create);
    tolua_endmodule(tolua_S);

    tolua_beginmodule(tolua_S, "Animate");
    stashCurrentModuleInRegistry(tolua_S, "Animate");
    tolua_function(tolua_S, "create", lua_zocos_Animate_create);
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

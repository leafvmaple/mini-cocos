#pragma once

struct lua_State;

namespace zocos {

int register_all_zocos(lua_State* tolua_S);
int register_all_zocos_manual(lua_State* tolua_S);
int register_all_zocos_manual_deprecated(lua_State* tolua_S);

} // namespace zocos

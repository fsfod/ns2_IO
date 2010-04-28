#include "StdAfx.h"
#include "LuaErrorWrapper.h"


void LuaErrorWrapper::DoLuaError(lua_State* L){
	lua_pushstring(L, this->what());
	lua_error(L);
}


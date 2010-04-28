#pragma once
#include "stdafx.h"


class LuaErrorWrapper : public std::exception{
	public:
		LuaErrorWrapper(char* ErrorMessage) : std::exception(ErrorMessage){}
		virtual void DoLuaError(lua_State* L);
};



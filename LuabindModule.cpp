
#include "lua.h"
#include "luabind/open.hpp"

__declspec(dllexport) int __cdecl luabind_open(lua_State* L){

  luabind::open(L);

 return 0;
}

#include "lua.h"

namespace luabind {
   void open(lua_State* L);
}

extern "C" __declspec(dllexport) int __cdecl luaopen_luabind(lua_State* L){

  luabind::open(L);

 return 0;
}
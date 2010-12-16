#pragma once
#include "StringUtil.h"
#include "PathStringArg.h"

namespace luabind{
	template <>struct default_converter<PathStringArg>: native_converter_base<PathStringArg>{

		int compute_score(lua_State* L, int index){
			return lua_type(L, index) == LUA_TSTRING ? 0 : -1;
		}

		PathStringArg from(lua_State* L, int index){
			size_t StringSize = 0;
			const char* s = luaL_checklstring (L, index, &StringSize);

			if(s == NULL){
				throw std::exception("Missing string argument TODO");
			}

			size_t len = strlen(s);

			if(len != StringSize){
				throw std::exception("Path string arguments can't contain embedded nulls");
			}

			return PathStringArg(s, len);
		}

		void to(lua_State* L, PathStringArg const& path){
			throw std::exception("Not implemented");
			//lua_pushlstring(L, path.c_str(), path.size());
		}
	};
	
	template <>struct default_converter<const PathStringArg>: default_converter<PathStringArg>{};
	template <>struct default_converter<PathStringArg const&>: default_converter<PathStringArg>{};
}
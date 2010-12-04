#pragma once
#include "StringUtil.h"

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

			if(s[0] == '/' || s[0] == '\\'){
				s = s+ 1;
			}

			PathStringArg path;
#if defined(UnicodePathString)
			UTF8ToWString(s, len, path);
#else
			path.assign(s);
#endif
			std::for_each(path.begin(), path.end(), [](PathString::value_type& c){if(c == '\\')c = '/';});

			if(path.find_first_of(_T(":%|<>\"")) != NULL){
				throw std::exception("The Path cannot contain :%|<>\"");
			}

			if(path.find(L"..") != -1){
				throw std::exception("The Path cannot contain the directory up string \"..\"");
			}

			return path;
		}

		void to(lua_State* L, PathStringArg const& path){
#if defined(UNICODE)
			std::string utf8Path;
			WStringToUTF8STLString(path, utf8Path);

			lua_pushlstring(L, utf8Path.c_str(), utf8Path.size());
#else
			lua_pushlstring(L, path.c_str(), path.size());
#endif
		}
	};
	
	template <>struct default_converter<const PathStringArg>: default_converter<PathStringArg>{};
	template <>struct default_converter<PathStringArg const&>: default_converter<PathStringArg>{};
}
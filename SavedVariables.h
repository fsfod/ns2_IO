#pragma once
#include "stdafx.h"

class luabind::object;

class SavedVariables{
	public:
		SavedVariables(const std::string& fname, luabind::table<luabind::object> const& table);
		virtual ~SavedVariables();

		int Save(lua_State *L);
		void Load(lua_State *L);

		static void RegisterObjects(lua_State *L);

private:
		void SaveTable(lua_State *L);
		bool SerializeCustomType(lua_State *L, int index);
		bool IsaWrapper(lua_State *L, int index, const char* type);

		PathString FileName;
		std::vector<std::string> VariableNames;
		std::string IndentString;
		FILE* SavedVariableFile;

		static void CheckCreateSVDir();
		static void CacheObjectIdentData(lua_State* L);

		static PathString SavedVariablesFolderPath;
};


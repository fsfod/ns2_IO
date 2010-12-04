#pragma once
#include "stdafx.h"

class luabind::object;

struct VarTableRef{

	VarTableRef(){}
	
	VarTableRef(std::string tableName, std::string globalName){
		TableName = tableName;
		globalName = GlobalName;
	}

	std::string TableName;
	std::string GlobalName;
	luabind::handle Table;
};

class SavedVariables{
	public:
		SavedVariables(lua_State* L, const std::string& fname, luabind::table<luabind::object> const& tableNames);
		SavedVariables(lua_State* L, const std::string& fname, luabind::table<luabind::object> const& tableNames, luabind::table<luabind::object> const& containerTable);
		virtual ~SavedVariables();

		int Save(lua_State *L);
		void Load(lua_State *L);
		void SetExitAutoSaving(bool AutoSave);

		static void RegisterObjects(lua_State *L);

private:
		void Init(lua_State* L, const std::string& fname, luabind::table<luabind::object> const& tableNames);
		void SaveTable(lua_State *L);
		bool SerializeCustomType(lua_State *L, int index);
		bool IsaWrapper(lua_State *L, int index, const char* type);
		void CreateShutdownCallback(lua_State *L);

		PathString FileName;
		std::vector<std::string> VariableNames;
		luabind::object ContainerTable;
		std::string IndentString;
		FILE* SavedVariableFile;
		void** ShutdownClosureThisPointer;

		static void CheckCreateSVDir();
		static void CacheObjectIdentData(lua_State* L);
		static int ShutdownCallback(lua_State *L);

		static PathString SavedVariablesFolderPath;
		bool ExitAutoSave;
};


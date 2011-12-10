#pragma once
#include "stdafx.h"
#include "ISavedVariables.h"

class SavedVariables : public ISavedVariables{
	public:
    SavedVariables(lua_State* L, const std::string& fname, LuabindTable const& tableNames, LuabindTable const& containerTable) 
      :SavedVariableFile(NULL), ShutdownClosureThisPointer(NULL){

      ExitAutoSave = true;

      Setup(L, fname, tableNames, containerTable);
    }

    virtual ~SavedVariables();
		
    void virtual Setup(lua_State* L, const std::string& fname, luabind::table<luabind::object> const& tableNames, luabind::table<luabind::object> const& containerTable);
    
		virtual void Save(lua_State *L);
		virtual void Load(lua_State *L);
		void SetExitAutoSaving(bool AutoSave);

		static void RegisterObjects(lua_State *L);

private:
		void Init(lua_State* L, const std::string& fname, luabind::table<luabind::object> const& tableNames);
		void SaveTable(lua_State *L);
		bool SerializeCustomType(lua_State *L, int index);
		bool IsaWrapper(lua_State *L, int index, const char* type);
		void CreateShutdownCallback(lua_State *L);

		PlatformPath FileName;

		std::vector<std::string> VariableNames;
		luabind::object ContainerTable;
    
    FILE* SavedVariableFile;
    
		std::string IndentString;
		void** ShutdownClosureThisPointer;

		static void CheckCreateSVDir();
		static int ShutdownCallback(lua_State *L);

		static PlatformPath SavedVariablesFolderPath;
};

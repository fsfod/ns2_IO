#pragma once
#include "stdafx.h"
#include "SQLTableSerializer.h"

class luabind::object;

class SQLiteDatabase;
class SQLitePreparedStatment;

class SavedVariables{
	public:
    friend class SQLTableSerializer;

		SavedVariables(lua_State* L, const std::string& fname, luabind::table<luabind::object> const& tableNames);
		SavedVariables(lua_State* L, const std::string& fname, luabind::table<luabind::object> const& tableNames, luabind::table<luabind::object> const& containerTable);
		virtual ~SavedVariables();
    
    int SaveSQL(lua_State *L);
    int LoadSQL(lua_State *L);
    
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
    
    void SaveSQL_Table(lua_State *L, int tableIndex, SQLitePreparedStatment& InsertStatement);

    inline bool ReadSQLTableValue(lua_State *L, SQLitePreparedStatment& statement, int column);
    inline bool SaveSQLTableValue(lua_State *L, SQLitePreparedStatment& InsertStatement, int column, int luaIndex = -1);
    
    void SaveSQLTable(lua_State *L, const string& tableName, vector<string>& fieldList, bool hashTable);
    int GetTableId(const void* table, bool& seenBefore);

    string Name;
		PlatformPath FileName;

		std::vector<std::string> VariableNames;
		luabind::object ContainerTable;
    
    std::unordered_map<const void*, int> FoundTables;

    FILE* SavedVariableFile;
    SQLiteDatabase* db;
    int TableCount;

		std::string IndentString;
		void** ShutdownClosureThisPointer;

		static void CheckCreateSVDir();
		static void CacheObjectIdentData(lua_State* L);
		static int ShutdownCallback(lua_State *L);

    template<int luaIndex, int columnOffset> bool SaveLuaValue(lua_State *L, SQLitePreparedStatment& stmt, int& saveTableindex);
    template<int columnOffset> bool LoadSQLValue(lua_State *L, SQLitePreparedStatment& stmt, int typeTypeField, int valueType);
    
    vector<SQLTableSerializer> TableEntrys;

		static PlatformPath SavedVariablesFolderPath;
		bool ExitAutoSave;
};


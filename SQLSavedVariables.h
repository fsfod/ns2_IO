#pragma once
#include "stdafx.h"
#include "SQLTableSerializer.h"
#include "ISavedVariables.h"

class luabind::object;

class SQLiteDatabase;
class SQLitePreparedStatment;

class SQLSavedVariables : public ISavedVariables{

public:
  friend class SQLTableSerializer;

  SQLSavedVariables();
  virtual ~SQLSavedVariables();

  virtual void Setup(lua_State* L, const std::string& fname, LuabindTable const& tableNames, LuabindTable const& containerTable);

  virtual void Save(lua_State *L);
  virtual void Load(lua_State *L);

  virtual void SetExitAutoSaving(bool AutoSave);

private:
  void SaveSQL_Table(lua_State *L, int tableIndex, SQLitePreparedStatment& InsertStatement);

  inline bool ReadSQLTableValue(lua_State *L, SQLitePreparedStatment& statement, int column);
  inline bool SaveSQLTableValue(lua_State *L, SQLitePreparedStatment& InsertStatement, int column, int luaIndex = -1);

  void SaveSQLTable(lua_State *L, const string& tableName, vector<string>& fieldList, bool hashTable);

private:
  template<int luaIndex, int columnOffset> bool SaveLuaValue(lua_State *L, SQLitePreparedStatment& stmt, int& saveTableindex);
  template<int columnOffset> bool LoadSQLValue(lua_State *L, SQLitePreparedStatment& stmt, int typeTypeField, int valueType);
  
  int GetTableId(const void* table, bool& seenBefore);
  std::unordered_map<const void*, int> FoundTables;
  int TableCount;

  SQLiteDatabase* db;
  vector<SQLTableSerializer> TableEntrys;
  std::vector<std::string> VariableNames;
  luabind::object ContainerTable;
};
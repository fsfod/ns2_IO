#pragma once

#include "stdafx.h"

class SQLiteDatabase;
class SQLitePreparedStatment;
class SQLSavedVariables;
class SQLTableMetaData;

class SQLTableSerializer{

public:
  SQLTableSerializer(lua_State* L, const char* tableName, int infoTableIndex, SQLSavedVariables* sv);
  ~SQLTableSerializer();
  
  void Save(lua_State *L);
  void Load(lua_State* L);

private:
  const char* GetWildCardString(int fieldCount);
  string CreateFieldListString();

  void ReadTableSchema();
  SQLitePreparedStatment CreateSelectStatment();
  SQLitePreparedStatment CreateInsertStatment();

  void CreateTable(const string& tableName);
  
  bool IsHashTable();
  bool CheckIsHashTable(lua_State *L);

  bool PushLuaTable(lua_State *L, bool createTable = false);

  inline void BindLuaValue(lua_State *L, SQLitePreparedStatment& InsertStatement, int column, int luaIndex);
  void BindHashTableEntry(lua_State* L, SQLitePreparedStatment& query, int tableIndex, int baseFieldIndex, int fieldCount);

  inline bool ReadSqlValue(lua_State *L, SQLitePreparedStatment& statement, int column, int columType);

  SQLSavedVariables* SV;

  SQLiteDatabase* DB;
  string TableName;
  vector<string> FieldNames;
  SQLTableMetaData* MetaData;

  string CompiledFieldListString;

  bool IsHashTableChecked;  
  bool CachedIsHashTable;
  int DBTableKeyType;
};


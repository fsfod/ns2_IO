#include "StdAfx.h"
#include "SQLSavedVariables.h"
#include "StringUtil.h"
#include "SQLite3Database.h"

extern SQLiteDatabase clientDB;


struct SQLLuaType{
  enum SQLLuaType_t :int{
    //SQLite compactly stores the numbers 0 and 1 and NULL values
    Boolean = 0,
    Table = 1,
  };
};

class SQLLoadHelper{

public:
  SQLLoadHelper(lua_State *L,int startStack) : StackStartIndex(startStack){
    lua_createtable(L, 250, 0);
    OverFlowTable = lua_gettop(L);

    StackStartIndex = startStack+1;
  }

  void PushTable(lua_State *L, int tableId){
    lua_rawgeti(L, OverFlowTable, tableId);

    if(lua_type(L, -1) == LUA_TNIL){
      lua_pop(L, 1);
      lua_newtable(L);

      //duplicate the table since rawset pops it from the stack and we need to leave it on the stack for the caller
      lua_pushvalue(L, -1);

      lua_rawseti(L, tableId, OverFlowTable);
    }
  }

  template<int columnOffset> bool LoadSQLValue(lua_State *L, SQLitePreparedStatment& LoadStatement, int typeTypeField, int valueType){

    int size;

    if(typeTypeField == SQLITE_NULL){
      switch(valueType){
      case SQLITE_INTEGER:
      case SQLITE_FLOAT:
        lua_pushnumber(L, LoadStatement.GetDouble(6));
        break;

      case SQLITE_TEXT:
        lua_pushlstring(L, (const char*)LoadStatement.GetString(6, size), size);
        break;

      case SQLITE_BLOB:
        lua_pushlstring(L, (const char*)LoadStatement.GetBlob(6, size), size);
        break;

      default:
        return false;
      }
    }else{
      switch(typeTypeField){
      case SQLLuaType::Boolean:
        lua_pushboolean(L, LoadStatement.GetInt32(5));
        break;

      case SQLLuaType::Table:
        PushTable(L, LoadStatement.GetInt32(5));
        break;

      default:
        return false;
      }
    }

    return true;
  }

private:
  int OverFlowTable;
  int StackStartIndex;
};



void SQLSavedVariables::Load(lua_State *L){
  if(db == NULL){      
    return;
  }

  TableCount = 0;

  if(!db->TableExists(Name.c_str()) ){
    return;
  }

  if(ContainerTable.is_valid())return;

  BOOST_FOREACH(SQLTableSerializer& entry, TableEntrys){
    entry.Load(L);
  }

  SQLLoadHelper tableList(L, lua_gettop(L));

  ContainerTable.push(L);
  int ContainerIndex = lua_gettop(L);

  SQLitePreparedStatment LoadStatement = db->CreateStatmentFmt("SELECT * From [%s] ORDER BY TID, EntryID", Name.c_str());

  int currentArrayIndex = 1;

  while(LoadStatement.Step()){
    int keyType = LoadStatement.GetRowValueType(4);
    int keyTypeType = LoadStatement.GetRowValueType(3);

    int tableid = LoadStatement.GetInt32(2);

    if(keyTypeType == SQLITE_NULL && keyType == SQLITE_NULL){
      lua_pushnumber(L, currentArrayIndex++);
    }else{
      tableList.LoadSQLValue<3>(L, LoadStatement, keyTypeType, keyType);
    }

    tableList.LoadSQLValue<5>(L, LoadStatement, LoadStatement.GetRowValueType(5), LoadStatement.GetRowValueType(6));

    if(tableid == 0) {
      int size = 0;
      const char* FieldName = (const char*)LoadStatement.GetBlob(4, size);

      lua_setfield(L, ContainerIndex, FieldName);
    }
  }
}

template<int luaIndex, int columnOffset> inline bool SQLSavedVariables::SaveLuaValue(lua_State *L, SQLitePreparedStatment& stmt, int& saveTableindex){

  int FieldType = -1;

  switch(lua_type(L, luaIndex)){
  case LUA_TNUMBER:{
    double num = lua_tonumber(L, luaIndex);
    int value = (int)num;

    if(num == value){
      stmt.BindValue(columnOffset+1, value);
    }else{
      stmt.BindValue(columnOffset+1, num);
    }
                   }break;

  case LUA_TSTRING:{
    size_t size = 0;
    stmt.BindLuaString(columnOffset+1,  L, luaIndex);
    break;}

  case LUA_TBOOLEAN:
    FieldType = SQLLuaType::Boolean;
    stmt.BindValue(columnOffset+1, lua_toboolean(L, luaIndex));
    break;

  case LUA_TTABLE:{
    FieldType = SQLLuaType::Table;
    bool saveTable;
    int index = GetTableId(lua_topointer(L, luaIndex), saveTable);

    if(saveTable){
      saveTableindex = index;
    }

    stmt.BindValue(columnOffset+1, index);
    break;}

  default:
    return false;
  }

  if(FieldType == -1){
    //just insert a null since this is a type we can automatically deduce from the SQL type
    stmt.BindValue(columnOffset, nullptr);
  }else{
    stmt.BindValue(columnOffset, FieldType);
  }

  return true;
}


void SQLSavedVariables::SaveSQL_Table(lua_State *L, int tableIndex, SQLitePreparedStatment& InsertStatement){
  lua_pushnil(L);

  int InArray = 0;

  int KeyTableIndex = -1, ValueTableIndex = -1;
  bool SaveKeyTable = false;

  while(lua_next(L, -2) != 0){
    bool validValue = false;

    int FieldType = -1;

    switch(lua_type(L, -2)){
    case LUA_TNUMBER:{
      double num = lua_tonumber(L, -2);
      int value = (int)num;

      if(num == value){
        if(InArray != -1){
          if(InArray == value-1){
            InsertStatement.BindNullValue(3);
            break;
          }else{
            InArray = -1;
          }
        }

        InsertStatement.BindValue(3, value);
      }else{
        InsertStatement.BindValue(3, num);
      }
                     }break;

    case LUA_TSTRING:
      InsertStatement.BindLuaString(3, L, -2);
      break;

    case LUA_TTABLE:
      FieldType = SQLLuaType::Table;
      KeyTableIndex= GetTableId(lua_topointer(L, -2), SaveKeyTable);
      InsertStatement.BindValue(5, KeyTableIndex);
      break;

    default:
      validValue = false;
      break;
    }

    validValue = SaveLuaValue<-1, 4>(L, InsertStatement, ValueTableIndex);

    if(validValue){
      InsertStatement.BindValues(tableIndex, nullptr);

      if(FieldType == -1){
        //just insert a null since this is a type we can automatically deduce from the SQL type
        InsertStatement.BindValue(2, nullptr);
      }else{
        InsertStatement.BindValue(2, FieldType);
      }

      InsertStatement.ExecuteAndReset();
    }

    if(SaveKeyTable){
      lua_pushvalue(L, -2);
      SaveSQL_Table(L, KeyTableIndex, InsertStatement);
      lua_pop(L, 1);
      SaveKeyTable = false;
    }

    if(ValueTableIndex != -1){
      SaveSQL_Table(L, ValueTableIndex, InsertStatement);
      ValueTableIndex = -1;
    }

    lua_pop(L, 1);
  }
}

int SQLSavedVariables::GetTableId(const void* table, bool& notCreatedYet){

  auto it = FoundTables.find(table);

  if(it == FoundTables.end()){
    int index = ++TableCount;
    FoundTables[table] = index;

    notCreatedYet = true;

    return index;
  }else{
    notCreatedYet = false;
    return it->second;
  }
}

void SQLSavedVariables::Save(lua_State *L){

  if(db == NULL){      
    return;
  }

  TableCount = 0;

  bool TableExists = db->TableExists(Name.c_str());

  if(!TableExists){
    db->ExecuteSQLNoResultsFmt("create table if not exists [%s] (EntryID INTEGER PRIMARY KEY, TID integer, KT, Key, VT, Val)", Name.c_str());
  }

  SQLitePreparedStatment InsertStatement = db->CreateStatmentFmt("INSERT into [%s] values (NULL,?,?,?,?,?);", Name.c_str());

  SQLiteTransaction Transaction(*db);

  //if the table already exists clear out all the old values 
  //SQlite has optimization when there is no WHERE clause in a DELETE FROM it will just truncate the table instead of clearing out each row
  if(TableExists){
    db->ExecuteSQLNoResults(("delete from "+Name).c_str());
  }

  int ContainerIndex = LUA_GLOBALSINDEX;

  if(ContainerTable.is_valid()) {
    ContainerTable.push(L);
    ContainerIndex = lua_gettop(L);
  }

  int TableIndex;
  bool SaveTable = false;

  BOOST_FOREACH(const string& VariableName, VariableNames){
    lua_getfield(L, ContainerIndex, VariableName.c_str());

    int CurrentStackPos = lua_gettop(L);

    int type = lua_type(L, -1);

    if(type == LUA_TNIL){
      lua_pop(L, 1);
      continue;
    }

    bool invalid = false;

    if(SaveLuaValue<-1, 4>(L, InsertStatement, TableIndex)){
      //table index will always be 0 for root values and the field type and value will always be string
      InsertStatement.BindValues(0, nullptr, VariableName.c_str());

      InsertStatement.ExecuteAndReset();
    }

    if(TableIndex != -1){
      SaveSQL_Table(L, TableIndex, InsertStatement);
      TableIndex = -1;
    }

    _ASSERT_EXPR(lua_gettop(L) == CurrentStackPos, _T("Stack Position is not the position before the call to SaveTable"));

    lua_pop(L, 1);
  }


  Transaction.Commit();

  return;
}

void SQLSavedVariables::Setup( lua_State* L, const std::string& fname, luabind::table<luabind::object> const& tableNames, luabind::table<luabind::object> const& containerTable ){
  db = &clientDB;

  if(luabind::type(tableNames) == LUA_TTABLE){

    tableNames.push(L);
    lua_pushnil(L);

    while(lua_next(L, -2) != 0){
      int keyType = lua_type(L, -2);
      int valueType = lua_type(L, -1);


      if(valueType == LUA_TSTRING){
        VariableNames.push_back(lua_tostring(L, -1));
      }else{
        if(keyType == LUA_TSTRING){
          TableEntrys.push_back(SQLTableSerializer(L, lua_tostring(L, -2), -1, this));
        }
      }
      lua_pop(L, 1);
    }
  }

  //throw exception("Found an entry in the TableNames list that was not a string");

  if(Name.find_first_of(InvalidFileNameCharsA) != string::npos){
    throw exception("the SavedVariables name cannot contain any of these characters  \\ \" . ");
  }

  Name = fname;


}

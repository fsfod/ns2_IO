#include "StdAfx.h"
#include <sstream>
#include "SQLite3Database.h"
#include "SQLTableSerializer.h"
#include "SQLTableMetaData.h"
#include "SQLSavedVariables.h"
#include "EngineInterfaces.h"

struct TableKeyType{
  enum KeyType{
    Unknown = 0,
    ArrayIndex,
    Integer,
    String,
  };
};

extern char buf[1000];

string DumpStack2(lua_State *L){

  memset(buf, 0, sizeof(buf));

  int count = lua_gettop(L);

  int pos = sprintf(buf,  "Stack Size = %i: ", count);


  for(int i = 1; i < count+1 ; i++){
    pos += sprintf(buf+pos,  "%i = %s, ", i, lua_typename(L, lua_type(L, i)) );
  }

  return buf;
}

const char* WildCardFields[] = {
  "",
  "?",
  "?,?",
  "?,?",
  "?,?,?",
  "?,?,?,?",
  "?,?,?,?,?",
  "?,?,?,?,?,?",
};

bool ValidFieldName(const string& name){
  
  if(isdigit(name[0]) || name[0] == '_'){
    return false;//cannot start with a digit
  }

  for(int i = 0; i < name.size() ; i++){
    if(!isalnum(name[i]))return false;//Field names cannot a name[i] char
  }

  return true;
}

//"CREATE TABLE IF NOT EXISTS [%s] (tablename TEXT PRIMARY KEY, DataVersion INTEGER, INTEGER, KeyType INTEGER, columncount INTEGER, fieldlist STRING)";

SQLTableSerializer::SQLTableSerializer(lua_State* L, const char* tableName, int infoTableIndex, SQLSavedVariables* sv) 
  : TableName(tableName), IsHashTableChecked(false) , SV(sv), DB(sv->db){

  lua_pushvalue(L, infoTableIndex);
  lua_pushnil(L);
  
  while(lua_next(L, -2) != 0){   
    if(lua_type(L, -1) == LUA_TSTRING){
      string fieldName = lua_tostring(L, -1);
  
      if(!ValidFieldName(fieldName)){
        throw std::runtime_error("SavedVariable field name '"+fieldName+ "' contains invalid characters or starts with a digit or a _");
      }
  
      FieldNames.push_back(std::move(fieldName));
    }
  
    lua_pop(L, 1);
  }
  
  lua_pop(L, 1);

  CompiledFieldListString = CreateFieldListString();
  
  if(DB->TableExists(TableName)){
    MetaData  = SQLTableMetaData::GetMetaDataRecord(sv->db, TableName);

    if(MetaData == NULL){
      LogMessage((TableName+" table exists but there was no table table metadata entry for it").c_str());
    }

   ReadTableSchema();
  }else{
    DBTableKeyType = TableKeyType::Unknown;
  }
}


SQLTableSerializer::~SQLTableSerializer(){
}

static std::unordered_map<int, string> LongWildCardFields;


const char* SQLTableSerializer::GetWildCardString(int fieldCount){
  
  if(fieldCount < sizeof(WildCardFields)/sizeof(char*) ){
    return WildCardFields[fieldCount];
  }

  auto result = LongWildCardFields.find(fieldCount);

  if(result != LongWildCardFields.end()){
    return result->second.c_str();
  }else{
    std::stringstream fieldString;

    for(int i = 0; i < fieldCount ; i++){
      if(i != fieldCount-1){
        fieldString << "?,";
      }else{
        fieldString << '?';
      }
    }

    return (LongWildCardFields[fieldCount] = fieldString.str()).c_str();
  }
}


string SQLTableSerializer::CreateFieldListString(){
  
  std::stringstream fieldString;

  for(int i = 0; i < FieldNames.size() ; i++){
    if(i != FieldNames.size()-1){
      fieldString << FieldNames[i] << ',';
    }else{
      fieldString << FieldNames[i];
    }
  }

  return fieldString.str();
}

inline bool SQLTableSerializer::ReadSqlValue( lua_State *L, SQLitePreparedStatment& statement, int column, int columnType){

  int size;
  const char* data;

  switch(columnType){
    case SQLITE_INTEGER:
    case SQLITE_FLOAT:
      lua_pushnumber(L, statement.GetDouble(column));
    break;

    case SQLITE3_TEXT:
      data = statement.GetString(column, size);
      lua_pushlstring(L, data, size);
    break;

    case SQLITE_BLOB:
      data = (const char*)statement.GetBlob(column, size);

      if(size == 0){     
        lua_pushboolean(L, TRUE);
      }else if(size == 1){
        lua_pushboolean(L, false);
      }else{
        //TODO handling of userdata like vectors
        lua_pushnil(L);
      }
    break;

    case SQLITE_NULL:
      lua_pushnil(L);
   return false;
  }

  return true;
}

void SQLTableSerializer::Load(lua_State* L){
  
  int stackTop = lua_gettop(L);

  //the table doesn't exists in the db so theres no data for us to load
  if(!DB->TableExists(TableName))return;

  _ASSERT(DBTableKeyType != TableKeyType::Unknown);

  if(!PushLuaTable(L, true)){
    return;
  }

  int containerIndex = lua_gettop(L);

  BOOST_FOREACH(const string& name, FieldNames){
    lua_pushstring(L, name.c_str());
  }
  
  try{
    auto Query = CreateSelectStatment();

    if(!Query.Execute()){
      //we got no results back so the table must be empty
      return;
    }

    const int fieldCount = FieldNames.size();

    do{
      //load this entrys key before we push the table on the stack so its in the right position for the rawset at the end 
      ReadSqlValue(L, Query, 1, Query.GetRowValueType(0));

      lua_createtable(L, 0, 10);

      for(int i = 0; i < fieldCount ; i++){
        int columnType = Query.GetRowValueType(i+1);

        if(columnType == SQLITE_NULL)continue;

        //push the field name that we cached in the stack at the start
        lua_pushvalue(L, containerIndex+1+i);
        ReadSqlValue(L, Query, i+1, columnType);
        lua_rawset(L, -3);
      }

      //store our created table in the container table
      lua_rawset(L, containerIndex); 

    }while(Query.Execute());
  
  }catch(exception e){
    lua_settop(L, stackTop);
    throw e;
  }

  lua_settop(L, stackTop);
}

void SQLTableSerializer::CreateTable(const string& tableName){

  const char* fmtString = NULL;

  if(IsHashTable()){
    fmtString = "CREATE TABLE IF NOT EXISTS [%s] (__key TEXT PRIMARY KEY,%s)";
  }else{
    fmtString = "CREATE TABLE IF NOT EXISTS [%s] (__rowid INTEGER PRIMARY KEY, %s)";
  }

  DB->ExecuteSQLNoResultsFmt(fmtString, tableName.c_str(), CompiledFieldListString.c_str());
}

SQLitePreparedStatment SQLTableSerializer::CreateInsertStatment(){

  const char* fmtString = NULL;

  if(IsHashTable()){
    fmtString = "INSERT into [%s] (__key,%s) values (?,%s)";
  }else{
    fmtString = "INSERT into [%s] (__rowid, %s) values (NULL, %s)";
  }

 return DB->CreateStatmentFmt(fmtString, TableName.c_str(), CompiledFieldListString.c_str(), GetWildCardString(FieldNames.size()));
}

SQLitePreparedStatment SQLTableSerializer::CreateSelectStatment(){

  const char* fmtString = NULL;

  if(IsHashTable()){
    fmtString = "SELECT __key,%s FROM [%s]";
  }else{
    fmtString = "SELECT __rowid, %s FROM [%s] ORDER BY __rowid";
  }

  return DB->CreateStatmentFmt(fmtString, CompiledFieldListString.c_str(), TableName.c_str());
}

inline void SQLTableSerializer::BindLuaValue(lua_State *L, SQLitePreparedStatment& InsertStatement, int column, int luaIndex){

  int value;

  switch(lua_type(L, luaIndex)){
    case LUA_TNUMBER:{
      double num = lua_tonumber(L, luaIndex);
      int value = (int)num;

      if(num == value){
        InsertStatement.BindValue(column, value);
      }else{
        InsertStatement.BindValue(column, num);
      }
    }break;

    case LUA_TSTRING:{
      InsertStatement.BindLuaString(column, L, luaIndex);
    }break;

    case LUA_TBOOLEAN:
      value = 0;
      if(lua_toboolean(L, luaIndex) == TRUE){
        //a zero sized blob
        InsertStatement.BindValue(column, &value, 0);
      }else{
        InsertStatement.BindValue(column, &value, 1);
      }
      break;

      //case LUA_TTABLE:
    default:
      InsertStatement.BindNullValue(column);
  }
}

bool SQLTableSerializer::PushLuaTable( lua_State *L, bool createTable){

  SV->ContainerTable.push(L);

  if(lua_type(L, -1) != LUA_TTABLE){
    lua_pop(L, 1);
   return false;
  }

  lua_getfield(L, -1, TableName.c_str());

  if(lua_type(L, -1) != LUA_TTABLE){
    lua_pop(L, 2);

    if(createTable){
      lua_newtable(L);
      //duplicate the table theres so a copy of it still on the stack when we return
      lua_pushvalue(L, -1);

      lua_setfield(L, -3, TableName.c_str());
     return true;
    }

   return false;
  }

  lua_remove(L, -2);

  return true;
}

void SQLTableSerializer::BindHashTableEntry(lua_State* L, SQLitePreparedStatment& query, int tableIndex, int baseFieldIndex, int fieldCount){
  
  BindLuaValue(L, query, 1, tableIndex);

  for(int i = 0; i < fieldCount ; i++){
    lua_pushvalue(L, baseFieldIndex+i);
    lua_gettable(L, tableIndex);

    BindLuaValue(L, query, i+2, -1);

    lua_pop(L, 1);
  }
}

void SQLTableSerializer::Save(lua_State *L){
  
  int startingStackHeight = lua_gettop(L);

  if(!PushLuaTable(L)){
    return;
  }

  lua_pushnil(L);

  //check to see if the table is empty
  if(lua_next(L, -2) == 0){
    return;
  }else{
    //the table has something in it so we can continue clean up the key value left on the stack
    lua_pop(L, 2);
  }

  if(!IsHashTableChecked && !CheckIsHashTable(L)){
    throw std::runtime_error("Unable to determine if the table " + TableName + " is an array or hashtable");
  }

  const int containerIndex = lua_gettop(L);

  bool TableExists = DB->TableExists(TableName);

  if(!TableExists){
    CreateTable(TableName);
  }
  
  auto inserter = CreateInsertStatment();

  SQLiteTransaction trans(*DB);

  if(TableExists){
    DB->ClearTable(TableName);
  }

  const int fieldCount = FieldNames.size();
  const int baseFieldIndex = containerIndex+1;

  BOOST_FOREACH(const string& name, FieldNames){
    lua_pushstring(L, name.c_str());
  }

  lua_pushvalue(L, containerIndex);
  lua_pushnil(L);

  while(lua_next(L, -2) != 0){   
    BindLuaValue(L, inserter, 1, -2);

    for(int i = 0; i < fieldCount ; i++){
     lua_pushvalue(L, baseFieldIndex+i);
     lua_rawget(L, -2);
      
     BindLuaValue(L, inserter, i+2, -1);
     
     lua_pop(L, 1);
    }

    inserter.ExecuteAndReset();

    lua_pop(L, 1);
  }

  lua_pop(L, FieldNames.size()+1);

  trans.Commit();

  lua_settop(L, startingStackHeight);
}


bool SQLTableSerializer::CheckIsHashTable( lua_State *L ){

  lua_pushnil(L);

  if(lua_next(L, -2) == 0){
    lua_pop(L, 2);
   return false;
  }

  bool firstEntry = true;
  bool foundValidKey = false;

  do{
    if(lua_type(L, -2) == LUA_TSTRING){
      CachedIsHashTable = true;
      IsHashTableChecked = true;
    }else if(lua_type(L, -2) == LUA_TNUMBER){
      CachedIsHashTable = false;
      IsHashTableChecked = true;
    }

    firstEntry = false;
    lua_pop(L, 1);
  }while(lua_next(L, -2) != 0 && !IsHashTableChecked);

  lua_pop(L, 2);

  return IsHashTableChecked;
}

void SQLTableSerializer::ReadTableSchema(){

  DBTableKeyType = TableKeyType::Unknown;

  auto tableInfo = DB->CreateStatmentFmt("PRAGMA table_info([%s])", TableName.c_str());

  if(!tableInfo.Execute())return;

  do{
    const char* columnName = tableInfo.GetString(1);

    if(strcmp(columnName, "__key") == 0){
      const char* dataType = tableInfo.GetString(2);

      if(strcmp(dataType, "INTEGER") == 0){
        DBTableKeyType = TableKeyType::Integer;
      }else if(strcmp(dataType, "TEXT") == 0){
      	DBTableKeyType = TableKeyType::String;
      }
    }else if(strcmp(columnName, "__rowid") == 0){
      DBTableKeyType = TableKeyType::ArrayIndex;
     break;
    }
  }while(tableInfo.Step() && DBTableKeyType == TableKeyType::Unknown);
  
  if(DBTableKeyType == TableKeyType::Unknown){
    throw std::runtime_error(string("IsHashTable: table ") + TableName + "'s does not have column named either __key or __rowid");
  }
}

//this should only get called if the table is already created or the lua table has been checked
bool SQLTableSerializer::IsHashTable(){
  if(IsHashTableChecked)return CachedIsHashTable;

  if(DBTableKeyType != TableKeyType::Unknown){
    CachedIsHashTable = DBTableKeyType != TableKeyType::ArrayIndex;
   return CachedIsHashTable;
  }

  if(!DB->TableExists(TableName))throw exception("IsHashTable: Cannot determine if table is a hashtable because the db table has not been created yet");


  //sqlite3_col

  throw exception("IsHashTable has not previously been checked or the check failed");
}

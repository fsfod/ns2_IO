#include "StdAfx.h"
#include "SQLite3Database.h"
#include "SQLTableMetaData.h"
#include "EngineInterfaces.h"

#define FieldOrder TableName, OwnerMod, DataVersion, KeyType, ColumnCount, FieldList

const char* SQLTableMetaData::ColumnList = "TableName, OwnerMod, DataVersion, KeyType, ColumnCount, FieldList";

SQLTableMetaData::SQLTableMetaData(): DataVersion(1), KeyType(0), ColumnCount(0), TableEntryCreateSize(0), Loaded(false){
}

SQLTableMetaData::SQLTableMetaData(SQLiteDatabase* db, const string& tableName): 
  TableName(tableName), DataVersion(1), KeyType(0), ColumnCount(0), TableEntryCreateSize(0), Loaded(false){
  
  DB = db;
}

SQLTableMetaData::SQLTableMetaData(SQLiteDatabase* db, SQLitePreparedStatment& query){
  DB = db;
  InternalLoad(query);
}

SQLTableMetaData::~SQLTableMetaData(){
}

void SQLTableMetaData::Init(SQLiteDatabase* db){
  db->ExecuteSQLNoResults("CREATE TABLE IF NOT EXISTS __TableMetaData (TableName PRIMARY KEY, OwnerMod, DataVersion, KeyType, ColumnCount, FieldList)");
}

SQLTableMetaData* SQLTableMetaData::GetMetaDataRecord(SQLiteDatabase* db, const string& tableName){

  try{
    SQLitePreparedStatment& query = db->GetCachedStatmentFmt("SELECT %s FROM __TableMetaData WHERE TableName = ?", ColumnList);

    query.BindValues(tableName);

    if(!query.Execute()){
      return NULL;
    }

    return new SQLTableMetaData(db, query);
  }catch(SQLiteException e){
    LogMessage(e.what());
   return NULL;
  }
}

void SQLTableMetaData::InternalLoad(SQLitePreparedStatment& query){
  query.ReadValues(TableName, OwnerMod, DataVersion, KeyType, ColumnCount, FieldList);
  query.Reset();
}

bool SQLTableMetaData::Load(){

  SQLitePreparedStatment& query = DB->GetCachedStatmentFmt("SELECT %s FROM __TableMetaData WHERE TableName = ?", ColumnList);

  query.BindValues(TableName);

  if(query.Execute()){
    InternalLoad(query);
    Loaded = true;
  }else{
    Loaded = false;
  }

  return Loaded;
}

void SQLTableMetaData::Save(){
  SQLitePreparedStatment& query = DB->GetCachedStatmentFmt("INSERT or REPLACE into __TableMetaData values(%s) ", ColumnList);

  query.BindValues(TableName, OwnerMod, DataVersion, KeyType, ColumnCount, FieldList);
}
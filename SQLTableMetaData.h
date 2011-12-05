#pragma once

class SQLitePreparedStatment;
class SQLiteDatabase;

class SQLTableMetaData{
public:
  SQLTableMetaData();
  ~SQLTableMetaData();
  
  static SQLTableMetaData* GetMetaDataRecord(SQLiteDatabase* db, const string& tableName);

  static void Init(SQLiteDatabase* db);

  const string& GetTableName(){
    return TableName;
  }

  int GetDataVersion(){
    return DataVersion;
  }

  void SetDataVersion(int newVersion){
    DataVersion = newVersion;
    Save();
  }

  bool Load();
  void Save();

private:
  SQLTableMetaData(SQLiteDatabase* db, const string& tableName);
  SQLTableMetaData(SQLiteDatabase* db, SQLitePreparedStatment& query);
  void InternalLoad(SQLitePreparedStatment& query);

  string TableName;
  string OwnerMod;
  int DataVersion;
  int KeyType;
  int ColumnCount;
  string FieldList;
  int TableEntryCreateSize;

  static const char* ColumnList;
  SQLiteDatabase* DB;
  bool Loaded;
};


#pragma once
#include "SQLitePreparedStatment.h"
#include <unordered_map>


class SQLiteDatabase;
class SQLTableMetaData;

class SQLiteTransaction{
public:
  friend class SQLiteDatabase;

  SQLiteTransaction(SQLiteDatabase& db);

  ~SQLiteTransaction(){
    if(!RolledBack && !Commited){
      Rollback();
    }
  }

  void Rollback();
  void Commit();

  void RollbackTriggered(){
    RolledBack = true;
  }

private:
  SQLiteTransaction() : DB(NULL){
  }

  SQLiteDatabase* DB;
  bool RolledBack;
  bool Commited;
};

struct SQLiteColumnInfo{

public:
  SQLiteColumnInfo(SQLitePreparedStatment& stmt);

  string Name;
  int DataType;
  bool PrimaryKey;
  bool NotNull;
};

class SQLiteDatabase{
public:
  friend class SQLiteTransaction;

  SQLiteDatabase() : DBConnection(NULL){
  }

  SQLiteDatabase(PlatformPath path, int flags = SQLITE_OPEN_READWRITE | SQLITE_OPEN_CREATE){
    Open(path, flags);
  }

  ~SQLiteDatabase(){
    Close();
  }

  void Open(PlatformPath path, int flags = SQLITE_OPEN_READWRITE | SQLITE_OPEN_CREATE);

  void Close();

  void SetAuthLockdown(bool enabled);

  bool TableExists(const string& tableName){
    return TableExists(tableName.c_str());
  }

  bool TableExists(const char* tableName){

    if(!TableExistStmt.Created()){
      TableExistStmt = CreateStatment("SELECT name FROM sqlite_master WHERE type='table' AND name=?");
    }else{
      TableExistStmt.Reset();
    }

    TableExistStmt.BindValue(1, tableName);

    //if we got a result back it means the table exists
    return TableExistStmt.Execute();
  }

  void ClearTable(const string& tableName){
    ExecuteSQLNoResults(("DELETE FROM "+tableName).c_str());
  }
  
  vector<SQLiteColumnInfo> GetTableColumnInfo(const string& tableName);

  void RenameTable(const string& oldName, const string& newName){
    ExecuteSQLNoResultsFmt("ALTER TABLE %s RENAME TO %s", oldName.c_str(), newName.c_str());
  }

  /*
  //Return true if we got any data back
  bool Execute(){
    int result = sqlite3_step(statement);

    if(result == SQLITE_ROW)return true;

    if(result != SQLITE_OK && result != SQLITE_DONE){
      throw SQLiteException(result);
    }

    return false;
  }
*/
  int ExecuteSQLNoResults(const char* sql){
    CheckDatabase();

    char* localError=0;

    int rc = sqlite3_exec(DBConnection, sql, 0, 0, &localError);

    if(rc == SQLITE_OK){
      return sqlite3_changes(DBConnection);
    }else{
      throw SQLiteException(localError, rc);
    }
  }

  int ExecuteSQLNoResults(const char* sql,  char*& localError){
    CheckDatabase();

    return sqlite3_exec(DBConnection, sql, 0, 0, &localError);
  }

  int ExecuteSQLNoResultsFmt(const char* sql, ...);

  void SetBusyTimeout(int timeoutMs){
    int rc = sqlite3_busy_timeout(DBConnection, timeoutMs);

    if(rc != SQLITE_OK)throw SQLiteException(DBConnection, rc);;
  }

  void CheckDatabase(){
    if(DBConnection == NULL)throw SQLiteException("Database is not open");
  }

  SQLitePreparedStatment CreateStatment(const char* sql);
  SQLitePreparedStatment CreateStatmentFmt(const char* sql, ...);

  SQLitePreparedStatment& GetCachedStatment(const char* sql){
    auto result = StatementCache.find((void*)sql);

    if(result != StatementCache.end()){
       result->second.Reset();
      return result->second;
    }

   return (StatementCache[(void*)sql] = CreateStatment(sql));
  }

  SQLitePreparedStatment& GetCachedStatmentFmt(const char* sql, ...);

protected:  
  void RollbackHook(){
    if(CurrentTransaction != NULL){
      CurrentTransaction->RollbackTriggered();
      CurrentTransaction = NULL;
    }
  }

  int Rollback(){
    ExecuteSQLNoResults("rollback transaction");
    CurrentTransaction = NULL;
  }

  void Commit(){
    ExecuteSQLNoResults("commit");
    CurrentTransaction = NULL;
  }

  void BeingTransaction(SQLiteTransaction& trans);
  
  static int DefaultAuthCallback(void* thisPtr, int action, const char* arg1, const char* arg2, const char* arg3, const char* arg4);

protected:
  sqlite3* DBConnection;
  SQLiteTransaction* CurrentTransaction;
  SQLitePreparedStatment TableExistStmt;

  std::unordered_map<void*, SQLitePreparedStatment> StatementCache;
};


#include "StdAfx.h"
#include "SQLite3Database.h"
#include "EngineInterfaces.h"

#include <boost/thread/thread.hpp>
#include <boost/thread/condition_variable.hpp>
#include <concurrent_queue.h>



using Concurrency::concurrent_queue;
using boost::condition_variable;
using boost::unique_lock;
using boost::mutex;
using namespace boost;

SQLitePreparedStatment SQLiteDatabase::CreateStatmentFmt(const char* sql, ...){
  CheckDatabase();

  va_list va;

  va_start(va, sql);
    char* formatedSQL = sqlite3_vmprintf(sql, va);
  va_end(va);

  const char* tail=0;
  sqlite3_stmt* stmt = NULL;

  int rc = sqlite3_prepare_v2((sqlite3*)DBConnection, formatedSQL, -1, &stmt, &tail);
  
  sqlite3_free(formatedSQL);

  if(rc != SQLITE_OK || stmt == NULL){
    throw SQLiteException(DBConnection, rc);
  }

  return SQLitePreparedStatment(stmt);
}

SQLitePreparedStatment SQLiteDatabase::CreateStatment(const char* sql){
  CheckDatabase();

  const char* tail=0;
  sqlite3_stmt* stmt;

  int rc = sqlite3_prepare_v2((sqlite3*)DBConnection, sql, -1, &stmt, &tail);

  if(rc != SQLITE_OK){
    throw SQLiteException(DBConnection, rc);
  }

  return SQLitePreparedStatment(stmt);
}

void SQLiteDatabase::Open( PlatformPath path, int flags){
  int rc = sqlite3_open_v2(path.string().c_str(), (sqlite3**) &DBConnection, flags, NULL);

  if (rc != SQLITE_OK){
    const char* localError = sqlite3_errmsg(DBConnection);
    Close();
    throw SQLiteException(localError, rc);
  }

  rc = sqlite3_extended_result_codes(DBConnection, 1);

  if(rc != SQLITE_OK){
    const char* localError = sqlite3_errmsg(DBConnection);
    Close();
    throw SQLiteException(localError, rc);
  }

  typedef void (*rollbackCallback)(void*);
 
  sqlite3_rollback_hook(DBConnection,  
                        (rollbackCallback)&[](void* thisPtr){
                          reinterpret_cast<SQLiteDatabase*>(thisPtr)->RollbackHook();
                        },
                        this);

  SetBusyTimeout(100);
}

void SQLiteDatabase::SetAuthLockdown(bool enabled){
  
  typedef int (*xAuth)(void*,int,const char*,const char*,const char*,const char*) ;

  int rc = sqlite3_set_authorizer(DBConnection, SQLiteDatabase::DefaultAuthCallback, this);
  
  /*
  (xAuth)&[](void* thisPtr, int action, const char* arg1, const char* arg2, const char* arg3, const char* arg4){
   // reinterpret_cast<SQLiteDatabase*>(thisPtr)->RollbackHook();
  },
  this);
  */
}

int SQLiteDatabase::DefaultAuthCallback(void* thisPtr, int action, const char* arg1, const char* arg2, const char* dbName, const char* arg4){

  switch(action){
    case SQLITE_ATTACH:
    case SQLITE_DETACH:
      return SQLITE_DENY;
    break;

    case SQLITE_PRAGMA:
      if(strcmp(arg1, "table_info") == 0 || strcmp(arg1, "index_info") == 0){
        return SQLITE_OK;
      }
      return SQLITE_DENY;
    break;

    default:
      return SQLITE_OK;
  }
}

void SQLiteDatabase::BeingTransaction(SQLiteTransaction& trans){
  ExecuteSQLNoResults("begin transaction");
  CurrentTransaction = &trans;
}

void SQLiteDatabase::Close(){
  if(DBConnection == NULL)return;

  TableExistStmt.Finalize();

  sqlite3_close(DBConnection);

  DBConnection = NULL;
}

SQLiteColumnInfo::SQLiteColumnInfo(SQLitePreparedStatment& stmt) : 
  Name(stmt.GetString(2)){
  
  int size;
  const char* datatype = stmt.GetString(3, size);

  switch(*datatype){
    case 'T'://TEXT
      DataType = SQLITE3_TEXT;
    break;

    case 'I'://INTEGER
      DataType = SQLITE_INTEGER;
    break;

    case 'N'://NULL
      DataType = SQLITE_NULL;
    break;

    case 'R'://REAL
      DataType = SQLITE_FLOAT;
    break;

    case 'B'://BLOB
      DataType = SQLITE_BLOB;
    break;
  }

  NotNull = stmt.GetInt32(4) != 1;
  PrimaryKey = stmt.GetInt32(6) != 1;
}

vector<SQLiteColumnInfo> SQLiteDatabase::GetTableColumnInfo( const string& tableName){

  auto tableInfo = CreateStatmentFmt("PRAGMA table_info([%s])", tableName.c_str());

  tableInfo.Execute();

  vector<SQLiteColumnInfo> result;

  do{
    result.push_back(SQLiteColumnInfo(tableInfo));
  }while(tableInfo.Step());
  
  return result;
}

int SQLiteDatabase::ExecuteSQLNoResultsFmt(const char* sql, ... ){
  CheckDatabase();

  va_list va;

  va_start(va, sql);
  char* formatedSQL = sqlite3_vmprintf(sql, va);
  va_end(va);

  char* localError=0;

  int rc = sqlite3_exec(DBConnection, formatedSQL, 0, 0, &localError);

  sqlite3_free(formatedSQL);

  if(rc == SQLITE_OK){
    return sqlite3_changes(DBConnection);
  }else{
    throw SQLiteException(localError, rc);
  }
}

SQLitePreparedStatment& SQLiteDatabase::GetCachedStatmentFmt( const char* sql, ... ){
  auto result = StatementCache.find((void*)sql);

  if(result != StatementCache.end()){
    result->second.Reset();
    return result->second;
  }

  va_list va;

  va_start(va, sql);
  char* formatedSQL = sqlite3_vmprintf(sql, va);
  va_end(va);

  try{  
    SQLitePreparedStatment& result = (StatementCache[(void*)sql] = CreateStatmentFmt(formatedSQL));

    sqlite3_free(formatedSQL);

    return result;
  }catch (SQLiteException& e){
    sqlite3_free(formatedSQL);

    throw e;
  }
}

void SQLiteTransaction::Rollback(){
  if(RolledBack)return;

  DB->Commit();

  RolledBack = true;
}

void SQLiteTransaction::Commit(){
  if(Commited)return;

  DB->Commit();

  Commited = true;
}

SQLiteTransaction::SQLiteTransaction( SQLiteDatabase& db ) : DB(&db), RolledBack(false), Commited(false){
  db.BeingTransaction(*this);
}

class AsyncSQLStatment{

public:
  AsyncSQLStatment(const char* sql) : SQL(sql), luaStringRef(0){
  }

  AsyncSQLStatment(lua_State* L, int index){
    luaStringRef = luaL_ref(L, index);
    SQL = lua_tostring(L, index);
  }

  bool Execute(SQLiteDatabase* DB){
    char* error = NULL;
    
    int result = DB->ExecuteSQLNoResults(SQL, error);

    if(error != NULL){
      LogMessage(error);
    }

    Executed = true;

    return luaStringRef != 0;
  }

  void CleanUpLuaRefs(lua_State* L){
    if(luaStringRef != 0){
      lua_unref(L, luaStringRef);
    }
  }

private:
  const char* SQL;
  union{
    int luaStringRef;
    SQLitePreparedStatment* Statment;
  };

  bool Executed;
  bool SQLIsLuaString;
};

class AsyncSQLQueue{
  
public:
  AsyncSQLQueue(SQLiteDatabase* db) : DB(db), Exit(0), StatmentList(){
    boost::thread(&AsyncSQLQueue::ThreadLoop, this);
  }

  void AddStatment(AsyncSQLStatment* stmt){
    StatmentList.push(stmt);
    NewItemEvent.notify_all();
  }

  void ThreadLoop(){
    
    while(true){
      AsyncSQLStatment* item;

      if(!StatmentList.try_pop(item)){
        boost::unique_lock<boost::mutex> lock(DBLock);
        NewItemEvent.wait(lock);

        if(Exit == 1){
          return;
        }
      }

      if(item->Execute(DB)){
        ProcessedStatments.push(item);
      }
    }
  }

  void FreeLuaRefs(lua_State* L){
    AsyncSQLStatment* item;

    while(ProcessedStatments.try_pop(item)){
      item->CleanUpLuaRefs(L);
      delete item;
    }
  }

private:
  AsyncSQLQueue(const AsyncSQLQueue& right) {}

  boost::mutex DBLock;
  condition_variable NewItemEvent;
  concurrent_queue<AsyncSQLStatment*> StatmentList;

  concurrent_queue<AsyncSQLStatment*> ProcessedStatments;

  SQLiteDatabase* DB;
  boost::thread Thread;

  mutable int Exit;
};


#pragma once

extern "C" {
#include "SQLite/sqlite3.h"
}

#include "stdafx.h"
#include "SQLiteException.h"

#include <boost/preprocessor/repetition/enum_binary_params.hpp>
#include <boost/preprocessor/repetition/repeat_from_to.hpp>


#define SQLMaxArgs 12

class SQLitePreparedStatment{

public:
  //friend class SQLiteDatabase;

  SQLitePreparedStatment(sqlite3_stmt* stmt): statement(stmt){
  }

  SQLitePreparedStatment(): statement(NULL){
  }

  SQLitePreparedStatment(SQLitePreparedStatment&& right){
   statement = right.statement;
   right.statement = NULL;
  }

  SQLitePreparedStatment& operator =(SQLitePreparedStatment&& other){ 
    Finalize();

    statement = other.statement;
    other.statement = NULL;

    return *this;
  }

  ~SQLitePreparedStatment(){
    Finalize();
  }

  void Finalize(){
    if(statement == NULL)return;

    sqlite3_finalize(statement);
    statement = NULL;
  }

  void Reset(){
    //ignore the result since its just returns the same value as the last sqlite3_step call
    sqlite3_reset(statement);
  }

  void ExecuteNoResults(){
    int result = sqlite3_step(statement);

    if(result != SQLITE_OK && result != SQLITE_DONE){
      //We shouldn't be getting rows for a no result call
      _ASSERT(result != SQLITE_ROW);
      throw SQLiteException(result);
    }
  }

  //Return true if we got any data back
  bool Execute(){
    int result = sqlite3_step(statement);

    if(result == SQLITE_ROW)return true;

    if(result != SQLITE_OK && result != SQLITE_DONE){
      throw SQLiteException(result);
    }

    return false;
  }

  bool Step(){
    int result = sqlite3_step(statement);

    if(result == SQLITE_ROW)return true;

    if(result != SQLITE_OK && result != SQLITE_DONE){
      throw SQLiteException(result);
    }

    return false;
  }

  #define READVALUE_LINE(z, n, _)  v##n = ReadValue<T##n##>(n);

  #define MakeReadValueN(z, n, _)  template<BOOST_PP_ENUM_PARAMS(n, typename T)> \
    inline void ReadValues( BOOST_PP_ENUM_BINARY_PARAMS(n, T, & v) ){\
    BOOST_PP_REPEAT_ ## z(n, READVALUE_LINE, ~)\
  }\

  BOOST_PP_REPEAT_FROM_TO(1, 13, MakeReadValueN, ~);

  template<typename T> T inline ReadValue(int columnIndex){
    static_assert(false, "Non supported value type");
  }

  template<> int ReadValue<int>(int columnIndex){
    return sqlite3_column_int(statement, columnIndex);
  }

  template<> double ReadValue<double>(int columnIndex){
    return sqlite3_column_double(statement, columnIndex);
  }

  template<> string ReadValue<string>(int columnIndex){
    return (const char*)sqlite3_column_text(statement, columnIndex);
  }

  template<> const char* ReadValue<const char*>(int columnIndex){
    return (const char*)sqlite3_column_text(statement, columnIndex);
  }

  template<> __int64 ReadValue<__int64>(int columnIndex){
    return sqlite3_column_int64(statement, columnIndex);
  }

  int GetRowValueType(int columnIndex){
    return sqlite3_column_type(statement, columnIndex);
  }

  int GetDataSize(int columnIndex){
    return sqlite3_column_bytes(statement, columnIndex);
  }

  double GetDouble(int columnIndex){
    return sqlite3_column_double(statement, columnIndex);
  }

  int GetInt32(int columnIndex){
    return sqlite3_column_int(statement, columnIndex);
  }

  __int64 GetInt64(int columnIndex){
    return sqlite3_column_int64(statement, columnIndex);
  }

  const void* GetBlob(int columnIndex, int& size){
    size = GetDataSize(columnIndex);

    return sqlite3_column_blob(statement, columnIndex);
  }

  const char* GetString(int columnIndex, int& size){
    size = GetDataSize(columnIndex);

    return (const char*)sqlite3_column_text(statement, columnIndex);
  }

  const char* GetString(int columnIndex){
    return (const char*)sqlite3_column_text(statement, columnIndex);
  }

  void ExecuteAndReset(){
    int result = sqlite3_step(statement);

    if(result != SQLITE_OK && result != SQLITE_DONE){
      throw SQLiteException(GetLastErrorMessage(), result);
    }
    
    Reset();
  }

  const char* GetLastErrorMessage(){
    sqlite3* db = GetDbHandle();

    return db != NULL ? sqlite3_errmsg(db) : NULL;
  }

  sqlite3* GetDbHandle(){
    return sqlite3_db_handle(statement);
  }
   
  template<typename T1> void ExecuteNoResults(T1 value1){
    int result = BindValue(0, value1);

    if(result != SQLITE_OK)throw SQLiteException(statement, result);

    if(result != SQLITE_OK && result != SQLITE_DONE){
      throw SQLiteException(statement, result);
    }

    Reset();
  }

  template<typename T> int BindValue(int index, T value){
    static_assert(false, "Non supported bind value");
  }

  inline int BindValue(int index, const float& value){
    return sqlite3_bind_double(statement, index, value);
  }

  inline int BindValue(int index, const double& value){
    return sqlite3_bind_double(statement, index, value);
  }

  inline int BindValue(int index, const int& value){
    return sqlite3_bind_int(statement, index, value);
  }

  inline int BindValue(int index, __int64 value){
    return sqlite3_bind_int64(statement, index, value);
  }

  inline int BindNullValue(int index){
    return sqlite3_bind_null(statement, index);
  }

  inline int BindValue(int index, nullptr_t value){
    return sqlite3_bind_null(statement, index);
  }

  inline int BindValue(int index, const string& s){
    return sqlite3_bind_text(statement, index, s.c_str(), s.size(), SQLITE_TRANSIENT);
  }

  template<size_t size> inline int BindValue(int index, const char (&s)[size]){
    return sqlite3_bind_text(statement, index, s, size, SQLITE_STATIC);
  }

  inline int BindValue(int index, const char* s){
    return sqlite3_bind_text(statement, index, s, -1, SQLITE_STATIC);
  }

  inline int BindLuaString(int index, lua_State* L, int luaIndex){
    size_t size;
    const char* str = lua_tolstring(L, luaIndex, &size);

    return sqlite3_bind_text(statement, index, str, size, SQLITE_STATIC);
  }

  inline int BindValue(int index, const char* s, size_t size){
    return sqlite3_bind_text(statement, index, s, size, SQLITE_STATIC);
  }

  inline int BindValue(int index, const void* s, size_t size){
    return sqlite3_bind_blob(statement, index, s, size, SQLITE_STATIC);
  }

  inline int BindValue(int index, bool value){
    return sqlite3_bind_int(statement, index, value ? 1 : 0);
  }

  #define BINDVALUE_LINE(z, n, _)  result = BindValue(n+1, v##n);\
    if(result != SQLITE_OK) throw SQLiteException(statement, result);\

  #define MakeBindValueN(z, n, _)  template<BOOST_PP_ENUM_PARAMS(n, typename T)> \
  inline void BindValues( BOOST_PP_ENUM_BINARY_PARAMS(n, T,  v) ){\
    int result; \
    BOOST_PP_REPEAT_ ## z(n, BINDVALUE_LINE, ~)\
  }\

  BOOST_PP_REPEAT_FROM_TO(1, 13, MakeBindValueN, ~);
  /*
  template<typename T1, typename T2> void BindValues(T1 value1, T2 value2){
    int result = BindValue(1, value1);

    if(result != SQLITE_OK)return throw SQLiteException(statement, result);

    result = BindValue(2, value2);

    if(result != SQLITE_OK)return throw SQLiteException(statement, result);
  }

  template<typename T1, typename T2, typename T3> void BindValues(T1 value1, T2 value2, T3 value3){
    int result = BindValue(1, value1);

    if(result != SQLITE_OK)return throw SQLiteException(statement, result);

    result = BindValue(2, value2);

    if(result != SQLITE_OK)return throw SQLiteException(statement, result);

    result = BindValue(3, value3);

    if(result != SQLITE_OK)return throw SQLiteException(statement, result);
  }
  */
  template<typename T1, typename T2> inline void BindValuesOffset(int offset, T1 value1, T2 value2){
  int result = BindValue(offset, value1);

  if(result != SQLITE_OK)return throw SQLiteException(statement, result);

  result = BindValue(offset+1, value2);

  if(result != SQLITE_OK)return throw SQLiteException(statement, result);
  }
  
  bool Created(){
    return statement != NULL;
  }

protected:
  sqlite3_stmt* statement;

private:
  SQLitePreparedStatment(const SQLitePreparedStatment& right){
  }
};


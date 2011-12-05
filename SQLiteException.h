#pragma once

#include "SQLite/sqlite3.h"

class SQLiteException : public std::exception{
public:
  SQLiteException() : ErrorCode(0), FullErrorMessage(){
  }

  SQLiteException(int errorCode) : ErrorCode(errorCode), FullErrorMessage(){
  }

  SQLiteException(const char* errorMsg, int errorCode) : ErrorCode(errorCode), FullErrorMessage(errorMsg){
  }

  SQLiteException(const char* errorMsg) : FullErrorMessage(errorMsg), ErrorCode(-1){
  }

  SQLiteException(sqlite3_stmt* statement, int errorCode) : ErrorCode(errorCode), FullErrorMessage(){
    sqlite3* db = sqlite3_db_handle(statement);

    if(db != NULL){
      CaptureErrorMessage(db);
    }
  }

  SQLiteException(sqlite3* db, int errorCode = 0) : ErrorCode(errorCode), FullErrorMessage(){
    CaptureErrorMessage(db);
  }
  
  virtual ~SQLiteException(){
  }

  void CaptureErrorMessage(sqlite3* db){
    const char* msg = sqlite3_errmsg(db);

    if(msg == NULL){
      msg = "internal or unknown error";
    }

    FullErrorMessage = msg;
  }

  

  virtual const char* what()  const;

private:
  int ErrorCode;
  int ExtendedError;
  std::string FullErrorMessage;
};


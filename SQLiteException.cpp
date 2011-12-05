#include "StdAfx.h"
#include "SQLiteException.h"

struct SQLiteError{
  enum SQLiteErrorType : int{
    Ok = SQLITE_OK,
    Error = SQLITE_ERROR,
    Internal = SQLITE_INTERNAL,

    DatabaseNotOpen = 1000,
  };
};

const char* SQLiteException::what()  const{
  //if we were created with error message instead of just an error code
  if(!FullErrorMessage.empty())return FullErrorMessage.c_str();

  switch(ErrorCode){
    case SQLITE_OK:          return "not an error(SQLITE_OK)";
    case SQLITE_ERROR:       return "SQL logic error or missing database";
    case SQLITE_INTERNAL:    return "internal error";
    case SQLITE_PERM:        return "access permission denied";
    case SQLITE_ABORT:       return "callback requested query abort";
    case SQLITE_BUSY:        return "database is locked";
    case SQLITE_LOCKED:      return "database table is locked";
    case SQLITE_NOMEM:       return "out of memory";
    case SQLITE_READONLY:    return "attempt to write a readonly database";
    case SQLITE_INTERRUPT:   return "interrupted";
    case SQLITE_IOERR:       return "disk I/O error";
    case SQLITE_CORRUPT:     return "database disk image is malformed";
    case SQLITE_NOTFOUND:    return "unknown operation";
    case SQLITE_FULL:        return "database or disk is full";
    case SQLITE_CANTOPEN:    return "unable to open database file";
    case SQLITE_PROTOCOL:    return "locking protocol";
    case SQLITE_EMPTY:       return "table contains no data";
    case SQLITE_SCHEMA:      return "database schema has changed";
    case SQLITE_TOOBIG:      return "string or blob too big";
    case SQLITE_CONSTRAINT:  return "constraint failed";
    case SQLITE_MISMATCH:    return "datatype mismatch";
    case SQLITE_MISUSE:      return "library routine called out of sequence";
    case SQLITE_NOLFS:       return "large file support is disabled";
    case SQLITE_AUTH:        return "authorization denied";
    case SQLITE_FORMAT:      return "auxiliary database format error";
    case SQLITE_RANGE:       return "bind or column index out of range";
    case SQLITE_NOTADB:      return "file is encrypted or is not a database";

  default: return "Unknown error";
  }
}

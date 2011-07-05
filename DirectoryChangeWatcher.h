#pragma once

#include "VC2005Compat.h"
#include <boost/thread/mutex.hpp>

__declspec(noinline) static void LogWin32Error(int error , const char* owner = NULL, const char* errorMsg = NULL){
  wchar_t* lpMsgBuf = 0;

  DWORD retval = ::FormatMessageW(FORMAT_MESSAGE_ALLOCATE_BUFFER | FORMAT_MESSAGE_FROM_SYSTEM | FORMAT_MESSAGE_IGNORE_INSERTS,
                                  NULL, error, MAKELANGID(LANG_NEUTRAL, SUBLANG_DEFAULT), (LPWSTR)&lpMsgBuf, 0, NULL);

  if(retval == 0){

  }
}

class DirectoryChangeWatcher{

public:
  DirectoryChangeWatcher(PlatformPath path);

  ~DirectoryChangeWatcher();

  void CheckForChanges(VC05Vector<VC05string>& changed);

  void ReadChanges(DWORD BytesRead);
  bool BeginReadChanges();

private:
  //put our OVERLAPPED first so we can take advantage 
  OVERLAPPED OL;
  char Buffer[4000];
  
  boost::mutex MapLock;

  std::map<wstring, DWORD> LastChanges;
  HANDLE DirectoryHandle;

  static const int NotifyFilter = FILE_NOTIFY_CHANGE_FILE_NAME|FILE_NOTIFY_CHANGE_SIZE| FILE_NOTIFY_CHANGE_LAST_WRITE;
  volatile int HasChanges;
  friend class WatcherIOCompletion;
};

class WatcherIOCompletion{

public:
  static void RegisterWatcher(DirectoryChangeWatcher* watcher){
    /*
    HANDLE result = CreateIoCompletionPort(watcher->DirectoryHandle, CompletionPort, NULL, 1);
    
    if(result == NULL){
      LogWin32Error(GetLastError());
      throw exception("Failed to register watcher to completion port");
    }
    */

    if(BindIoCompletionCallback(watcher->DirectoryHandle, &ProcessResult, 0) == 0){
      LogWin32Error(GetLastError());
      throw exception("Failed to bind completion to callback function");
    }
  }

  static void StartUp(HANDLE completionPort){
   

    CompletionPort = completionPort;
  }

  static void WINAPI ProcessResult(DWORD ErrorCode, DWORD BytesRead, LPOVERLAPPED lpOverlapped){
    if(ErrorCode == 0){
      DirectoryChangeWatcher* watcher = reinterpret_cast<DirectoryChangeWatcher*>(lpOverlapped);
      
      watcher->ReadChanges(BytesRead);
 
      watcher->BeginReadChanges();

    }else if(ErrorCode == ERROR_OPERATION_ABORTED){
      return;
    }else{
      
    }
  }
private:
  static HANDLE CompletionPort;
};
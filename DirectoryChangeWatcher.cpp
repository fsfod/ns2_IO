#include "StdAfx.h"
#include "DirectoryChangeWatcher.h"

DirectoryChangeWatcher::DirectoryChangeWatcher(PlatformPath path){
  memset(&OL, 0, sizeof(OL));

  //we include FILE_SHARE_DELETE so its possible to move or rename or delete files in the root of the directory
  DirectoryHandle =  CreateFile(path.c_str(), FILE_LIST_DIRECTORY, FILE_SHARE_READ| FILE_SHARE_WRITE| FILE_SHARE_DELETE,
                                NULL, OPEN_EXISTING, FILE_FLAG_BACKUP_SEMANTICS|FILE_FLAG_OVERLAPPED, NULL);

  if(DirectoryHandle == INVALID_HANDLE_VALUE){
    int error = GetLastError();

    throw exception("Failed to open directory to read changes");
  }

  if(!BeginReadChanges()){
    int error = GetLastError();

    throw exception("Encountered error while trying to start reading directory changes");
  }

  WatcherIOCompletion::RegisterWatcher(this);
}


DirectoryChangeWatcher::~DirectoryChangeWatcher(){

  CancelIo(DirectoryHandle);

  //make sure the kernel has stopped using our overlapped object
  DWORD dummy;
  GetOverlappedResult(DirectoryHandle, &OL, &dummy, true);

  CloseHandle(DirectoryHandle);
}


void DirectoryChangeWatcher::CheckForChanges(VC05Vector<VC05string>& changed){

  //we don't really care if we get a stale value here 
  if(LastChanges.empty())return;

  boost::mutex::scoped_lock lock(MapLock);

  DWORD WindowStart = GetTickCount()-500;

  for(auto it = LastChanges.begin(); it != LastChanges.end();){
    if(it->second < WindowStart){
      string path = WStringToUTF8STLString(it->first);
      boost::to_lower(path);

      changed.push_back(VC05string(path));

      LastChanges.erase(it++);
    }else{
      ++it;
    }
  }
}

bool DirectoryChangeWatcher::BeginReadChanges(){

  if(ReadDirectoryChangesW(DirectoryHandle, Buffer, sizeof(Buffer), true, NotifyFilter, NULL, &OL, NULL) == 0){
    int error = GetLastError();

    return false;
  }

  return true;
}

void DirectoryChangeWatcher::ReadChanges(DWORD BytesRead){
  /*
  DWORD ResultSize = 0;

  int result = GetOverlappedResult(DirectoryHandle, &OL, &ResultSize, false);

  if(result == 0){
    //we should really decide not to try reading changes again if we get certain errors
    BeginReadChanges();

    return;
  }
  */

  boost::mutex::scoped_lock lock(MapLock);

  FILE_NOTIFY_INFORMATION* current = reinterpret_cast<FILE_NOTIFY_INFORMATION*>(&Buffer);
  void* end = (&Buffer)+BytesRead;

  while(current < end){
    int namesize = current->FileNameLength/sizeof(wchar_t);      
    
    LastChanges[wstring(current->FileName, namesize)] = GetTickCount();

    if(current->NextEntryOffset == 0)break;

    current =  (FILE_NOTIFY_INFORMATION*)( ((char*)current) + current->NextEntryOffset);
  }

  //BeginReadChanges();
}


HANDLE WatcherIOCompletion::CompletionPort = NULL;

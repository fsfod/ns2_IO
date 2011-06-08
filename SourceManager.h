#pragma once
#include "Stdafx.h"
#include "EngineInterfaces.h"


class Archive;
class ResourceOverrider;

class ChangeWatcher{

public:
  ChangeWatcher(PlatformPath path){
    //ChangeHandle = FindFirstChangeNotification(path.c_str(), true, FILE_NOTIFY_CHANGE_FILE_NAME|FILE_NOTIFY_CHANGE_SIZE| FILE_NOTIFY_CHANGE_LAST_WRITE);

   DirectoryHandle=  CreateFile(path.c_str(), FILE_LIST_DIRECTORY, FILE_SHARE_READ| FILE_SHARE_WRITE| FILE_SHARE_DELETE,
                                NULL, OPEN_EXISTING, FILE_FLAG_BACKUP_SEMANTICS, NULL);
  
    if(DirectoryHandle == INVALID_HANDLE_VALUE)throw exception();
  }

  ~ChangeWatcher(){
    CloseHandle(DirectoryHandle);
    DirectoryHandle = INVALID_HANDLE_VALUE;
  }

  void CheckForChanges(VC05Vector<VC05string>& changed){
    ReadChanges();

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

  void ReadChanges(){
    char Buffer[1024];
    DWORD ResultSize = 0;
     
    int result = ReadDirectoryChangesW(DirectoryHandle, Buffer, sizeof(Buffer), true, filter, &ResultSize, NULL, NULL);

    FILE_NOTIFY_INFORMATION* current = reinterpret_cast<FILE_NOTIFY_INFORMATION*>(&Buffer);

    while(true){      
      LastChanges[wstring(current->FileName, current->FileNameLength)] = GetTickCount();

      if(current->NextEntryOffset != 0){
        current =  reinterpret_cast<FILE_NOTIFY_INFORMATION*>(reinterpret_cast<char*>(current)+current->NextEntryOffset);
      }
    }
  }

private:
  static const int filter = FILE_NOTIFY_CHANGE_FILE_NAME|FILE_NOTIFY_CHANGE_SIZE| FILE_NOTIFY_CHANGE_LAST_WRITE;
  std::map<wstring, DWORD> LastChanges;
  HANDLE DirectoryHandle;
};
class FileSource;


class EndSource: public M4::FileSource{

public:
  EndSource() :ExtraSources(){
  }

  void AddSource(::FileSource* source){
    if(std::find(ExtraSources.begin(), ExtraSources.end(), source) != ExtraSources.end()){
      throw exception("That source has already been added");
    }

    ExtraSources.push_back(source);
  }

  void RemoveAllSources(){
    ExtraSources.clear();
  }

  virtual M4::File* OpenFile( const VC05string& path, bool something );

  virtual VC05string GetDescription(){
    return VC05string("EndSource");
  }

  virtual bool GetFileExists(const VC05string& path);

  virtual void GetChangedFiles(VC05Vector<VC05string>& ChangeFiles ) {

  }

  virtual int64_t GetFileModificationTime( const VC05string& path );

private:
  vector<::FileSource*> ExtraSources;
};

class SourceManager{

public: 
  static void SetupFilesystemMounting();

  static Archive* OpenArchive(const PlatformPath& path);
  static void ArchiveClosed(Archive* arch);

  static void CloseAllArchives();

  static void MountSource(::FileSource* source){
    ExtraSources->AddSource(source);
  }

private:
  static std::map<PlatformPath::string_type, Archive*> OpenArchives;

  static std::map<string, int> GroupNameToId;

  static ResourceOverrider* Overrider;
  static EndSource* ExtraSources;
};


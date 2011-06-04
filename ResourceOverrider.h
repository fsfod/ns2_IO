#pragma once

#include "EngineInterfaces.h"
#include "Archive.h"

class OverrideFile;

namespace boost{
 namespace iostreams{
  class mapped_file;
 }
}

class OverrideEntry{

public:
  OverrideEntry():Data(), ContainingArchive(NULL), FileIndex(-1), OverrideGroupTag(0){}

  OverrideEntry(Archive* owner, int index, bool PreloadFile, int groupTag = 0) : OverrideGroupTag(groupTag){
    FileIndex = index;
    ContainingArchive = owner;

    if(PreloadFile){
      Data.reset(ContainingArchive->ExtractFileToMemory(FileIndex), [](void* data){delete data;} );
    }
  }

  OverrideEntry(PlatformPath OverriderPath, bool PreloadFile) :Data(), FileIndex(-1), ContainingArchive(NULL), OverrideGroupTag(){
    OverriderFile = OverriderPath;
  }

  OverrideEntry(const OverrideEntry& other) :Data(other.Data), ContainingArchive(other.ContainingArchive), FileIndex(other.FileIndex), OverrideGroupTag(0){ 
    if(ContainingArchive != NULL){
      ContainingArchive->FileUnMounted(FileIndex);
    }
  }

  //OverrideEntry(OverrideEntry&& other): Data(), ContainingArchive(NULL){ 
  //  *this = std::move(other);
  //}

  OverrideEntry& operator=(const OverrideEntry& other){
    if(other.ContainingArchive != NULL){
      ContainingArchive->FileMounted(other.FileIndex);
    }

    if(ContainingArchive != NULL){
      ContainingArchive->FileUnMounted(FileIndex);
    }
    
    ContainingArchive = other.ContainingArchive;
    FileIndex = other.FileIndex;
    Data = other.Data;

    OverriderFile = other.OverriderFile;

    return *this;
  }

  /*
  OverrideEntry& operator=(OverrideEntry&& other){
    if(this != &other) {
      if(ContainingArchive != NULL){
        ContainingArchive->FileUnMounted(FileIndex);
      }

      ContainingArchive = other.ContainingArchive; 
      other.ContainingArchive = NULL;

      FileIndex = other.FileIndex;
      other.FileIndex = -1;

      Data = other.Data;
    }

    return *this;
  }
  */

  ~OverrideEntry(){
  }

  //we can't just return ourselves because the engine calls the destructor on the file object
  M4::File* MakeEngineFileObj();

  bool IsInArchive() const{
    return FileIndex != -1;
  }

  bool FromArchive(Archive* source){
    return IsInArchive() && source == ContainingArchive;
  }

  int GetGroupId(){
    return OverrideGroupTag;
  }

  void* Lock(uint32_t start, uint32_t size);
  void* Lock();
  void UnLock();
  uint32_t GetLength();

private:
  Archive* ContainingArchive;
  int OverrideGroupTag;
  int FileIndex;
  //this pointer is only set to something if
  shared_ptr<void> Data;
  
  boost::iostreams::mapped_file* MappedFile;
  PlatformPath OverriderFile;

 class OverrideFile :public M4::File{
  public:
    OverrideFile(OverrideEntry* owner): Data(){
      Owner = owner;
      Data.swap(Owner->Data);
    }

    ~OverrideFile(){
      UnLock();
    }

    virtual void* LockRange(uint32_t start, uint32_t size){
      return Owner->Lock(start, size);
    }

    virtual void* Lock(){
      return Owner->Lock();
    }

    void UnLock(){
      Owner->UnLock();
      Data.reset();
    }

    uint32_t GetLength(){
      return Owner->GetLength();
    }

    OverrideEntry* Owner;
    shared_ptr<void> Data;
  };
};


class ResourceOverrider : public M4::FileSource{
public:
  ResourceOverrider(): MapArchive(){
  }

  void MountFile(const string& path, Archive* container, int FileIndex, bool TriggerModifed = false){
    container->FileMounted(FileIndex);

    FileOverrides[path] = OverrideEntry(container, FileIndex, true);

    if(TriggerModifed)ChangedFiles.push_back(VC05string(path));
  }

  void MountFile(const string& path, PlatformPath OverriderPath, bool TriggerModifed = false){
    FileOverrides[path] = OverrideEntry(OverriderPath, true);
  }

  //path has tobe normalized before being passed to this
  bool IsFileOverriden(std::string path){
    return FileOverrides.find(path) != FileOverrides.end();
  }

  bool RemoveFileOverride(const string& path, bool TriggerModifed = true){

    auto result = FileOverrides.find(path);
    
    if(result != FileOverrides.end()){
        FileOverrides.erase(result);

        if(TriggerModifed)ChangedFiles.push_back(VC05string(path));
      return true;
    }

    return false;
  }

  virtual ~ResourceOverrider(){
    OutputDebugStringA("~MountedArchiveFilesSource");
  }

  void MountMapArchive(Archive* archive);

  void UnmountMapArchive(){
    MapArchive = NULL;
  }

  M4::File* OpenFile(const VC05string& path, bool something);

  bool GetFileExists(const VC05string& path);

  VC05string GetDescription(){
    return VC05string();
  }

  int UnmountGroup(int GroupId);
  void UnmountFilesFromArchive(Archive* source);
 
  void MountFilesList(Archive* source, const vector<pair<string, int>>& FileList, bool triggerChange){
    typedef pair<const string, int> EntryType;

    BOOST_FOREACH(const EntryType& file, FileList){
      MountFile(file.first, source, file.second, triggerChange);
    }
  }

  void GetChangedFiles(std::vector<VC05string>& ChangeFiles);

private:
  Archive* MapArchive;

  vector<VC05string> ChangedFiles;

  std::map<std::string, boost::shared_ptr<FileSource*>> ModFolders;

  std::map<std::string, OverrideEntry> FileOverrides;
};


typedef VC05Vector<std::pair<int, M4::FileSource*>> FileListType;

namespace M4{
  class FileSystem{
   public:
    virtual ~FileSystem(){}
  
    void AddSource(M4::FileSource* Source, unsigned int flags);
    void RemoveSource(M4::FileSource *Source);
  
    FileListType FileList;
  };
}
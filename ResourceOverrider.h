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
  OverrideEntry():Data(nullptr), ContainingArchive(nullptr), FileIndex(-1){}

  OverrideEntry(Archive* owner, int index, bool PreloadFile){
    FileIndex = index;
    ContainingArchive = owner;

    
    if(PreloadFile){
      Data = ContainingArchive->ExtractFileToMemory(FileIndex);
    }else{
      Data = nullptr;
    }
  }

  OverrideEntry(PlatformPath OverriderPath, bool PreloadFile) :Data(nullptr), FileIndex(-1){
    OverriderFile = OverriderPath;
  }

  OverrideEntry(const OverrideEntry& other) :Data(nullptr), ContainingArchive(other.ContainingArchive), FileIndex(other.FileIndex){ 
    Data = NULL;
  }

  OverrideEntry(OverrideEntry&& other){ 
    Data = nullptr;
    *this = std::move(other);
  }

  OverrideEntry& operator=(OverrideEntry&&other){
    ContainingArchive = other.ContainingArchive; 
    FileIndex = other.FileIndex;
    
    if(Data != nullptr)delete Data;
    Data = other.Data;

    if(this != &other) {
      other.Data = nullptr;
    }

    return *this;
  }

  ~OverrideEntry(){
    delete Data;
  }

  //we can't just return ourselves because the engine calls the destructor on the file object
  EngineFile* MakeEngineFileObj(){
    return new OverrideFile(this);
  }

  bool IsInArchive() const{
    return FileIndex != -1;
  }

  void* Lock(uint32_t start, uint32_t size);
  void* Lock();
  void UnLock();
  uint32_t GetLength();

private:
  int FileIndex;
  void* Data;
  
  Archive* ContainingArchive;
  boost::iostreams::mapped_file* MappedFile;
  PlatformPath OverriderFile;

 class OverrideFile :public EngineFile{
  public:
    OverrideFile(OverrideEntry* owner){
      Owner = owner;
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
    }

    uint32_t GetLength(){
      return Owner->GetLength();
    }

    OverrideEntry* Owner;
  };
};


class ResourceOverrider : public EngineFileSource{
public:
  ResourceOverrider(): MapArchive(){
  }

  void AddFileOverride(std::string path, Archive* container, int FileIndex){
    container->FileMounted(FileIndex);

    FileOverrides[path] = OverrideEntry(container, FileIndex, true);
  }

  //path has tobe normalized before being passed to this
  bool IsFileOverriden(std::string path){
    return FileOverrides.find(path) != FileOverrides.end();
  }

  void AddFileOverride(std::string path, PlatformPath OverriderPath){
    FileOverrides[path] = OverrideEntry(OverriderPath, true);
  }

  virtual ~ResourceOverrider(){
    OutputDebugStringA("~MountedArchiveFilesSource");
  }

  void MountMapArchive(Archive* archive);

  void UnmountMapArchive(){
    MapArchive = NULL;
  }

  EngineFile* OpenFile(const VC05string& path, bool something);

  bool GetFileExists(const VC05string& path);

  VC05string GetDescription(){
    return VC05string();
  }

private:
  EngineFileSource* ModSource;
  EngineFileSource* MainNS2Source;

  Archive* MapArchive;

  std::map<std::string, boost::shared_ptr<FileSource*>> ModFolders;

  std::map<std::string, OverrideEntry> FileOverrides;
};


typedef VC05Vector<std::pair<int, EngineFileSource*>> FileListType;

class M4FileSystem{

public:
  virtual ~M4FileSystem(){}

  void AddSource(EngineFileSource* Source, int flags){}

  FileListType FileList;
};
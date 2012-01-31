#pragma once
#include "StdAfx.h"
#include "Archive.h"

class DirectoryFileSource;
class FileSource;

namespace M4{
  class File;
};

class MountedFile{

public:
  MountedFile():Data(), ContainingSource(NULL), FileIndex(-1), OverrideGroupTag(0){}

  MountedFile(Archive* owner, int index, int groupTag = 0) : OverrideGroupTag(groupTag), Data(){
    FileIndex = index;
    ContainingSource = (FileSource*)owner;

    AddItemRef();
  }

  MountedFile(DirectoryFileSource* source, PlatformPath OverriderPath, bool PreloadFile) :Data(), FileIndex(-1), ContainingSource((FileSource*)source), OverrideGroupTag(){
    OverriderFile = OverriderPath;
  }

  MountedFile(const MountedFile& other) :Data(other.Data), ContainingSource(other.ContainingSource), FileIndex(other.FileIndex), OverrideGroupTag(0){ 
    AddItemRef();
  }
  ~MountedFile();

  void AddItemRef();
  void ReleaseItemRef();

  void SetPreloadData(shared_ptr<void> preloadData){
    Data = preloadData;
  }

  MountedFile& operator=(const MountedFile& other){
    if(&other == this){
      return *this;
    }

    ReleaseItemRef();

    ContainingSource = other.ContainingSource;
    FileIndex = other.FileIndex;
    Data = other.Data;

    OverriderFile = other.OverriderFile;

    AddItemRef();

    return *this;
  }

  //we can't just return ourselves because the engine deletes the file object
  M4::File* MakeEngineFileObj(M4::Allocator* alc);

  bool IsInArchive() const{
    return FileIndex != -1;
  }

  bool FromSource(FileSource* source){
    return source == ContainingSource;
  }

  FileSource* GetSource(){
    return ContainingSource;
  }

  int GetGroupId(){
    return OverrideGroupTag;
  }

  uint32_t GetLength();

  int64_t GetFileModifiedTime();

private:
  FileSource* ContainingSource;
  int FileIndex;
  int OverrideGroupTag;
  //this pointer is only set to something if data was preloaded
  shared_ptr<void> Data;
  PlatformPath OverriderFile;
};
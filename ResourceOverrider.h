#pragma once

#include "EngineInterfaces.h"
#include "Archive.h"
#include "SourceCollection.h"
#include "MountedFile.h"

class OverrideFile;

class ResourceOverrider : public SourceCollection{

public:
  ResourceOverrider(): MapArchive(NULL), FileOverrides(), ChangedFiles(){
  }

  void Destroy(int options){
    this->~ResourceOverrider();
  }

  void MountFile(const string& path, MountedFile& file, bool TriggerModifed){
    FileOverrides[path] = file;

    if(TriggerModifed)ChangedFiles.push_back(VC05string(path));
  }

  void MountFile(const string& path, Archive* container, int FileIndex, bool TriggerModifed){
    container->FileMounted(FileIndex);

    FileOverrides[path] = MountedFile(container, FileIndex, true);

    if(TriggerModifed)ChangedFiles.push_back(VC05string(path));
  }

 // void MountFile2(const string& path, PlatformPath OverriderPath, bool TriggerModifed = false){
 //   FileOverrides[path] = MountedFile(OverriderPath, true);
 // }

  //path has tobe normalized before being passed to this
  bool IsFileMounted(std::string path){
    return FileOverrides.find(path) != FileOverrides.end();
  }

  bool UnmountFile(const string& path, bool TriggerModifed = true){

    auto result = FileOverrides.find(path);
    
    if(result != FileOverrides.end()){
        FileOverrides.erase(result);

        if(TriggerModifed)ChangedFiles.push_back(VC05string(path));
      return true;
    }

    return false;
  }

  void MountMapArchive(Archive* archive);
  void UnmountMapArchive();

  M4::File* OpenFile(M4::Allocator* alc, const char* path, bool something);
  bool GetFileExists(const char* path);

  VC05string GetDescription(){
    return VC05string("Overrider Source");
  }

  int UnmountGroup(int GroupId);
  int UnmountFilesFromSource(::FileSource* source);
 
  void GetFilesFromSource(::FileSource* source, vector<pair<const std::string, MountedFile>*>& FoundFiles);

  void MountFilesList(Archive* source, const vector<pair<string, int>>& FileList, bool triggerChange){
    typedef pair<const string, int> EntryType;

    BOOST_FOREACH(const EntryType& file, FileList){
      MountFile(file.first, source, file.second, triggerChange);
    }
  }

  int64_t GetFileModificationTime(const char* path);

  //void GetChangedFiles(VC05Vector<VC05string>& ChangeFiles);

private:
  Archive* MapArchive;

  vector<VC05string> ChangedFiles;
  std::unordered_map<std::string, MountedFile> FileOverrides;
};
#include "StdAfx.h"
#include "ResourceOverrider.h"
#include "StringUtil.h"
#include "SevenZip.h"
#include "NSRootDir.h"

//using namespace std;

M4::File* ResourceOverrider::OpenFile(M4::Allocator* alc, const VC05string& path, bool something){

  string NormPath = NormalizedPath(path.c_str(), path.size());

  auto result = FileOverrides.find(NormPath);
  
  if(result == FileOverrides.end()){
    return GetEngineFile(NormPath, alc);
  }
  
  return result->second.MakeEngineFileObj();
}

bool ResourceOverrider::GetFileExists(const VC05string& path){
  string NormPath = NormalizedPath(path.c_str(), path.size());

  return FileOverrides.find(NormPath) != FileOverrides.end() || FileExists(NormPath);
}

int ResourceOverrider::UnmountGroup(int GroupId){
  int count = 0;

  for(auto it = FileOverrides.begin(); it != FileOverrides.end(); ) {
    if(it->second.GetGroupId() == GroupId) {
      ChangedFiles.push_back(VC05string(it->first));
      FileOverrides.erase(it++);
      count++;
    }else{
      ++it;
    }
  }

  return count;
}

int ResourceOverrider::UnmountFilesFromSource(::FileSource* source){

  int count = 0;

  for(auto it = FileOverrides.begin(); it != FileOverrides.end(); ) {
    if(it->second.FromSource(source)) {
      ChangedFiles.push_back(VC05string(it->first));

      FileOverrides.erase(it++);
      count++;
    }else{
      ++it;
    }
  }

  return count;
}

int64_t ResourceOverrider::GetFileModificationTime(const VC05string& path){
  string NormPath = NormalizedPath(path.c_str(), path.size());

  auto result = FileOverrides.find(NormPath);

  if(result == FileOverrides.end())return GetFileModifiedTime(NormPath);

  return result->second.GetFileModifiedTime();
}

void ResourceOverrider::UnmountMapArchive(){
  if(MapArchive != NULL){
    RemoveSource(MapArchive);
    MapArchive->FileUnMounted(-1);
  }

  MapArchive = NULL;
}

void ResourceOverrider::MountMapArchive( Archive* archive ){
  archive->FileMounted(-1);

  if(MapArchive != NULL)UnmountMapArchive();
  MapArchive = archive;
  AddSource(MapArchive);
}

void ResourceOverrider::GetFilesFromSource(::FileSource* source, vector<pair<const std::string, MountedFile>*>& FoundFiles ){

  for(auto it = FileOverrides.begin(); it != FileOverrides.end(); ) {
    ::FileSource* entrySource = it->second.GetSource();

    if(entrySource == source) {
      FoundFiles.push_back(&*it);
    }
  }
}


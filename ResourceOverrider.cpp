#include "StdAfx.h"
#include "ResourceOverrider.h"
#include "StringUtil.h"
#include "C7ZipLibrary.h"
#include "NSRootDir.h"
#include <boost/iostreams/device/mapped_file.hpp>

//using namespace std;
using namespace boost::iostreams;

extern C7ZipLibrary* SevenZip;

M4::File* ResourceOverrider::OpenFile(const VC05string& path, bool something){

  string NormPath = NormalizedPath(path.c_str(), path.size());

  if(MapArchive != NULL){ 
    int fileIndex = MapArchive->GetFileIndex(NormPath);

    if(fileIndex != -1){
      return MapArchive->GetEngineFile(fileIndex);
    }
  }

  auto result = FileOverrides.find(NormPath);
  if(result == FileOverrides.end())return NULL;
  
  return result->second.MakeEngineFileObj();
}

bool ResourceOverrider::GetFileExists(const VC05string& path) {

  string NormPath = NormalizedPath(path.c_str(), path.size());

  if(MapArchive != NULL && MapArchive->GetFileIndex(NormPath) != -1){ 
    return true;
  }

  return FileOverrides.find(NormPath) != FileOverrides.end();
}

void ResourceOverrider::MountMapArchive(Archive* archive){
  archive->FileMounted(-1);
  MapArchive = archive;
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

void ResourceOverrider::UnmountFilesFromArchive(Archive* source){

  for(auto it = FileOverrides.begin(); it != FileOverrides.end(); ) {
    if(it->second.FromArchive(source)) {
      ChangedFiles.push_back(VC05string(it->first));

      FileOverrides.erase(it++);
    }else{
      ++it;
    }
  }
}

int64_t ResourceOverrider::GetFileModificationTime(const VC05string& path){
  string NormPath = NormalizedPath(path.c_str(), path.size());

  if(MapArchive != NULL){ 
    int fileIndex = MapArchive->GetFileIndex(NormPath);

    if(fileIndex != -1){
      return MapArchive->GetModifiedTime(fileIndex);
    }
  }

  auto result = FileOverrides.find(NormPath);

  if(result == FileOverrides.end())return 0;

  return result->second.GetFileModifiedTime();
}

void ResourceOverrider::GetChangedFiles(std::vector<VC05string>& ChangedFileList){

  if(ChangedFiles.size() != 0)return;

  BOOST_FOREACH(VC05string& file, ChangedFiles){
    ChangedFileList.push_back(std::move(file));
  }

  ChangedFiles.clear();
}

void* OverrideEntry::Lock(uint32_t start, uint32_t size){
  //sadly boosts mapped_file doesn't give us a way to unmap a view and map another at a different offset so just map the whole file
  return ((char*)Lock())+start;
}

void* OverrideEntry::Lock(){

  if(Data == NULL){
    try{
      if(IsInArchive()){
        Data.reset((char*)ContainingArchive->ExtractFileToMemory(FileIndex));
      }else{
        MappedFile = new mapped_file(OverriderFile.wstring());

        Data.reset((char*)MappedFile->data());
      }
    }catch(exception e){
      return NULL;
    }
  }

  return Data.get();
}

void OverrideEntry::UnLock(){
  if(IsInArchive()){
    Data.reset();
  }else{
    if(MappedFile != nullptr)delete MappedFile;
    MappedFile = nullptr;
  }
}

uint32_t OverrideEntry::GetLength(){
  if(IsInArchive()){
    return ContainingArchive->GetFileSize(FileIndex);
  }else{
    return boostfs::file_size(OverriderFile);
  }
}

M4::File* OverrideEntry::MakeEngineFileObj(){
  if(IsInArchive()){
    return ContainingArchive->GetEngineFile(FileIndex);
  }else{
    return new DirEngineFile(OverriderFile);
  }
}

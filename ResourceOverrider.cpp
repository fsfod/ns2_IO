#include "StdAfx.h"
#include "ResourceOverrider.h"
#include "StringUtil.h"
#include "C7ZipLibrary.h"
#include <boost/iostreams/device/mapped_file.hpp>

//using namespace std;
using namespace boost::iostreams;

extern C7ZipLibrary* SevenZip;

EngineFile* ResourceOverrider::OpenFile(const VC05string& path, bool something){

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

  if(MapArchive != NULL){ 
   string NormPath = NormalizedPath(path.c_str(), path.size());

    int fileIndex = MapArchive->GetFileIndex(NormPath);

    if(fileIndex != -1){
      return MapArchive->GetEngineFile(fileIndex);
    }
  }

  return false;
}

void ResourceOverrider::MountMapArchive( Archive* archive )
{
  archive->FileMounted(-1);
  MapArchive = archive;
}

void* OverrideEntry::Lock(uint32_t start, uint32_t size){
  //sadly boosts mapped_file doesn't give us a way to unmap a view and map another at a different offset so just map the whole file
  return ((char*)Lock())+start;
}

void* OverrideEntry::Lock(){
  if(Data == NULL){
    if(IsInArchive()){
      Data = ContainingArchive->ExtractFileToMemory(FileIndex);
    }else{
      MappedFile = new mapped_file(OverriderFile.wstring());

      Data = MappedFile->data();
    }
  }

  return Data;
}

void OverrideEntry::UnLock(){
  if(IsInArchive()){
    delete Data;
  }else{
    if(MappedFile != nullptr)delete MappedFile;
    MappedFile = nullptr;
  }

  Data = nullptr;
}

uint32_t OverrideEntry::GetLength(){
  if(IsInArchive()){
    return ContainingArchive->GetFileSize(FileIndex);;
  }else{
    return boostfs::file_size(OverriderFile);
  }
}
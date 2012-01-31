#include "StdAfx.h"
#include "MountedFile.h"
#include "Archive.h"
#include "NSRootDir.h"
#include <boost/iostreams/device/mapped_file.hpp>

MountedFile::~MountedFile(){
  ReleaseItemRef();
}

int64_t MountedFile::GetFileModifiedTime(){
  if(IsInArchive()){
    return static_cast<Archive*>(ContainingSource)->GetModifiedTime(FileIndex);
  }else{
    return boostfs::last_write_time(OverriderFile);
  }
}

uint32_t MountedFile::GetLength(){
  if(IsInArchive()){
    return static_cast<Archive*>(ContainingSource)->GetFileSize(FileIndex);
  }else{
    return boostfs::file_size(OverriderFile);
  }
}

M4::File* MountedFile::MakeEngineFileObj(M4::Allocator* alc){

  if(IsInArchive()){
    return static_cast<Archive*>(ContainingSource)->GetEngineFile(alc, FileIndex);
  }else{
    // static_cast<DirectoryFileSource*>(ContainingSource)->GetEngineFile();


    return new(alc) DirEngineFile(OverriderFile);
  }
}

void MountedFile::AddItemRef(){
  if(ContainingSource != NULL && IsInArchive()){
    static_cast<Archive*>(ContainingSource)->FileMounted(FileIndex);
  }
}

void MountedFile::ReleaseItemRef(){
  if(ContainingSource != NULL && IsInArchive()){
    static_cast<Archive*>(ContainingSource)->FileUnMounted(FileIndex);
  }
}

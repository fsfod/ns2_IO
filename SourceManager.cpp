#include "StdAfx.h"
#include "SourceManager.h"
#include "SevenZip.h"
#include "Archive.h"
#include "EngineInterfaces.h"
#include "ResourceOverrider.h"

#include <boost\algorithm\string\case_conv.hpp>

extern SevenZip* SevenZipLib;

std::map<PlatformPath::string_type, Archive*> SourceManager::OpenArchives;
ResourceOverrider* SourceManager::Overrider = NULL;
SourceCollection* SourceManager::ExtraSources = NULL;

extern ResourceOverrider* OverrideSource;



PlatformPath::string_type NormalizePath(const PlatformPath& path){

  PlatformPath::string_type normpath = boost::to_lower_copy(path.native());
  boost::replace(normpath, '\\', '/');

  return normpath;
}

void SourceManager::SetupFilesystemMounting(){
/*
  HMODULE engine = GetModuleHandleA("engine.dll");

  if(engine == INVALID_HANDLE_VALUE){
    throw exception("Failed to get engine.dll handle");
  }
*/

  M4::FileSystem& FileSystem = M4::Singleton<M4::FileSystem>::Get();

  OverrideSource = Overrider = new (FileSystem.Allocate(sizeof(ResourceOverrider))) ResourceOverrider();

  //make sure theres space for ResourceOverrider by adding a dummy
  FileSystem.AddSource2(nullptr, 1);

   M4::FileListType& FileSystemList = FileSystem.FileList;

  //shift everything up a slot so we can add our override source at the start
  FileSystemList.pop_back();
  FileSystemList.push_front(std::pair<int, M4::FileSource*>(1, Overrider));

  ExtraSources = new (FileSystem.Allocate(sizeof(SourceCollection))) SourceCollection();

  FileSystem.AddSource2(ExtraSources, 1);
}


void SourceManager::ArchiveClosed(Archive* arch){

  auto normpath = NormalizePath(arch->GetArchivePath());
  auto result = OpenArchives.find(normpath);

  if(result != OpenArchives.end()){
    if(result->second == arch){
      OpenArchives.erase(result);
      //arch->CheckDelete();
    }else{
      //we some how got passed an archive we don't own
      _ASSERT(false);
    }
  }
}

Archive* SourceManager::OpenArchive(const PlatformPath& path){

  auto normpath = NormalizePath(path);
  auto result = OpenArchives.find(normpath);

  if(result != OpenArchives.end()){
    return result->second;
  }

  Archive* archive = SevenZipLib->OpenArchive(normpath);

  OpenArchives[normpath] = archive;

  return archive;
}

void SourceManager::CloseAllArchives(){

}

int SourceManager::UnmountFilesFromSource( ::FileSource* source ){
  return Overrider->UnmountFilesFromSource(source);
}


void SourceManager::MountFile(const string& path, MountedFile& File, bool TriggerModifed /*= false*/ ){
  Overrider->MountFile(path, File, TriggerModifed);
}

void SourceManager::MountFilesList( Archive* source, vector<pair<string, int>>& fileList, int GroupId ,bool triggerModifed /*= false */ ){
  Overrider->MountFilesList(source, fileList, triggerModifed);
}

bool SourceManager::UnmountFile(const string& path){
  return Overrider->UnmountFile(path);
}

void SourceManager::MountSource( ::FileSource* source ){
  ExtraSources->AddSource(source);
}


#include "StdAfx.h"
#include "SourceManager.h"
#include "C7ZipLibrary.h"
#include "Archive.h"
#include "EngineInterfaces.h"
#include "ResourceOverrider.h"

#include <boost\algorithm\string\case_conv.hpp>

extern C7ZipLibrary* SevenZip;

std::map<PlatformPath::string_type, Archive*> SourceManager::OpenArchives;
ResourceOverrider* SourceManager::Overrider = NULL;
EndSource* SourceManager::ExtraSources = NULL;

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

  OverrideSource = Overrider = new ResourceOverrider();

  //make sure theres space for ResourceOverrider by adding a dummy
  FileSystem.AddSource(nullptr, 1);

  FileListType& FileSystemList = FileSystem.FileList;

  int listsize = FileSystemList.size();

  //shift everything up a slot so we can add our override source at the start
  FileSystemList.pop_back();
  FileSystemList.push_front(std::pair<int, M4::FileSource*>(1, Overrider));

  ExtraSources = new EndSource();

  FileSystem.AddSource(ExtraSources, 1);
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

  Archive* archive = SevenZip->OpenArchive(normpath);

  OpenArchives[normpath] = archive;

  return archive;
}

void SourceManager::CloseAllArchives(){

}

M4::File* EndSource::OpenFile( const VC05string& path, bool something ){
  string normpath = NormalizedPath(path.c_str(), path.size());

  BOOST_FOREACH(::FileSource* source, ExtraSources){
    M4::File* file = source->GetEngineFile(normpath);

    if(file != NULL){
      return file;
    }
  }

  return NULL;
}

bool EndSource::GetFileExists( const VC05string& path ){
  string normpath = NormalizedPath(path.c_str(), path.size());

  BOOST_FOREACH(::FileSource* source, ExtraSources){
    if(source->FileExist(normpath))return true;
  }

  return false;
}

std::int64_t EndSource::GetFileModificationTime( const VC05string& path ){
  string normpath = NormalizedPath(path.c_str(), path.size());

  BOOST_FOREACH(::FileSource* source, ExtraSources){
    if(source->FileExist(normpath)){
      return source->GetFileModificationTime(normpath);
    }
  }

  return 0;
}

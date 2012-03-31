#include "StdAfx.h"
#include "SourceCollection.h"

extern void LogMessage(const char* msg);

M4::File* SourceCollection::OpenFile(M4::Allocator* alc, const char* path, bool something){

  try{
    string normpath = NormalizePath(path);

    return GetEngineFile(normpath, alc);
  }catch(exception e){
    LogMessage(e.what());
  }

  return NULL;
}

bool SourceCollection::GetFileExists( const char* path )
{

  try{
    return FileExists(NormalizePath(path));
  }catch(exception e){
    LogMessage(e.what());
  }

  return false;
}

std::int64_t SourceCollection::GetFileModificationTime( const char* path )
{
  try{
    return GetFileModifiedTime(NormalizePath(path));
  }catch(exception e){
    LogMessage(e.what());
  }

  return 0;
}

void SourceCollection::GetChangedFiles(VC05Vector<VC05string>& ChangedFiles){
  if(ChangedFileList.size() != 0)return;

  
  BOOST_FOREACH(::FileSource* source, SourceList){
    source->GetChangedFiles(ChangedFiles);
  }


  BOOST_FOREACH(VC05string& file, ChangedFileList){
    ChangedFiles.push_back(std::move(file));
  }

  ChangedFileList.clear();
}

void SourceCollection::AddSource( ::FileSource* source ){
  if(std::find(SourceList.begin(), SourceList.end(), source) != SourceList.end()){
    throw exception("That source has already been added");
  }

  SourceList.push_back(source);
}

::FileSource* SourceCollection::FindSourceWithPath(const PlatformPath& path){

  PlatformPath normPath = NormalizePath(path.c_str());

  BOOST_FOREACH(::FileSource* source, SourceList){
    if(source->GetNormlizedFilePath() == normPath){
      return source;
    }
  }

  return NULL;
}

bool SourceCollection::RemoveSource( ::FileSource* source ){
  auto pos = std::find(SourceList.begin(), SourceList.end(), source);

  if(pos == SourceList.end())return false;

  SourceList.erase(pos);
  return true;
}

#pragma once
#include "StdAfx.h"
#include "FileSource.h"

class SourceCollection: public M4::FileSource{

public:
  SourceCollection() :SourceList(){
  }

  void AddSource(::FileSource* source);

  bool RemoveSource(::FileSource* source);

  void RemoveAllSources(){
    SourceList.clear();
  }

  virtual M4::File* OpenFile( const VC05string& path, bool something );

  virtual VC05string GetDescription(){
    return VC05string("EndSource");
  }

  virtual bool GetFileExists(const VC05string& path);
  virtual void GetChangedFiles(VC05Vector<VC05string>& ChangedFiles);
  virtual int64_t GetFileModificationTime(const VC05string& path);

  M4::File* GetEngineFile(const string& path){
    BOOST_FOREACH(::FileSource* source, SourceList){
      M4::File* file = source->GetEngineFile(path);

      if(file != NULL){
        return file;
      }
    }

    return NULL;
  }

  bool FileExists(const string& path){
    BOOST_FOREACH(::FileSource* source, SourceList){
      if(source->FileExist(path)){
        return true;
      }
    }

    return false;
  }

  std::int64_t GetFileModifiedTime(const string& path){
    BOOST_FOREACH(::FileSource* source, SourceList){
      if(source->FileExist(path)){
        return source->GetFileModificationTime(path);
      }
    }
   return 0;
  }

private:
  vector<::FileSource*> SourceList;
  vector<VC05string> ChangedFileList;
};

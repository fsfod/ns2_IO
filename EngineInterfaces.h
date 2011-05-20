#pragma once

#include "VC2005Compat.h"

class EngineFile{
public:
  virtual ~EngineFile(){}
  virtual uint32_t GetLength() = 0;

  //these 2 can't have the same name seems to mess up how the vtable is generated
  virtual void* LockRange(uint32_t start, uint32_t size) = 0;
  virtual void* Lock() = 0;

  virtual void UnLock() = 0;
};


class EngineFileSource{
public:
  virtual ~EngineFileSource(){}

  virtual EngineFile* OpenFile(const VC05string& path, bool something) = 0;
  virtual bool GetFileExists(const VC05string& path) = 0;

  //these 2 are optional
  virtual void GetChangedFiles(std::vector<VC05string>& ChangeFiles){
    return;
  }

  virtual void GetMatchingFileNames(const VC05string& path, bool something, std::vector<VC05string>& FoundFiles){
    return;
  }

  virtual VC05string GetDescription()= 0;
};
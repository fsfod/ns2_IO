#pragma once

#include "VC2005Compat.h"

 
namespace M4{

  template<typename T> class Singleton{
    public:
       static T& Get();
  };

  class Log{
   public:
     Log& Log::operator<<(char const *);
  };
  

  class Allocator{

   public:
    virtual ~Allocator(){}

    virtual void* AllocateAligned(size_t size, int alignment) = 0;
    virtual void Free(void* data);
    virtual void ReallocateAligned(void *data, size_t size, int alignment) = 0;
    virtual size_t GetAllocationSize(void *data) = 0;
  };

  class File{
   public:
    virtual uint32_t GetLength() = 0;
  
    //these 2 can't have the same name seems to mess up how the vtable is generated
    virtual void* LockRange(uint32_t start, uint32_t size) = 0;
    virtual void* Lock() = 0;
  
    virtual void UnLock() = 0;

    virtual void Destroy(int deleteOptions) = 0;
  };

  class FileSource{
   public:
    virtual ~FileSource(){}
  
    virtual M4::File* OpenFile(M4::Allocator* alc, const VC05string& path, bool something) = 0;
    virtual bool GetFileExists(const VC05string& path) = 0;
    virtual int64_t GetFileModificationTime(const VC05string& path) = 0;

    //these 2 are optional
    virtual void GetChangedFiles(VC05Vector<VC05string>& ChangeFiles){
      return;
    }
  
    virtual void GetMatchingFileNames(const VC05string& path, bool something, std::vector<VC05string>& FoundFiles){
      return;
    }
  
    virtual VC05string GetDescription()= 0;
  };

  typedef VC05Vector<std::pair<int, FileSource*>> FileListType;

  class FileSystem{
   public:
    virtual ~FileSystem(){}

   void AddSource2(M4::FileSource* Source, unsigned int flags){
     AddSource(Source, flags);
   }

   int padding1;

   FileListType FileList;

   private:
    void AddSource(M4::FileSource* Source, unsigned int flags);
    void RemoveSource(M4::FileSource *Source);
  };
};

static void LogMessage(const char* msg){
  M4::Singleton<M4::Log>::Get() << msg;
}
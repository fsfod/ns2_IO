#pragma once

#include "stdafx.h"
#include "7zip/CPP/Common/MyCom.h"
#include "7zip/CPP/7zip/Archive/IArchive.h"
#include <boost/iostreams/device/mapped_file.hpp>

using namespace boost::filesystem;
#include <boost/iostreams/stream.hpp>
#include <memory>

//using namespace std;
using namespace boost::iostreams;


class SimpleOutSteam : public ISequentialOutStream, public CMyUnknownImp{

public:
  MY_UNKNOWN_IMP

    SimpleOutSteam(): Size(0), Capacity(0), Position(0), buffer(NULL){
  }

  SimpleOutSteam(int size): Size(0), Capacity(0), Position(0), buffer(NULL){
    SetSize(size);
  }

  virtual ~SimpleOutSteam(){
    if(buffer != NULL){
      delete buffer;
      buffer = NULL;
    }
  }

  std::unique_ptr<char> TransferBufferOwnership(){

    if(buffer == NULL){
      throw exception("Cannot transfer a NULL buffer");
    }

    char* ret = buffer;

    buffer = NULL;
    Size = Capacity = Position = 0;

    return std::unique_ptr<char>(ret);
  }

  STDMETHOD(Write)(const void *data, UInt32 dataSize, UInt32 *processedSize){
    if(Position+(int)dataSize > Capacity){
      SetSize(Capacity*2);
    }

    memcpy_s(buffer+Position, Capacity-Position, data, dataSize);

    *processedSize = dataSize;

    Position += dataSize;

    return S_OK;
  }

  STDMETHOD(SetSize)(Int64 newSize){

    _ASSERT(newSize < INT32_MAX);

    if(newSize > Capacity){
      char* old = buffer;

      buffer = new char[(int)newSize];

      if(Size != 0){
        memcpy_s(buffer, (int)newSize, old, Size);
      }

      Capacity = (int)newSize;

      if(old != NULL)delete old;
    }

    Size = (int)newSize;

    if(Position > Size){
      Position = Size;
    }

    return S_OK;
  }

  int LoadChunk(lua_State* L, const char* ChunkName){
    return luaL_loadbuffer(L, buffer, Position, ChunkName);
  }

  uint32_t GetSize() 
  {
    throw std::exception("The method or operation is not implemented.");
  }


private:
  char* buffer;

  int Size, Position, Capacity;

  friend class ExtractCallback;
};

class MemMappedOutSteam : public ISequentialOutStream, public CMyUnknownImp{

public:
  MY_UNKNOWN_IMP

    MemMappedOutSteam(): Size(0), Position(0), buffer(NULL){
  }

  MemMappedOutSteam(PlatformPath path, int size): Size(size), Position(0), buffer(NULL){
    basic_mapped_file_params<wstring> params(path.wstring());
    params.new_file_size = size;
    params.flags = mapped_file_base::readwrite;

    mappedfile = mapped_file_sink(params);
    buffer = mappedfile.data();
  }

  virtual ~MemMappedOutSteam(){
    mappedfile.close();
  }

  STDMETHOD(Write)(const void *data, UInt32 dataSize, UInt32 *processedSize){
    _ASSERT(Position+(int)dataSize <= Size);

    memcpy_s(buffer+Position, Size-Position, data, dataSize);
    Position += dataSize;

    return S_OK;
  }

  STDMETHOD(SetSize)(Int64 newSize){
    _ASSERT(newSize <= Size);

    return S_OK;
  }

private:
  mapped_file_sink mappedfile;
  char* buffer;
  int Size, Position;

  friend class ExtractCallback;
};

class SingleExtractCallback: public IArchiveExtractCallback,
  public CMyUnknownImp{

public:
  MY_UNKNOWN_IMP

  SingleExtractCallback(int index, ISequentialOutStream *stream) : 
    FileIndex(index), Stream(stream), Result(-1){

    stream->AddRef();
    stream->AddRef();

    AddRef();
    AddRef();
  }

  // IProgress
  STDMETHOD(SetTotal)(UInt64 size){
    return S_OK;
  }

  STDMETHOD(SetCompleted)(const UInt64 *completeValue){
    return S_OK;
  }

  // IArchiveExtractCallback
  STDMETHOD(GetStream)(UInt32 index, ISequentialOutStream **outStream, Int32 askExtractMode){
    _ASSERT(index == FileIndex);

    if(askExtractMode != NArchive::NExtract::NAskMode::kExtract)return S_OK;

    *outStream = Stream;

    return S_OK;
  }

  STDMETHOD(PrepareOperation)(Int32 askExtractMode){
    return S_OK;
  }

  void CheckResult(){
    switch(Result){
    case NArchive::NExtract::NOperationResult::kOK:
      break;

    case NArchive::NExtract::NOperationResult::kUnSupportedMethod:
      throw exception("error extracting file unsupported compressing method used on file");

    case NArchive::NExtract::NOperationResult::kCRCError:
      throw exception("error extracting file crc mismatch");

    case NArchive::NExtract::NOperationResult::kDataError:
      throw exception("error extracting file unknown data error");
    }
  }

  //we delay throwing an error from this result till CheckResult is called
  STDMETHOD(SetOperationResult)(Int32 OperationResult){
    _ASSERT(Result == -1);
    Result = OperationResult;
    return S_OK;
  }


private:
  ISequentialOutStream* Stream;
  int Result;
  int FileIndex;
};

#include "stdafx.h"
#include "LuaErrorWrapper.h"
#include "StringUtil.h"
#include "FileSource.h"
#include "FileInfo.h"
#include <boost/iostreams/device/mapped_file.hpp>

#pragma once
#include <system_error>

class DirectoryFileSource;
class DirEngineFile;

class DirectoryFileSource : public FileSource{

public:
	DirectoryFileSource(){}
	DirectoryFileSource(char* path);

	DirectoryFileSource(const PlatformPath& DirectoryPath, const string& GamePath, DirectoryFileSource* ParentSource = NULL);
	virtual ~DirectoryFileSource(){}

	bool FileExists(const PathStringArg& path);
  bool DirectoryExists(const PathStringArg& path);
	int FindFiles(const PathStringArg& SearchPath, const PathStringArg& NamePatten, FileSearchResult& FoundFiles);
	int FindDirectorys(const PathStringArg& SearchPath, const PathStringArg& NamePatten, FileSearchResult& FoundDirectorys);
	bool FileSize(const PathStringArg& path, double& Filesize);
	bool GetModifiedTime(const PathStringArg& Path, int32_t& Time);

  bool FileExists(const std::string& FilePath);
  bool GetFileStat(const std::string& Path, FileInfo& stat);

  PlatformPath MakePlatformPath(std::string path) const{
    return RealPath/path;
  }

  PlatformPath MakePlatformPath(const PathStringArg& path) const{
    return path.CreatePath(RealPath);
  }

  PlatformPath operator/(const std::wstring& path) const{
    return RealPath/path;
  }

  PlatformPath operator/(const PlatformPath& path) const{
    return RealPath/path;
  }

  PlatformPath operator/(const std::string& path) const{
    return RealPath/path;
  }

  PlatformPath operator/(const char* path) const{
    return RealPath/path;
  }

	const std::string& get_Path();

	PathString& GetPath(){
		return GameFileSystemPath;
	}

	const PlatformPath& GetFileSystemPath(){
		return RealPath;
	}

  virtual bool FileExist(const string& path){
    return boostfs::exists(RealPath/path);
  }

  virtual M4::File* GetEngineFile(const string& path);

  virtual void MountFile(const PathStringArg& FilePath, const PathStringArg& DestinationPath);
  virtual void MountFiles(const PathStringArg& BasePath, const PathStringArg& DestinationPath);
  
  DirectoryFileSource* CreateChildSource(const string& SubDirectory);
  
  DirectoryFileSource* LUA_CreateChildSource(const PathStringArg& SubDirectory){
    ConvertAndValidatePath(SubDirectory);
    return CreateChildSource(SubDirectory.GetNormalizedPath());
  }

  template<typename T> void ForEachFile(PlatformPath SubDirectory, T func);

private:
	void LoadLuaFile(lua_State* L, const PathStringArg& FilePath);
  int RelativeRequire(lua_State* L, const PathStringArg& ModuleName );

  PathString GameFileSystemPath;
	PlatformPath RealPath;

	DirectoryFileSource* ParentSource;
  std::map<string, DirectoryFileSource*> ChildSources;
};

class DirEngineFile :public M4::File{
public:
  DirEngineFile(PlatformPath filepath):FilePath(filepath), Data(){
  }

  DirEngineFile(PlatformPath filepath, shared_ptr<void>& PreloadedData):FilePath(filepath), Data(PreloadedData){
  }

  ~DirEngineFile(){
    UnLock();
  }

  virtual void* LockRange(uint32_t start, uint32_t size){
    return Lock();
  }

  virtual void* Lock(){
    using namespace boost::iostreams;

    if(Data == NULL){
      try{
        mapped_file* MappedFile = new mapped_file(FilePath.wstring() );

        Data.reset(MappedFile->data(), [MappedFile](void* value){
          delete MappedFile;
        });
      }catch(exception e){
        //should really log something here
        return NULL;
      }
    }

    return Data.get();
  }

  void UnLock(){
    Data.reset();
  }

  uint32_t GetLength(){
   boost::system::error_code error;
   
   uint32_t size = (uint32_t)boostfs::file_size(FilePath, error);

   return error.value() == 0 ? size : 0;
  }

  PlatformPath FilePath;
  shared_ptr<void> Data;
};

template<typename T> void DirectoryFileSource::ForEachFile(PlatformPath SubDirectory, T func){

  FileInfo FindData;
  memset(&FindData, 0, sizeof(FileInfo));


  PlatformPath& path = (SubDirectory.empty() ? RealPath : (RealPath/SubDirectory));

  if(path.native().back() != '*'){
    path /= L"*";
  }

  HANDLE  FHandle = FindFirstFile(path.c_str(),  &FindData);

  if(FHandle == INVALID_HANDLE_VALUE){
    return;
  }

  do{
    if(FindData.cFileName[0] != '.' || !(FindData.cFileName[1] == 0 || (FindData.cFileName[1] == '.' && FindData.cFileName[2] == 0))){
      func(FindData);
    }
  }while(FindNextFile(FHandle, &FindData));
}


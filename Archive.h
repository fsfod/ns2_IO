#include "stdafx.h"
#include "LuaErrorWrapper.h"
#include "StringUtil.h"
#include "FileSource.h"
#include "SourceDirectory.h"

#pragma once

class C7ZipLibrary;
struct IInArchive;




struct FileEntry{
	int FileIndex;

	FileEntry(){
		FileIndex = -1;
	}

	FileEntry(int index){
		FileIndex = index;
	}
};


class Archive : public FileSource, SourceDirectory<FileEntry>{

public:
	Archive(C7ZipLibrary* owner, PathString archivepath, IInArchive* Reader);
	virtual ~Archive();

	bool FileExists(const PathStringArg& path);
	bool DirectoryExists(const PathStringArg& path);
	int FindFiles(const PathStringArg& SearchPath, const PathStringArg& NamePatten, FileSearchResult& FoundFiles);
	int FindDirectorys(const PathStringArg& SearchPath, const PathStringArg& NamePatten, FileSearchResult& FoundDirectorys);
	bool FileSize(const PathStringArg& path, double& Filesize);
	bool GetModifiedTime(const PathStringArg& Path, int32_t& Time);

	// Called directly by Lua
	void LoadLuaFile(lua_State* L, const PathStringArg& FilePath);

	const std::string& get_Path(){return FileSystemPath;}

	static luabind::scope RegisterClass();
  
  void ExtractFile(uint32_t FileIndex, const PlatformPath& FilePath);
  void* ExtractFileToMemory(uint32_t FileIndex);

  int GetFileIndex(const std::string& path){

    auto file = PathToFile.find(path);

    if(file == PathToFile.end()){
      return -1;
    }

    return file->second.FileIndex;
  }

  EngineFile* GetEngineFile(int FileIndex){
    if(FileIndex < 0)throw std::exception("GetEngineFile: Invalid File index");

    return new ArchiveFile(this, FileIndex);
  }

  uint32_t GetFileCRC( int FileIndex );
  uint32_t GetFileSize(int index);

  shared_ptr<FileSource> GetLuaPointer();
  void FileMounted( int FileIndex );

private:
	void AddFilesToDirectorys();
	SelfType* CreateDirectorysForPath(const std::string& path, std::vector<int>& SlashIndexs);
  
  PlatformPath ArchivePath;

	PathString FileSystemPath;
	C7ZipLibrary* Owner;
	IInArchive* Reader;

  bool HasMountedFiles;
  boost::weak_ptr<FileSource> LuaPointer;

	std::hash_map<std::string, FileEntry> PathToFile;
	std::hash_map<std::string, SelfType*> PathToDirectory;
	std::string ArchiveName;


  class ArchiveFile :public EngineFile{
    public:
      ArchiveFile(Archive* owner, int fileIndex, bool Preload = false) : Owner(owner), FileIndex(fileIndex), Data(NULL){
        if(Preload)Lock();
      }

      virtual ~ArchiveFile(){
        UnLock();
      }

      virtual void* LockRange(uint32_t start, uint32_t size){
        return Lock();
      }

      virtual void* Lock(){

        if(Data == NULL){
          try{
            Data = Owner->ExtractFileToMemory(FileIndex);
          }catch (exception e){
            return NULL;
          }
        }

        return Data;
      }

      virtual void UnLock(){
        delete Data;
        Data = NULL;
      }

      virtual uint32_t GetLength(){
        return Owner->GetFileSize(FileIndex);
      }

    private:
      void* Data;
      int FileIndex;
      Archive* Owner;
    };
};
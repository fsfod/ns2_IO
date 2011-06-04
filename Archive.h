#pragma once

#include "stdafx.h"
#include "StringUtil.h"
#include "SourceManager.h"
#include "FileSource.h"
#include "SourceDirectory.h"


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

  virtual void MountFile(const PathStringArg& FilePath, const PathStringArg& DestinationPath);
  virtual void MountFiles(const PathStringArg& BasePath, const PathStringArg& DestinationPath);

	// Called directly by Lua
	void LoadLuaFile(lua_State* L, const PathStringArg& FilePath);

	const std::string& get_Path(){return FileSystemPath;}

  const PlatformPath& GetArchivePath(){return ArchivePath;}

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

  shared_ptr<FileSource> GetLuaPointer();

  M4::File* GetEngineFile(const string& path);

  virtual bool FileExist(const string& path){
    return PathToFile.find(path) != PathToFile.end();
  }

  M4::File* GetEngineFile(int FileIndex){
    if(FileIndex < 0)throw std::exception("GetEngineFile: Invalid File index");

    return new ArchiveFile(this, FileIndex);
  }

  uint32_t GetFileCRC(int FileIndex);
  uint32_t GetFileSize(int index);

  void CheckDelete(){
    if(MountedFileCount == 0 && LuaPointer.use_count() == 0){
      SourceManager::ArchiveClosed(this);
      delete this;
    }
  }

  void FileMounted(int FileIndex){
    if(FileIndex != -1){
     MountedFileCount++;
    }
  }
  
  void FileUnMounted(int FileIndex){
    if(FileIndex != -1){
      if(--MountedFileCount == 0){
        CheckDelete();
      }
    }
  }

  void CreateMountList(const std::string& BasePath, const std::string& path, vector<pair<string, int>>& fileList);

private:
	void AddFilesToDirectorys();
	SelfType* CreateDirectorysForPath(const std::string& path, std::vector<int>& SlashIndexs);

  PlatformPath ArchivePath;

	PathString FileSystemPath;
	C7ZipLibrary* Owner;
	IInArchive* Reader;

  int MountedFileCount;
  boost::weak_ptr<FileSource> LuaPointer;

	std::hash_map<std::string, FileEntry> PathToFile;
	std::hash_map<std::string, SelfType*> PathToDirectory;
	std::string ArchiveName;


  class ArchiveFile :public M4::File{
    public:
      ArchiveFile(Archive* owner, int fileIndex, bool preload = false) : Owner(owner), FileIndex(fileIndex), Data(){
        if(preload)Lock();
        Owner->FileMounted(FileIndex);
      }

      ArchiveFile(Archive* owner, int fileIndex, shared_ptr<char>& preloadData) : Owner(owner), FileIndex(fileIndex), Data(preloadData){
        Owner->FileMounted(FileIndex);
      }

      virtual ~ArchiveFile(){
        UnLock();
        Owner->FileUnMounted(FileIndex);
      }

      virtual void* LockRange(uint32_t start, uint32_t size){
        return Lock();
      }

      virtual void* Lock(){

        if(Data == NULL){
          try{
            Data.reset((char*)Owner->ExtractFileToMemory(FileIndex));
          }catch (exception e){
            return NULL;
          }
        }

        return Data.get();
      }

      virtual void UnLock(){
        Data.reset();
      }

      virtual uint32_t GetLength(){
        return Owner->GetFileSize(FileIndex);
      }

    private:
      shared_ptr<char> Data;
      int FileIndex;
      Archive* Owner;
    };
};
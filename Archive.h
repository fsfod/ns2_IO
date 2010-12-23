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

private:
	void AddFilesToDirectorys();
	SelfType* CreateDirectorysForPath(const std::string& path, std::vector<int>& SlashIndexs);
	int GetFileEntrysSize(const FileEntry& fileentry);


	PlatformPath ArchivePath;

	PathString FileSystemPath;
	C7ZipLibrary* Owner;
	IInArchive* Reader;

	std::hash_map<std::string, FileEntry> PathToFile;
	std::hash_map<std::string, SelfType*> PathToDirectory;
	std::string ArchiveName;
};
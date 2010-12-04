#include "stdafx.h"
#include "LuaErrorWrapper.h"
#include "StringUtil.h"
#include "FileSource.h"

#pragma once

class C7ZipLibrary;
struct IInArchive;


template <class NameStringType, class FileRecord> class SourceDirectory{

typedef typename SourceDirectory<NameStringType, FileRecord> SelfType;

public:
	typedef typename std::hash_map<NameStringType, FileRecord>::value_type FileNode;
	typedef typename std::hash_map<NameStringType, SelfType>::value_type DirectoryNode;

	void FindFiles(PathString Patten){

		BOOST_FOREACH(const FileNode& file, Files){
			file.first.find_last_of(Patten);

			PathMatchSpecEx(file.first.c_str(), Patten.c_str(), PMSF_NORMAL);
		}
	}

protected:
	SelfType* Parent;
	std::hash_map<NameStringType, FileRecord> Files;
	std::hash_map<NameStringType, SelfType> Directorys;
};

struct FileEntry{
	int FileIndex;

	FileEntry(){
		FileIndex = -1;
	}

	FileEntry(int index){
		FileIndex = index;
	}
};

class Archive : public FileSource{

public:
	Archive(C7ZipLibrary* owner, PathString archivepath, IInArchive* Reader);
	~Archive();

	bool FileExists(const PathString& path);
	bool DirectoryExists(const PathString& path);
	int FindFiles(const PathString& SearchPath, const PathString& NamePatten, FileSearchResult& FoundFiles);
	int FindDirectorys(const PathString& SearchPath, const PathString& NamePatten, FileSearchResult& FoundDirectorys);
	bool FileSize(const PathString& path, double& Filesize);
	bool GetModifiedTime(const PathString& Path, int32_t& Time);

	// Called directly by Lua
	void LoadLuaFile(lua_State* L, const PathStringArg& FilePath);

	const PathStringArg& get_Path(){return static_cast<const PathStringArg&>(FileSystemPath);}

	static luabind::scope RegisterClass();

private:
	PathString ArchivePath, FileSystemPath;
	C7ZipLibrary* Owner;
	IInArchive* Reader;

	std::hash_map<PathString, FileEntry> Files;
	SourceDirectory<PathString, FileEntry> Directorys;
};
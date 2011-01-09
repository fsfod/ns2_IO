#include "stdafx.h"
#include "LuaErrorWrapper.h"
#include "StringUtil.h"
#include "FileSource.h"

#pragma once

class DirectoryFileSource;


class DirectoryFileSource : public FileSource{

public:
	DirectoryFileSource(){}
	DirectoryFileSource(char* path);

	DirectoryFileSource(const PlatformPath& DirectoryPath, const PathString& GamePath, bool isRootSource);
	virtual ~DirectoryFileSource(){}

	bool FileExists(const PathStringArg& path);
	bool DirectoryExists(const PathStringArg& path);
	int FindFiles(const PathStringArg& SearchPath, const PathStringArg& NamePatten, FileSearchResult& FoundFiles);
	int FindDirectorys(const PathStringArg& SearchPath, const PathStringArg& NamePatten, FileSearchResult& FoundDirectorys);
	bool FileSize(const PathStringArg& path, double& Filesize);
	bool GetModifiedTime(const PathStringArg& Path, int32_t& Time);


	const std::string& get_Path();

  PlatformPath CompletePath(const PlatformPath& Path);

	PathString& GetPath(){
		return GameFileSystemPath;
	}

	const PlatformPath& GetFileSystemPath(){
		return RealPath;
	}


private:
	void SetupPathStrings(const PathString& nsroot, const PathString& path);
	void LoadLuaFile(lua_State* L, const PathStringArg& FilePath);
  int RelativeRequire(lua_State* L, const PathStringArg& ModuleName );
	PathString GameFileSystemPath;
	PlatformPath RealPath;
	bool IsRootSource;
};


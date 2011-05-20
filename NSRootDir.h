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

  bool FileExists(const std::string& FilePath);
  bool GetFileStat(const std::string& Path, PlatformFileStat& stat);

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


private:
	void SetupPathStrings(const PathString& nsroot, const PathString& path);
	void LoadLuaFile(lua_State* L, const PathStringArg& FilePath);
  int RelativeRequire(lua_State* L, const PathStringArg& ModuleName );
	PathString GameFileSystemPath;
	PlatformPath RealPath;
	bool IsRootSource;
};


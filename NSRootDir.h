#include "stdafx.h"
#include "LuaErrorWrapper.h"
#include "StringUtil.h"
#include "FileSource.h"

#pragma once

class NSRootDir;


class NSRootDir : public FileSource{

public:
		NSRootDir(){}
		NSRootDir(PathString nsroot, char* path){
#if defined(UNICODE)
		std::wstring wpath;
		UTF8ToWString(path, wpath);

		SetupPathStrings(nsroot, wpath);
#else
		SetupPathStrings(nsroot, PathString(path));
#endif // UINCODE
	}

	NSRootDir(PathString nsroot, PathString path){SetupPathStrings(nsroot, path);}
	~NSRootDir(){}

	bool FileExists(const PathString& path);
	bool DirectoryExists(const PathString& path);
	int FindFiles(const PathString& SearchPath, const PathString& NamePatten, FileSearchResult& FoundFiles);
	int FindDirectorys(const PathString& SearchPath, const PathString& NamePatten, FileSearchResult& FoundDirectorys);
	bool FileSize(const PathString& path, double& Filesize);
	bool GetModifiedTime(const PathString& Path, int32_t& Time);

	const PathStringArg& get_Path();

	PathString& GetPath(){
		return GameFileSystemPath;
	}

private:
	void SetupPathStrings(const PathString& nsroot, const PathString& path);
	void LoadLuaFile(lua_State* L, const PathStringArg& FilePath);
	

	PathString GameFileSystemPath, RealPath;
};


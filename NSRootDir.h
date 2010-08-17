#include "stdafx.h"
#include <unordered_map>
#include "LuaErrorWrapper.h"
#include "StringUtil.h"

#pragma once

class NSRootDir;

typedef std::tr1::unordered_map<std::string,NSRootDir*> FileSearchResult;


class NSRootDir{

public:
		NSRootDir(){PathLength = 0;}
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
	int FindFiles(const PathString SearchPath, FileSearchResult& FoundFiles);
	int FindDirectorys(const PathString SearchPath, FileSearchResult& FoundDirectorys);
	bool FileSize(const PathString& path, uint32_t& Filesize);
	bool GetModifiedTime(const PathString& Path, int32_t& Time);

	PathString& GetPath(){
		return GameFileSystemPath;
	}

	void LoadLuaFile(lua_State* L, const PathStringArg& FilePath);

	static luabind::scope RegisterClass();

private:
	int32_t Wrapper_GetModifedTime(const PathStringArg& Path);
	bool Wrapper_FileExists(const PathStringArg& path);
	uint32_t Wrapper_FileSize(const PathStringArg& Path);
	luabind::object NSRootDir::Wrapper_FindDirectorys(lua_State *L, const PathStringArg& SearchString);
	luabind::object Wrapper_FindFiles(lua_State* L, const PathStringArg& SearchString);

private:

	void CheckSpace(int newspareCapcity);
	void SetupPathStrings(const PathString& nsroot, const PathString& path);
	PathString GameFileSystemPath, RealPath;
	int32_t PathLength;
};

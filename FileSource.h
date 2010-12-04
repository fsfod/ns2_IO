#pragma once
#include "stdafx.h"
#include <unordered_map>

class FileSource;

typedef std::tr1::unordered_map<std::string,FileSource*> FileSearchResult;

class FileSource :luabind::wrap_base{
public:

	virtual ~FileSource(){}

	virtual bool FileExists(const PathString& path) = 0;
	virtual bool DirectoryExists(const PathString& path) = 0;
	virtual int FindFiles(const PathString& SearchPath, const PathString& NamePatten, FileSearchResult& FoundFiles) = 0;
	virtual int FindDirectorys(const PathString& SearchPath, const PathString& NamePatten, FileSearchResult& FoundDirectorys) = 0;
	virtual bool FileSize(const PathString& path, double& Filesize) = 0;
	virtual bool GetModifiedTime(const PathString& Path, int32_t& Time) = 0;

	// Called directly by Lua
	virtual void LoadLuaFile(lua_State* L, const PathStringArg& FilePath) = 0;
	virtual const PathStringArg& get_Path() = 0;

	static luabind::scope RegisterClass();

private:
	int32_t Lua_GetModifedTime(const PathStringArg& Path);
	bool Lua_FileExists(const PathStringArg& path);
	double Lua_FileSize(const PathStringArg& Path);
	luabind::object Lua_FindDirectorys(lua_State *L, const PathStringArg& SearchPath, const PathStringArg& NamePatten);
	luabind::object Lua_FindFiles(lua_State* L, const PathStringArg& SearchPath, const PathStringArg& NamePatten);

	const PathStringArg& Lua_get_Path(){
		return get_Path();
	}
};
#pragma once
#include "stdafx.h"
#include <unordered_map>
#include "EngineInterfaces.h"

class FileSource;

typedef std::tr1::unordered_map<std::string,FileSource*> FileSearchResult;

class FileSource{

public:
	virtual ~FileSource(){}

	virtual bool FileExists(const PathStringArg& path) = 0;
	virtual bool DirectoryExists(const PathStringArg& path) = 0;
	virtual int FindFiles(const PathStringArg& SearchPath, const PathStringArg& NamePatten, FileSearchResult& FoundFiles) = 0;
	virtual int FindDirectorys(const PathStringArg& SearchPath, const PathStringArg& NamePatten, FileSearchResult& FoundDirectorys) = 0;
	virtual bool FileSize(const PathStringArg& path, double& Filesize) = 0;
	virtual bool GetModifiedTime(const PathStringArg& Path, int32_t& Time) = 0;
	virtual void LoadLuaFile(lua_State* L, const PathStringArg& path) = 0;

	virtual const std::string& get_Path() = 0;

	static luabind::scope RegisterClass();

private:
	int32_t Lua_GetModifedTime(const PathStringArg& Path);
	bool Lua_FileExists(const PathStringArg& path);
	double Lua_FileSize(const PathStringArg& Path);
	luabind::object Lua_FindDirectorys(lua_State *L, const PathStringArg& SearchPath, const PathStringArg& NamePatten);
	luabind::object Lua_FindFiles(lua_State* L, const PathStringArg& SearchPath, const PathStringArg& NamePatten);
	void Lua_LoadLuaFile(lua_State *L, const PathStringArg& path);
};
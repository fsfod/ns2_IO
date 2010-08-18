#pragma once

#include "stdafx.h"
#include "NSRootDir.h"

class LuaModule{
	public:
		static uint32_t GetFileSize(const PathStringArg& Path);
		static bool FileExists(const PathStringArg& Path);
		static int GetDateModified(const PathStringArg& Path);

		static luabind::object FindFiles(lua_State* L, const PathStringArg& SearchPath);
		static luabind::object FindDirectorys(lua_State* L, const PathStringArg& SearchPath);

		static luabind::object GetDirRootList(lua_State *L);
		static const PathStringArg&  GetGameString();

		static NSRootDir* LuaModule::GetRootDirectory(int index);

		static void Initialize(lua_State *L){
			FindNSRoot();
			FindDirectoryRoots();
		}

		static PathString NSRoot, GameString;

	private:
		static bool FindNSRoot();
		static void FindDirectoryRoots();
		static void ParseGameCommandline(PathString& CommandLine);

		static std::vector<NSRootDir> RootDirs;
};
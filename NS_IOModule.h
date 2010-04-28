#pragma once

#include "stdafx.h"
#include "NSRootDir.h"

class LuaModule{
	public:
		static void ConvertAndValidatePath(lua_State *L, int ArgumentIndex, PathString& path);
		
		static int GetFileSize(lua_State *L);
		static int FileExists(lua_State *L);
		static int FindFilesInDir(lua_State *L);
		static int FindDirectorys(lua_State *L);
		static int GetDirRootList(lua_State *L);
		static int GetGameString(lua_State *L);
		static int GetDateModifed(lua_State* L);

		static void Initialize(lua_State *L){
			FindNSRoot();
			FindDirectoryRoots();
		}

	private:
		static bool FindNSRoot();
		static void FindDirectoryRoots();
		static void ParseGameCommandline(PathString& CommandLine);
		int SaveTableTest(lua_State *L);

		static PathString NSRoot, GameString;
		static std::vector<NSRootDir> RootDirs;
};
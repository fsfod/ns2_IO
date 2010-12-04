#pragma once

#include "stdafx.h"
#include "NSRootDir.h"
#include <boost/ptr_container/ptr_vector.hpp>


class LuaModule{
	public:
		static double GetFileSize(const PathStringArg& Path);
		static bool FileExists(const PathStringArg& Path);
		static int GetDateModified(const PathStringArg& Path);

		static luabind::object FindFiles(lua_State* L, const PathStringArg SearchPath, const PathStringArg& NamePatten);
		static luabind::object FindDirectorys(lua_State* L, const PathStringArg SearchPath, const PathStringArg& NamePatten);

		static luabind::object GetDirRootList(lua_State *L);
		static const PathStringArg&  GetGameString();

		static FileSource* LuaModule::GetRootDirectory(int index);

		static void Initialize(lua_State *L);
		std::string GetCommandLineRaw();

		static PathString NSRoot, GameString, CommandLine;

	private:
		static void FindNSRoot();
		static void FindDirectoryRoots();
		static void ParseGameCommandline(PathString& CommandLine);

		static boost::ptr_vector<FileSource> RootDirs;
};
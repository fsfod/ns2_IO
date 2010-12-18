#pragma once

#include "stdafx.h"
#include "NSRootDir.h"
#include <boost/ptr_container/ptr_vector.hpp>


class LuaModule{
	public:
		static double GetFileSize(const PathStringArg& Path);
		static bool FileExists(const PathStringArg& Path);
		static int GetDateModified(const PathStringArg& Path);

		static luabind::object FindFiles(lua_State* L, const PathStringArg& SearchPath, const PathStringArg& NamePatten);
		static luabind::object FindDirectorys(lua_State* L, const PathStringArg& SearchPath, const PathStringArg& NamePatten);
		
		static FileSource* OpenArchive(lua_State* L, FileSource* ContainingSource, const PathStringArg& Path);
		static FileSource* OpenArchive2(lua_State* L, const PathStringArg& Path);

		static luabind::object GetDirRootList(lua_State *L);
		static const std::string& GetGameString();

		static FileSource* LuaModule::GetRootDirectory(int index);
		
		static int LoadLuaFile(lua_State* L, const PlatformPath& FilePath, const char* chunkname);

		static void Initialize(lua_State *L);
		std::string GetCommandLineRaw();

		static std::string CommandLine, GameString;
		static PlatformPath GameStringPath;

	private:
		static void FindNSRoot();
		static void AddMainFileSources();
		static void ParseGameCommandline(PlatformString& CommandLine);
		

		static boost::ptr_vector<FileSource> RootDirs;
};
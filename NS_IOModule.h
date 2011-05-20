#pragma once

#include "stdafx.h"
#include "NSRootDir.h"
#include "ExtractCache.h"
#include <boost/ptr_container/ptr_vector.hpp>

class LuaModule{
  public:
		static bool FileExists(const PathStringArg& Path);
    static bool DirectoryExists(const PathStringArg& path);
		static int GetDateModified(const PathStringArg& Path);
    static double GetFileSize(const PathStringArg& Path);

		static luabind::object FindFiles(lua_State* L, const PathStringArg& SearchPath, const PathStringArg& NamePatten);
		static luabind::object FindDirectorys(lua_State* L, const PathStringArg& SearchPath, const PathStringArg& NamePatten);
		
    static luabind::object GetSupportedArchiveFormats(lua_State *L);
		static boost::shared_ptr<FileSource> OpenArchive(lua_State* L, FileSource* ContainingSource, const PathStringArg& Path);
		static boost::shared_ptr<FileSource> OpenArchive2(lua_State* L, const PathStringArg& Path);
    static void MountArchiveFile(FileSource* Source, const PathStringArg& FileInArchive, const PathStringArg& DestinationPath);
    static void MountMapArchive(Archive* archive);
    static void UnMountMapArchive();

		static luabind::object GetDirRootList(lua_State *L);
		static bool IsRootFileSource(FileSource* source);
		static const std::string& GetGameString();
    static int LoadLuaDllModule(lua_State* L, FileSource* Source, const PathStringArg& DllPath );
		
		static int LoadLuaFile(lua_State* L, const PlatformPath& FilePath, const char* chunkname);

    static void ExtractResource(FileSource* Archive, const PathStringArg& FileInArchive, const PathStringArg& DestinationPath);

		static void Initialize(lua_State *L);
		std::string GetCommandLineRaw();

		static void PrintMessage(lua_State *L, const char* msg);
    static void PrintMessage(lua_State *L, const std::string& msg);

		static std::string CommandLine, GameString;
		static PlatformPath GameStringPath;

	private:
		static void FindNSRoot();
    static void SetupFilesystemMounting();
		static void ProcessCommandline();
		static void ParseGameCommandline(PlatformString& CommandLine);
		static void ProcessDirectoriesTXT(PlatformPath directorystxt);
    static void StaticInit(lua_State* L);

		static boost::ptr_vector<FileSource> RootDirs;
		static bool GameIsZip;
   
    //static ExtractCache LuaModule::ExtractedFileCache;

    static int VmShutDown(lua_State* L);
    static LuaModule* GetInstance(lua_State* L);
    
    static void LoadExtractCache();

public:
    LuaModule(lua_State* L);
    ~LuaModule(){}
    
    luabind::object MessageFunc;
    static DirectoryFileSource* ModDirectory;
};
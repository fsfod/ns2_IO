//NS2_IO by fsfod

#include "stdafx.h"
#include "NS_IOModule.h"
#include "StringUtil.h"
#include "SevenZip.h"
#include "PathStringConverter.h"

#include "ExtractCache.h"


#include <boost/algorithm/string/case_conv.hpp>
#include <boost/algorithm/string/trim.hpp>
#include <boost/iostreams/device/mapped_file.hpp>
#include <boost/iostreams/stream.hpp>
#include <fstream>

#include "ResourceOverrider.h"
#include "SourceManager.h"
#include "SQLite3Database.h"
#include "SQLTableMetaData.h"

using namespace  std;

PlatformPath NSRootPath(_T(""));
SevenZip* SevenZipLib = NULL;

//LuaModule::LuaModule(){
string LuaModule::CommandLine("");
string LuaModule::GameString("");

PlatformPath LuaModule::GameStringPath(_T(""));

DirectoryFileSource* LuaModule::ModDirectory = NULL;
boost::ptr_vector<FileSource> LuaModule::RootDirs(0);

ResourceOverrider* OverrideSource = nullptr;

//ExtractCache LuaModule::ExtractedFileCache;

//luabind::object LuaModule::MessageFunc;
bool LuaModule::GameIsZip = false;
//{

//PathMatchSpecEx for matching files names on windows
//fnmatch on linux will do the same directorys 


LuaModule::LuaModule(lua_State* L){
  
  lua_createtable(L,0, 1);
  lua_pushcfunction(L, LuaModule::VmShutDown);
  lua_setfield(L, -2, "__gc");
  lua_setmetatable(L, -2);

  lua_getfield(L, LUA_GLOBALSINDEX, "Shared");
  if(lua_type(L, -1) != LUA_TNIL){
    lua_getfield(L, -1, "Message");

    if(lua_type(L, -1) != LUA_TNIL){
      MessageFunc = luabind::object(luabind::from_stack(L, -1));
    }
    lua_pop(L, 1);
  }
  lua_pop(L, 1);
}

int LuaModule::VmShutDown(lua_State* L){

  LuaModule* instance = (LuaModule*)lua_touserdata(L, 1);

  //PrintMessage(L, "LuaModule Shutdown");

  //this could be either the Server or Client VM it doesn't really for matter unmounting 
  OverrideSource->UnmountMapArchive();

  if(instance != NULL){
    instance->~LuaModule();
  }

  return 0;
}

void LuaModule::Initialize(lua_State *L){

  LuaModule* vm = new (lua_newuserdata(L, sizeof(LuaModule))) LuaModule(L);
  lua_setfield(L, LUA_REGISTRYINDEX, "NS2_IOState");

  VC2005RunTime::GetInstance();

  StaticInit(L);
}

/*
void LuaModule::SetupFilesystemMounting(){

  HMODULE engine = GetModuleHandleA("engine.dll");
  
  if(engine == INVALID_HANDLE_VALUE){
    throw exception("Failed to get engine.dll handle");
  }

  M4::FileSystem& FileSystem = M4::Singleton<M4::FileSystem>::Get();

  OverrideSource = new ResourceOverrider();

  //make sure theres space for ResourceOverrider by adding a dummy
  FileSystem.AddSource(nullptr, 1);
 
  FileListType& FileSystemList = FileSystem.FileList;

  int listsize = FileSystemList.size();
  
  //shift everything up a slot so we can add our override source at the start
  FileSystemList.pop_back();
  FileSystemList.push_front(std::pair<int, M4::FileSource*>(1, OverrideSource));
}
*/

bool FirstLoad = true;

void LuaModule::LoadExtractCache(){

  PlatformPath CacheDirectory = *ModDirectory/"ExtractCache";

  //ExtractedFileCache.Load(CacheDirectory, ModDirectory->GetFileSystemPath());
}

SQLiteDatabase clientDB;

void LuaModule::StaticInit(lua_State* L){

  if(!FirstLoad)return;

  //increment our dll ref count by 1 so don't get destroyed on vm tear down but on process exit
  LoadLibraryA("NS2_IO.dll");

  FirstLoad = false;

  FindNSRoot();

  //if -game is set 
  ProcessCommandline();

  auto SevenZipPath = (GameStringPath.empty() ? NSRootPath : GameStringPath) /"7z.dll";

  if(boostfs::exists(SevenZipPath)){
    try{
      SevenZipLib = SevenZip::Init(SevenZipPath);
    }catch(exception e){

      string msg = "error while loading 7zip library: ";
      msg += e.what();
      PrintMessage(L, msg.c_str());

      SevenZipLib = NULL;
    }

    if(SevenZipLib != NULL && GameIsZip){
      try{
        RootDirs.push_back(SourceManager::OpenArchive(GameStringPath));
      }catch(exception e){
        string msg = "error while opening archive set with -game:";
        msg += e.what();
        PrintMessage(L, msg.c_str());
      }
    }
  }else{
    PrintMessage(L, "cannot find 7zip.dll so archive support is disabled");
  }

  if(GameIsZip){
   if(SevenZipLib == NULL) PrintMessage(L, "cannot open archive set with -game when 7zip is not loaded");
  }else{
    if(!GameStringPath.empty()){
      ModDirectory = new DirectoryFileSource(GameStringPath, "");
      RootDirs.push_back(ModDirectory);
    } 
  }

  RootDirs.push_back(new DirectoryFileSource("ns2"));
  RootDirs.push_back(new DirectoryFileSource("core"));

  auto directorystxt = NSRootPath/"directories.txt";

  if(boostfs::exists(directorystxt)){
    ProcessDirectoriesTXT(directorystxt);
  }

  SourceManager::SetupFilesystemMounting();

  try{
    clientDB.Open(NSRootPath/"SavedVariables/client.sqlitedb");
    SQLTableMetaData::Init(&clientDB);
  }catch (SQLiteException& e){
    PrintMessage(L, e.what());
  }

 // LoadExtractCache();
}

LuaModule* LuaModule::GetInstance(lua_State* L){
  
  lua_getfield(L, LUA_REGISTRYINDEX, "NS2_IOState");
  
  LuaModule* instance = (LuaModule*)lua_touserdata(L, -1);

  if(instance == NULL){
    throw exception("failed to get NS2_IOState");
  }

  lua_pop(L, 1);

  return instance;
}

void LuaModule::PrintMessage(lua_State *L, const string& msg){
  PrintMessage(L, msg.c_str());
}

void LuaModule::PrintMessage(lua_State *L, const char* msg){
  
  auto func = GetInstance(L)->MessageFunc;

	if(func.is_valid()){
		luabind::call_function<void>(func, msg);
  }
}

void LuaModule::ProcessDirectoriesTXT(PlatformPath directorystxt){

	if(boostfs::file_size(directorystxt) == 0){
		return;
	}

	using namespace boost::iostreams;

	mapped_file_source file(directorystxt.native());
	stream<mapped_file_source> input(file);

	string line;

	while(getline(input, line)){
		boost::trim(line);
		if(line.empty())continue;

		PlatformPath path = line;

		if(!path.has_root_name())path = NSRootPath/path;
			if(boostfs::is_directory(path)){
				RootDirs.push_back(new DirectoryFileSource(path/"ns2", "ns2"));
				RootDirs.push_back(new DirectoryFileSource(path/"core", "core"));
			}
	}
}

void LuaModule::FindNSRoot(){
	HMODULE hmodule = GetModuleHandleW(NULL);

	if(hmodule == INVALID_HANDLE_VALUE){
		throw exception("NSIO failed to get the module handle to ns2.exe to find ns2 root directory");
	}

	wstring ExePath;

	ExePath.resize(MAX_PATH, 0);

	int pathLen = GetModuleFileName(hmodule, (wchar_t*)ExePath.c_str(), MAX_PATH);

	if(pathLen > MAX_PATH) {
		ExePath.resize(pathLen);

		pathLen = GetModuleFileName(hmodule, (wchar_t*)ExePath.c_str(), pathLen);
	}

	replace(ExePath, '\\', '/');

	boostfs::path exepath = boostfs::path(ExePath);
	NSRootPath = exepath.parent_path();
}

void LuaModule::ProcessCommandline(){

	wstring NativeCommandLine = GetCommandLine();

	WStringToUTF8STLString(NativeCommandLine, CommandLine);

	int arg_postion = NativeCommandLine.find(_T("-game"));

	if(arg_postion != string::npos){
		PlatformString GameCmd = NativeCommandLine.substr(arg_postion+5);

		ParseGameCommandline(GameCmd);
	}
}

void LuaModule::ParseGameCommandline(PlatformString& CommandLine ){

		if(CommandLine.length() < 2){
			throw exception("failed to parse \"game\" command line argument(find arg start)");
		}

		bool HasQuotedString = false;

		int startpos = CommandLine.find_first_not_of(_T(" =\t"));

		if(startpos == PathString::npos){
			throw exception("failed to parse \"game\" command line argument(find arg start)");
		}

		int end = PathString::npos;
		
		if(CommandLine[startpos] == '\"') {
			++startpos;
			HasQuotedString = true;

			//find the closing quote
			end = CommandLine.find_first_of('\"', startpos);

			if(end == PathString::npos){
				throw exception("failed to parse \"game\" command line argument(can't find closing quote)");
			}
		}else{
			end = CommandLine.find_first_of(_T(" \t"), startpos);
		}

		//end could equal string::npos if the -game string was right at the end of the commandline
		PlatformString GameCmdPath = CommandLine.substr(startpos, end != PathString::npos ? end-startpos : PathString::npos);

		replace(GameCmdPath, '\\', '/');

		int FirstSlash = GameCmdPath.find('/');

		GameIsZip = false;

		if(FirstSlash == PathString::npos || FirstSlash == GameCmdPath.size()-1){
			GameStringPath = NSRootPath/GameCmdPath;
		}else{
			GameStringPath = GameCmdPath;

			//if the path has no root name i.e. "somedir\mymod" treat it as relative to NSRoot
			if(!GameStringPath.has_root_name()){
				GameStringPath = NSRootPath/GameCmdPath;
			}
		}

		auto ext = GameStringPath.extension().wstring();
		boost::to_lower(ext);

		if(!boostfs::exists(GameStringPath)){
			if(!ext.empty()){
				throw exception("Zip file passed to -game command line does not exist");
			}else{
				throw exception("Directory passed to -game command line does not exist");
			}
		}else{
			if(boostfs::is_directory(GameStringPath)){
				GameIsZip = false;
			}else{
				if(ext != _T(".zip")){
					throw exception("Only zip files can be passed to -game command line");
				}
				GameIsZip = true;
			}
		}
}


string LuaModule::GetCommandLineRaw(){
	return CommandLine;
}

bool LuaModule::DirectoryExists(const PathStringArg& path) {

  bool FileFound = false;

  BOOST_FOREACH(FileSource& source, RootDirs){
    if(source.DirectoryExists(path)) return true;
  }

  return false;
}

bool LuaModule::FileExists(const PathStringArg& path) {

	bool FileFound = false;

	BOOST_FOREACH(FileSource& source, RootDirs){
		if(source.FileExists(path)) return true;
	}

	return false;
}

double LuaModule::GetFileSize(const PathStringArg& path) {

	bool FileFound = false;
	double FileSize = 0;

		BOOST_FOREACH(FileSource& source, RootDirs){
			if(source.FileSize(path, FileSize)){
				return FileSize;
			}
		}

	throw exception("cannot get the size of a file that does not exist");
}

int LuaModule::GetDateModified(const PathStringArg& path){

	bool FileFound = false;
	int32_t ModifiedTime = 0;

		BOOST_FOREACH(FileSource& source, RootDirs){
			if(source.GetModifiedTime(path, ModifiedTime)){
				return ModifiedTime;
			}
		}

	throw exception("cannot get the Date Modified of a file that does not exist");
}

luabind::object LuaModule::FindFiles( lua_State* L, const PathStringArg& SearchPath, const PathStringArg& NamePatten ){

	FileSearchResult Results;

	ConvertAndValidatePath(SearchPath);
	ConvertAndValidatePath(NamePatten);

	BOOST_FOREACH(FileSource& source, RootDirs){
		source.FindFiles(SearchPath, NamePatten, Results);
	}

	lua_createtable(L, Results.size(), 0);
	luabind::object table = luabind::object(luabind::from_stack(L,-1));

	auto it = Results.begin();

	for(uint32_t i = 0; i < Results.size() ; i++,it++){
		table[it->first] =  it->second;
	}

	return table;
}

luabind::object LuaModule::FindDirectorys( lua_State* L, const PathStringArg& SearchPath, const PathStringArg& NamePatten ){

	FileSearchResult Results;

	ConvertAndValidatePath(SearchPath);
	ConvertAndValidatePath(NamePatten);

	BOOST_FOREACH(FileSource& source, RootDirs){
		source.FindDirectorys(SearchPath, NamePatten, Results);
	}

	lua_createtable(L, Results.size(), 0);
	luabind::object table = luabind::object(luabind::from_stack(L,-1));

	auto it = Results.begin();

	for(uint32_t i = 0; i < Results.size() ; i++,it++){
		table[it->first] =  it->second;
	}

	return table;
}

luabind::object LuaModule::GetDirRootList(lua_State *L) {

	lua_createtable(L, RootDirs.size(), 0);
	luabind::object table = luabind::object(luabind::from_stack(L,-1));

	auto it = RootDirs.begin();

	for(uint32_t i = 0; i < RootDirs.size() ; i++,it++){
		table[i+1] = luabind::object(L, &RootDirs[i]);
	}

	return table;
}

boost::shared_ptr<FileSource> LuaModule::OpenArchive2(lua_State* L, const PathStringArg& Path){
	return OpenArchive(L, NULL, Path);
}

boost::shared_ptr<FileSource> LuaModule::OpenArchive(lua_State* L, FileSource* ContainingSource, const PathStringArg& Path){

	if(SevenZipLib == NULL){
		throw exception("Cannot open archive because the 7zip library is not loaded");
	}

	auto NativePath = ConvertAndValidatePath(Path);

	if(ContainingSource == NULL){
		BOOST_FOREACH(FileSource& source, RootDirs){
			if(source.FileExists(Path)){
				ContainingSource = &source;
			 break;
			}
		}

		if(ContainingSource == NULL){
			throw exception("Cannot find Archive to open");
		}
	}

	DirectoryFileSource *dirsource = dynamic_cast<DirectoryFileSource *>(ContainingSource);
	
	if (NULL == dirsource){
		throw exception("Cannot open a archive from a non DirectoryFileSource");
	}

	auto FullPath = dirsource->GetFileSystemPath()/NativePath;

	if(!boostfs::exists(FullPath)){
		throw exception("Cannot find Archive to open");
	}

	return SourceManager::OpenArchive(FullPath)->GetLuaPointer();
}

luabind::object LuaModule::GetSupportedArchiveFormats(lua_State *L) {

  auto formats = SevenZipLib->GetSupportedFormats();

  lua_createtable(L, 0, RootDirs.size()*2);
  luabind::object table = luabind::object(luabind::from_stack(L,-1));

  for(uint32_t i = 0; i < formats.size() ; i++){
    table[formats[i]] = true;
  }

  return table;
}

bool LuaModule::IsRootFileSource(FileSource* source){

	for(int i = 0; i < (int)RootDirs.size() ; i++){
		if(&RootDirs[i] == source){
			return true;
		}
	}

	return false;
}

const string& LuaModule::GetGameString(){
	return GameString;
}

int LuaModule::LoadLuaDllModule(lua_State* L, FileSource* Source, const PathStringArg& DllPath){
  
  DirectoryFileSource *dirsource = dynamic_cast<DirectoryFileSource *>(Source);

  if (NULL == dirsource){
    throw exception("Cannot load a dll module from a non DirectoryFileSource");
  }
  
  auto ModulePath = *dirsource/ConvertAndValidatePath(DllPath);

  auto FileName = ModulePath.filename();

  PrintMessage(L, (string("Loading lua dll module ")+DllPath.GetNormalizedPath()).c_str());

  string EntryPoint = string("luaopen_")+FileName.replace_extension().string();

  //LoadDLL();

  int stackTop = lua_gettop(L);

  lua_getfield(L, LUA_GLOBALSINDEX, "package");
  lua_getfield(L, -1, "loadlib");
  lua_remove(L, -2);

  
  wchar_t Dummy[2];
  int pathLength = GetDllDirectory(0, (LPWSTR)&Dummy);
  
  wstring oldPath(pathLength, 0);
 
  if(pathLength != 0){
    GetDllDirectory(pathLength, const_cast<wchar_t*>(oldPath.c_str()));
  }

  SetDllDirectory(ModDirectory->GetFileSystemPath().c_str());
  
  
  int realLength = GetLongPathName(ModulePath.c_str(), NULL, 0);
  
  wstring RealPath;

  if(realLength != 0){
    RealPath.resize(realLength+1, 0);

    GetLongPathName(ModulePath.c_str(), const_cast<wchar_t*>(RealPath.c_str()), realLength);

    ModulePath = RealPath;
    HMODULE handle = LoadLibrary(ModulePath.c_str());
  }

  lua_pushstring(L, ModulePath.string().c_str());
  lua_pushstring(L, EntryPoint.c_str());

  lua_call(L, 2, 2);

  SetDllDirectory(oldPath.c_str());

  return 3;
}

void LuaModule::LoadLuaFile_LUA(lua_State* L, const PathStringArg& ScriptPath){
  ConvertAndValidatePath(ScriptPath);


  BOOST_FOREACH(FileSource& source, RootDirs){
    if(source.FileExists(ScriptPath)){
      int result = source.LoadLuaFile(L, ScriptPath);
      
      if(result != 0) {
        lua_pushnil(L);
        lua_insert(L, lua_gettop(L)-1);
      }
      
      return;
    }
  }
}

int LuaModule::LoadLuaFile(lua_State* L, const PlatformPath& FilePath, const char* chunkname){

	using namespace boost::iostreams;

	if(!boostfs::exists(FilePath)){
		throw exception("lua file does not exist");
	}

	mapped_file_source MappedFile;

  //mapped_file_sink out;

  //out.open("", 100, 100);

  //basic_mapped_file_params<boostfs::path> p;
  //p.flags
	/*
	have to be added cause

	path(const std::wstring& p) : narrow_(), wide_(p), is_wide_(true) { }

	path& operator=(const std::wstring& p){
	narrow_.clear();
	wide_.assign(p);
	is_wide_ = true;
	return *this;
	}
	*/

	//windows gives us an error if we try to map a 0 byte file
	//just fake loading a empty file
	if(boostfs::file_size(FilePath) == 0){
	//	return luaL_loadbuffer(L, " ", 1, FilePath.string().c_str());
	}

	//MappedFile.open(FilePath.wstring());

	int LoadResult;

  LoadResult = luaL_loadfile(L, FilePath.string().c_str());
  /*
	if(MappedFile.is_open()){
		 LoadResult = luaL_loadbuffer(L, MappedFile.data(), MappedFile.size(), chunkname);
	}else{
		throw exception("Failed to open the lua file for reading");
	}
  */
	//luaL_loadbuffer should of pushed either a function or an error message on to the stack
	return LoadResult;
}

void LuaModule::MountMapArchive(Archive* archive){
  OverrideSource->MountMapArchive(archive);
}

void LuaModule::UnMountMapArchive(){
  OverrideSource->UnmountMapArchive();
}

bool LuaModule::UnmountFile(const PathStringArg& path){
  return SourceManager::UnmountFile(path.GetNormalizedPath());
}

void LuaModule::ExtractResource(FileSource* Source, const PathStringArg& FileInArchive, const PathStringArg& DestinationPath){

  ConvertAndValidatePath(DestinationPath);
  Archive *ArchiveSource = dynamic_cast<Archive *>(Source);

  if(NULL == ArchiveSource){
    throw exception("Can only extract resource's from Archive file sources");
  }

  if(FileInArchive.GetExtension() != DestinationPath.GetExtension())throw exception("The destination file needs to have the same extension as the file from the archive");

  int index = ArchiveSource->GetFileIndex(FileInArchive.GetNormalizedPath());

  if(index == -1)throw exception();

 // SourceManager::MountFile(DestinationPath.GetNormalizedPath(), ArchiveSource, index);

 // ExtractedFileCache.ExtractResourceToPath(ArchiveSource, FileInArchive.GetNormalizedPath(), DestinationPath);
}

void LuaModule::OpenModsFolder(){
  ShellExecute(NULL, L"open", (*ModDirectory/"Mods").c_str(), NULL, NULL, SW_SHOWNORMAL);
}






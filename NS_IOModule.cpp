//NS2_IO by fsfod

#include "stdafx.h"
#include "NS_IOModule.h"
#include "StringUtil.h"
#include "C7ZipLibrary.h"
#include "PathStringConverter.h"

#include <boost/algorithm/string/case_conv.hpp>
#include <boost/algorithm/string/trim.hpp>
#include <boost/iostreams/device/mapped_file.hpp>
#include <boost/iostreams/stream.hpp>

using namespace  std;

PlatformPath NSRootPath(_T(""));
C7ZipLibrary* SevenZip = NULL;

//LuaModule::LuaModule(){
string LuaModule::CommandLine("");
string LuaModule::GameString("");
PlatformPath LuaModule::GameStringPath(_T(""));;
boost::ptr_vector<FileSource> LuaModule::RootDirs(0);

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

  if(instance != NULL){
    instance->~LuaModule();
  }

  return 0;
}

void LuaModule::Initialize(lua_State *L){

  LuaModule* vm = new (lua_newuserdata(L, sizeof(LuaModule))) LuaModule(L);
  lua_setfield(L, LUA_REGISTRYINDEX, "NS2_IOState");

  StaticInit(L);
}

bool FirstLoad = true;

void LuaModule::StaticInit(lua_State* L){

  if(!FirstLoad)return;

  FirstLoad = false;

  FindNSRoot();

  //if -game is set 
  ProcessCommandline();

  auto ServerZipPath = (GameStringPath.empty() ? NSRootPath : GameStringPath) /"7z.dll";

  if(boostfs::exists(ServerZipPath)){
    try{
      SevenZip = C7ZipLibrary::Init(ServerZipPath);
    }catch(exception e){

      string msg = "error while loading 7zip library: ";
      msg += e.what();
      PrintMessage(L, msg.c_str());

      SevenZip = NULL;
    }

    if(SevenZip != NULL && GameIsZip){
      try{
        RootDirs.push_back(SevenZip->OpenArchive(GameStringPath));
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
   if(SevenZip == NULL) PrintMessage(L, "cannot open archive set with -game when 7zip is not loaded");
  }else{
    if(!GameStringPath.empty()) RootDirs.push_back(new DirectoryFileSource(GameStringPath, "", true));
  }

  RootDirs.push_back(new DirectoryFileSource("ns2"));
  RootDirs.push_back(new DirectoryFileSource("core"));

  auto directorystxt = NSRootPath/"directories.txt";

  if(boostfs::exists(directorystxt)){
    ProcessDirectoriesTXT(directorystxt);
  }
}

LuaModule* LuaModule::GetInstance(lua_State* L){
  
  lua_getfield(L, LUA_REGISTRYINDEX, "NS2_IOState");
  
  LuaModule* instance = (LuaModule*)lua_touserdata(L, -1);

  if(instance == NULL){
    throw exception("failed to get NS2_IOState");
  }

  return instance;
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
				RootDirs.push_back(new DirectoryFileSource(path/"ns2", "ns2", true));
				RootDirs.push_back(new DirectoryFileSource(path/"core", "core", true));
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

FileSource* LuaModule::OpenArchive2(lua_State* L, const PathStringArg& Path){
	return OpenArchive(L, NULL, Path);
}

FileSource* LuaModule::OpenArchive(lua_State* L, FileSource* ContainingSource, const PathStringArg& Path){

	if(SevenZip == NULL){
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

	return SevenZip->OpenArchive(FullPath);
}

bool LuaModule::IsRootFileSource(FileSource* source){

	for(int i = 0; i < RootDirs.size() ; i++){
		if(&RootDirs[i] == source){
			return true;
		}
	}

	return false;
}

	const string& LuaModule::GetGameString(){
	return GameString;
}

int LuaModule::LoadLuaFile( lua_State* L, const PlatformPath& FilePath, const char* chunkname )
{

	using namespace boost::iostreams;

	if(!boostfs::exists(FilePath)){
		throw exception("lua file does not exist");
	}

	mapped_file_source MappedFile;

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
		return luaL_loadbuffer(L, " ", 1, FilePath.string().c_str());
	}

	MappedFile.open(FilePath.wstring());

	int LoadResult;

	if(MappedFile.is_open()){
		 LoadResult = luaL_loadbuffer(L, MappedFile.data(), MappedFile.size(), FilePath.string().c_str());
	}else{
		throw exception("Failed to open the lua file for reading");
	}

	//luaL_loadbuffer should of pushed either a function or an error message on to the stack
	return LoadResult;
}










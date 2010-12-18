//NS2_IO by fsfod

#include "stdafx.h"
#include "NS_IOModule.h"
#include "StringUtil.h"
#include "C7ZipLibrary.h"
#include "PathStringConverter.h"

#include <boost/algorithm/string/case_conv.hpp>
#include <boost/iostreams/device/mapped_file.hpp>


using namespace  std;

PlatformPath NSRootPath(_T(""));
C7ZipLibrary* SevenZip = NULL;

//LuaModule::LuaModule(){
string LuaModule::CommandLine("");
string LuaModule::GameString("");
PlatformPath LuaModule::GameStringPath(_T(""));;
boost::ptr_vector<FileSource> LuaModule::RootDirs(0);
//{

//PathMatchSpecEx for matching files names on windows
//fnmatch on linux will do the same directorys 


void LuaModule::Initialize(lua_State *L){
	
	FindNSRoot();

	auto ServerZipPath = NSRootPath/"7z.dll";

	if(boostfs::exists(ServerZipPath)){
		try{
			SevenZip = C7ZipLibrary::Init(ServerZipPath);

			//SevenZip->OpenArchive(PathString(_T("I:\\ns2stuff\\NS2_IO\\stuff.zip")));
		}catch(exception e){
			
		}
	}

	AddMainFileSources();

	auto directorystxt = NSRootPath/"directorys.txt";

	if(boostfs::exists(directorystxt)){
		//TODO handle directorys.txt
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

void LuaModule::ParseGameCommandline(PlatformString& CommandLine )
{

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

		bool IsZip = false;

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
				IsZip = false;
			}else{
				if(ext != _T(".zip")){
					throw exception("Only zip files can be passed to -game command line");
				}
				IsZip = true;
			}
		}



		if(!IsZip){
			RootDirs.push_back(new DirectoryFileSource(GameStringPath, ""));
		}else{
			RootDirs.push_back(SevenZip->OpenArchive(GameStringPath));
		}

}

void LuaModule::AddMainFileSources(){

	wstring NativeCommandLine = GetCommandLine();

	WStringToUTF8STLString(NativeCommandLine, CommandLine);

	int arg_postion = NativeCommandLine.find(_T("-game"));

	if(arg_postion != string::npos){
		PlatformString GameCmd = NativeCommandLine.substr(arg_postion+5);

		ParseGameCommandline(GameCmd);
	}

	RootDirs.push_back(new DirectoryFileSource("ns2"));
	RootDirs.push_back(new DirectoryFileSource("core"));
}

string LuaModule::GetCommandLineRaw(){
	return CommandLine;
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
		table[it->second] =  it->first;
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
		table[it->second] = it->first.c_str();
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
			if(source.FileExists(Path)) ContainingSource = &source;
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

FileSource* LuaModule::GetRootDirectory(int index) {
	
	if(index < 0 || (size_t)index >= RootDirs.size()) {
		throw exception("GetRootDirectory Directory index out of range");
	}

	return &RootDirs[index];
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

	boost::iostreams::mapped_file_source MappedFile;

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
		return luaL_loadbuffer(L, " ", 1, "");
	}

	MappedFile.open(FilePath.wstring());

	int LoadResult;

	if(MappedFile.is_open()){
		 LoadResult = luaL_loadbuffer(L, MappedFile.data(), MappedFile.size(), "");
	}else{
		throw exception("Failed to open the lua file for reading");
	}

	//luaL_loadbuffer should of pushed either a function or an error message on to the stack
	return LoadResult;
}










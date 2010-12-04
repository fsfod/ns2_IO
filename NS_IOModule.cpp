//NS2_IO by fsfod

#include "stdafx.h"
#include "NS_IOModule.h"
#include "StringUtil.h"
#include <algorithm>
namespace boostfs = boost::filesystem;

#include "PathStringConverter.h"
#include "StringUtil.h"


using namespace  std;

//LuaModule::LuaModule(){
PathString LuaModule::CommandLine(_T(""));
PathString LuaModule::NSRoot(_T(""));
PathString LuaModule::GameString(_T(""));;
boost::ptr_vector<FileSource> LuaModule::RootDirs(0);
//{

//PathMatchSpecEx for matching files names on windows
//fnmatch on linux will do the same directorys 


void LuaModule::Initialize(lua_State *L){
	
	FindNSRoot();
	FindDirectoryRoots();

	PathString directorystxt = NSRoot+_T("directorys.txt");

	if(boostfs::exists(directorystxt)){
		
	}
	
}

void LuaModule::FindNSRoot(){
	HMODULE hmodule = GetModuleHandleW(NULL);

	if(hmodule == INVALID_HANDLE_VALUE){
		throw LuaErrorWrapper("NSIO failed to get the module handle to ns2.exe to find ns2 root directory");
	}
	NSRoot.resize(MAX_PATH, 0);

	int pathLen = GetModuleFileName(hmodule, (wchar_t*)NSRoot.c_str(), MAX_PATH);

	if(pathLen > MAX_PATH) {
		NSRoot.resize(pathLen);

		pathLen = GetModuleFileName(hmodule, (wchar_t*)NSRoot.c_str(), pathLen);
	}
	
	boostfs::wpath exepath = boostfs::wpath(NSRoot);
	NSRoot = exepath.parent_path().string();

	replace(NSRoot.begin(), NSRoot.end(), '\\', '/');

	NSRoot.push_back('/');

}

void LuaModule::ParseGameCommandline(PathString& CommandLine){

	if(CommandLine.length() < 2  || CommandLine[0] != ' ' || CommandLine[0] == '=' ){
		throw exception("failed to parse \"game\" command line argument(find arg start)");
	}

		auto it = CommandLine.begin();

		bool HasQuotedString = false, Found = false;

		do{
			if(*it == '\"'){
				Found = true;
				HasQuotedString = true;
				break;
			}

			if(*it != ' ' &&  *it != '=' &&  *it != '\t'){
				Found = true;
				break;
			}
		}while(++it != CommandLine.end());

		auto PathArgStart = it;
		int startpos = it-CommandLine.begin();

		it++;

		Found = false;
		do{
			if(HasQuotedString){
				if(*it == '\"')break;
				Found = true;
			}else{
				if(*it == ' ' ||  *it == '\t'){
					Found = true;
					break;
				}
			}
		}while(++it != CommandLine.end());

		if((HasQuotedString && !Found) || (!HasQuotedString && it != CommandLine.end()) ){
			throw exception("failed to parse game command line argument(formating2)");
		}

		GameString = CommandLine.substr(startpos, it-PathArgStart);

		if(GameString.find(':') == string::npos){
			RootDirs.push_back(static_cast<FileSource*>(new NSRootDir(NSRoot, GameString.c_str()) ));
		}else{
			replace(GameString.begin(), GameString.end(), '\\', '/');

			int index = GameString.rfind('/');

			if(index == GameString.size()-1){
				index = GameString.rfind('/', GameString.size()-1);
			}

			if(index == string::npos) {
				throw exception("failed to parse game command line argument(path)");
			}

			PathString GameStringPath = GameString.substr(0, index+1);
			PathString DirName = GameString.substr(index+1, GameString.size() - index+1);

			if(DirName.back() == '/'){
				DirName.resize(DirName.size()-1);
			}

			RootDirs.push_back(new NSRootDir(GameStringPath, DirName.c_str()));
		}

		RootDirs.push_back(new NSRootDir(NSRoot, "ns2"));
		RootDirs.push_back(new NSRootDir(NSRoot, "core"));
}



void LuaModule::FindDirectoryRoots(){

	CommandLine.assign(GetCommandLine());

	int arg_postion = CommandLine.find(_T("-game"));

	if(arg_postion != string::npos){
		PathString GameCmd = CommandLine.substr(arg_postion+5);

		ParseGameCommandline(GameCmd);
	}else{
		RootDirs.push_back(new NSRootDir(NSRoot, "ns2"));
		RootDirs.push_back(new NSRootDir(NSRoot, "core"));
	}
}

string LuaModule::GetCommandLineRaw(){
	string s;
	WStringToUTF8STLString(CommandLine, s);
	
	return s;
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

luabind::object LuaModule::FindFiles( lua_State* L, const PathStringArg SearchPath, const PathStringArg& NamePatten ){

	FileSearchResult Results;

	BOOST_FOREACH(FileSource& source, RootDirs){
		source.FindFiles(SearchPath, NamePatten, Results);
	}

	lua_createtable(L, Results.size(), 0);
	luabind::object table = luabind::object(luabind::from_stack(L,-1));

	auto it = Results.begin();

	for(uint32_t i = 0; i < Results.size() ; i++,it++){
		table[i+1] =  it->first;
	}

	return table;
}

luabind::object LuaModule::FindDirectorys( lua_State* L, const PathStringArg SearchPath, const PathStringArg& NamePatten )
{

	FileSearchResult Results;

	BOOST_FOREACH(FileSource& source, RootDirs){
		source.FindDirectorys(SearchPath, NamePatten, Results);
	}

	lua_createtable(L, Results.size(), 0);
	luabind::object table = luabind::object(luabind::from_stack(L,-1));

	auto it = Results.begin();

	for(uint32_t i = 0; i < Results.size() ; i++,it++){
		table[i+1] = it->first.c_str();
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

FileSource* LuaModule::GetRootDirectory(int index) {
	
	if(index < 0 || (size_t)index >= RootDirs.size()) {
		throw exception("GetRootDirectory Directory index out of range");
	}

	return &RootDirs[index];
}

const PathStringArg& LuaModule::GetGameString() {
	return static_cast<PathStringArg&>(GameString);
}










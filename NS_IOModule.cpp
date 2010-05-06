//NS2_IO by fsfod

#include "stdafx.h"
#include "NS_IOModule.h"
#include "StringUtil.h"
#include <algorithm>
#include "PathStringConverter.h"
using namespace  std;

//LuaModule::LuaModule(){
PathString LuaModule::NSRoot(_T(""));
PathString LuaModule::GameString(_T(""));;
std::vector<NSRootDir> LuaModule::RootDirs(0);
//{


bool LuaModule::FindNSRoot(){
	HMODULE hmodule = GetModuleHandleA("ns2.exe");

	if(hmodule == INVALID_HANDLE_VALUE){
		throw LuaErrorWrapper("NSIO failed to get the module handle to ns2.exe to find ns2 root directory");
	}
	NSRoot.resize(MAX_PATH, 0);

	int pathLen = GetModuleFileName(hmodule, (wchar_t*)NSRoot.c_str(), MAX_PATH);

	if(pathLen > MAX_PATH) {
		NSRoot.resize(pathLen);

		pathLen = GetModuleFileName(hmodule, (wchar_t*)NSRoot.c_str(), pathLen);
	}

	for_each(NSRoot.begin(), NSRoot.end(), [](PathString::value_type& c){if(c == '\\')c = '/';});

	int index = NSRoot.find_last_of('/');

	if(index != -1) {
		NSRoot.resize(index, 0);
		NSRoot.push_back('/');
	}

	return true;
}

void LuaModule::ParseGameCommandline(PathString& CommandLine){

	if(CommandLine.length() < 2  || CommandLine[0] != ' ' || CommandLine[0] == '=' ){
		throw LuaErrorWrapper("failed to parse game command line argument(find arg start)");
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

		if(!(HasQuotedString && Found) || it != CommandLine.end()){
			throw LuaErrorWrapper("failed to parse game command line argument(formating2)");
		}

		GameString = CommandLine.substr(startpos, it-PathArgStart);

		if(GameString.find(':') == -1){
			RootDirs.push_back(NSRootDir(NSRoot, GameString.c_str()));
		}else{
			for_each(GameString.begin(), GameString.end(), [](wchar_t& c){if(c == '\\')c = '/';});

			int index = GameString.find_last_of('/');

			if(index == GameString.size()-1){
				GameString[GameString.size()-1] = 0;
				index = GameString.find_last_of('/');
				GameString[GameString.size()-1] = '/';
			}

			if(index == -1) {
				throw LuaErrorWrapper("failed to parse game command line argument(path)");
			}

			PathString GameStringPath = GameString.substr(0, index+1);
			PathString DirName = GameString.substr(index+1, GameString.size() - index+1);

			if(*--DirName.end() == '/'){
				DirName.resize(DirName.size()-1);
			}

			RootDirs.push_back(NSRootDir(GameStringPath, DirName.c_str()));
		}

		RootDirs.push_back(NSRootDir(NSRoot, "core"));
		RootDirs.push_back(NSRootDir(NSRoot, "n2"));
}



void LuaModule::FindDirectoryRoots(){

	PathString CommandLine(GetCommandLine());

	int arg_postion = CommandLine.find(_T("//game"));

	if(arg_postion != -1){
		PathString CommandLine = CommandLine.substr(arg_postion);

		ParseGameCommandline(CommandLine);
	}else{
		RootDirs.push_back(NSRootDir(NSRoot, "core"));
		RootDirs.push_back(NSRootDir(NSRoot, "ns2"));
	}
}


bool LuaModule::FileExists(const PathStringArg& path) {

	bool FileFound = false;

	for(int i = 0; i < 2 ; i++){
		if(RootDirs[i].FileExists(path)) return true;
	}

	return false;
}

uint32_t LuaModule::GetFileSize(const PathStringArg& path) {

	bool FileFound = false;
	uint32_t FileSize = 0;

	for(auto it = RootDirs.begin(); it < RootDirs.end() ; it++){
		if(it->FileSize(path, FileSize)){
			return FileSize;
		}
	}

	throw exception("cannot get the size of a file that does not exist");
}

int LuaModule::GetDateModifed(const PathStringArg& path){

		bool FileFound = false;
		uint32_t ModifedTime = 0;

		for(auto it = RootDirs.begin(); it < RootDirs.end() ; it++){
			if(it->GetModifedTime(path, ModifedTime)){
				return ModifedTime;
			}
		}

	throw exception("cannot get the size of a file that does not exist");
}

luabind::object LuaModule::FindFiles(lua_State* L, const PathStringArg& SearchString) {

	FileSearchResult Results;

	for_each(RootDirs.begin(), RootDirs.end(),
		[&](NSRootDir& dir){
			dir.FindFiles(SearchString, Results);
		}
	);

	lua_createtable(L, Results.size(), 0);
	luabind::object table = luabind::object(luabind::from_stack(L,-1));

	auto it = Results.begin();

	for(uint32_t i = 0; i < Results.size() ; i++,it++){
		table[i+1] =  it->first;
	}

	return table;
}

luabind::object LuaModule::FindDirectorys(lua_State *L, const PathStringArg& SearchString) {

	FileSearchResult Results;

	for_each(RootDirs.begin(), RootDirs.end(),
		[&](NSRootDir& dir){
			dir.FindDirectorys(SearchString, Results);
		}
	);

	lua_createtable(L, Results.size(), 0);
	luabind::object table = luabind::object(luabind::from_stack(L,-1));

	auto it = Results.begin();

	for(uint32_t i = 0; i < Results.size() ; i++,it++){
		table[i+1] =  it->first.c_str();
	}

	return table;
}

luabind::object LuaModule::GetDirRootList(lua_State *L) {

	lua_createtable(L, RootDirs.size(), 0);
	luabind::object table = luabind::object(luabind::from_stack(L,-1));

	auto it = RootDirs.begin();

	for(uint32_t i = 0; i < RootDirs.size() ; i++,it++){
#ifdef UnicodePathString
		//string Apath;
		//UTF16ToUTF8STLString(RootDirs[i].GetPath().c_str(), Apath);
		table[i+1] = static_cast<PathStringArg&>(RootDirs[i].GetPath());
#else
		const string& temp = (*it).GetPath();
		table[i+1] =  temp;
#endif
	}

	return table;
}

const PathStringArg& LuaModule::GetGameString() {
	return static_cast<PathStringArg&>(GameString);
}










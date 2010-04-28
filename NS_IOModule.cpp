//NS2_IO by fsfod

#include "stdafx.h"
#include "NS_IOModule.h"
#include "StringUtil.h"
#include <algorithm>

using namespace  std;

//LuaModule::LuaModule(){
PathString LuaModule::NSRoot = PathString(0);
PathString LuaModule::GameString = PathString(0);
std::vector<NSRootDir> LuaModule::RootDirs = std::vector<NSRootDir>(0);
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

	if(CommandLine.length() > 2  && CommandLine[0] == ' ' || CommandLine[0] == '=' ){
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
	}else{
		throw LuaErrorWrapper("failed to parse game command line argument(formating1)");
	}
}

void LuaModule::FindDirectoryRoots(){

	wchar_t* arg_postion = wcsstr(GetCommandLine(), L"/game");
	PathString CommandLine(GetCommandLine());


	if(arg_postion != NULL){
		PathString CommandLine(arg_postion+5);

		ParseGameCommandline(CommandLine);
	}else{
		RootDirs.push_back(NSRootDir(NSRoot, "core"));
		RootDirs.push_back(NSRootDir(NSRoot, "ns2"));
	}
}

void LuaModule::ConvertAndValidatePath(lua_State *L, int ArgumentIndex, PathString& path){
	size_t StringSize = 0;
	const char* string = luaL_checklstring (L, ArgumentIndex, &StringSize);

	if(string == NULL){
		throw LuaErrorWrapper("Missing string argument TODO");
	}

	if(string[0] == '/' || string[0] == '\\'){
		string = string+ 1;
	}

#if defined(UnicodePathString)
	int result = UTF8ToWString(string, path);
#else
	path.assign(string);
#endif

	if(path.find(L"..") != -1){
		throw LuaErrorWrapper("The Path cannot contain the directory up string \"..\"");
	}
}

int LuaModule::FileExists(lua_State *L) {

	try{
		PathString path;
		ConvertAndValidatePath(L, 1, path);


		bool FileFound = false;

		for(int i = 0; i < 2 ; i++){
			FileFound = RootDirs[i].FileExists(path);
			if(FileFound) break;
		}

		lua_pushboolean(L, FileFound);

		return 1;
	}catch(LuaErrorWrapper e){
		lua_pushstring(L, e.what());
		return 0;
	}
}

int LuaModule::GetFileSize(lua_State *L) {

	try{
		PathString path;
		ConvertAndValidatePath(L, 1, path);

		bool FileFound = false;
		uint32_t FileSize = 0;

		for(int i = 0; i < 2 ; i++){
			FileFound = RootDirs[i].FileSize(path, FileSize);
			if(FileFound) break;
		}

		if(FileFound){
			lua_pushnumber(L, FileSize);
		}else{
			lua_pushstring(L, "cannot get the size of a file that does not exist");
			lua_error(L);

		 return 0;
		}

		return 1;
	}catch(LuaErrorWrapper e){
		lua_pushstring(L, e.what());
		return 0;
	}
}

int LuaModule::GetDateModifed(lua_State* L){
	try{
		PathString path;
		ConvertAndValidatePath(L, 1, path);

		bool FileFound = false;
		uint64_t ModifedTime = 0;

		for(int i = 0; i < 2 ; i++){
			FileFound = RootDirs[i].GetModifedTime(path, ModifedTime);
			if(FileFound) break;
		}

		if(FileFound){
			lua_pushnumber(L, FileSize);
		}else{
			lua_pushstring(L, "cannot get the size of a file that does not exist");
			lua_error(L);

			return 0;
		}

		return 1;
	}catch(LuaErrorWrapper e){
		lua_pushstring(L, e.what());
		return 0;
	}
}
}

int LuaModule::FindFilesInDir(lua_State *L) {
	PathString SearchString;
	ConvertAndValidatePath(L, 1, SearchString);

	FileSearchResult Results;

	for(int i = 0; i < RootDirs.size() ; i++){
		int count = RootDirs[i].FindFiles(SearchString, Results);
	}

	lua_createtable(L, Results.size(), 0);
	int Table = lua_gettop(L);

	auto it = Results.begin();

	for(int i = 0; i < Results.size() ; i++,it++){
		lua_pushstring(L, it->first.c_str());
		lua_rawseti(L, Table, i+1);
	}

	return 1;
}

int LuaModule::FindDirectorys(lua_State *L) {
	PathString SearchString;
	
	ConvertAndValidatePath(L, 1, SearchString);

	FileSearchResult Results;

	for(unsigned int i = 0; i < RootDirs.size() ; i++){
		int count = RootDirs[i].FindDirectorys(SearchString, Results);
	}

	lua_createtable(L, Results.size(), 0);
	int Table = lua_gettop(L);

	auto it = Results.begin();

	for(unsigned int i = 0; i < Results.size() ; i++,it++){
		lua_pushstring(L, it->first.c_str());
		lua_rawseti(L, Table, i+1);
	}

	return 1;
}

int LuaModule::GetDirRootList(lua_State *L) {

	lua_createtable(L, RootDirs.size(), 0);
	int Table = lua_gettop(L);
	string Apath;

	for(unsigned int i = 0; i < RootDirs.size() ; i++){

#ifdef UnicodePathString
		UTF16ToUTF8STLString(RootDirs[i].GetPath().c_str(), Apath);
		lua_pushstring(L, Apath.c_str());
#else
		lua_pushstring(L, RootDirs[i].GetPath().c_str());
#endif

		lua_rawseti(L, Table, i+1);
	}

	return 1;
}

int LuaModule::GetGameString(lua_State *L) {
	string AGameString;
	
	if(GameString.empty()){
		lua_pushstring(L, "");
	}else{
		UTF16ToUTF8STLString(GameString.c_str(), AGameString);
		lua_pushstring(L, AGameString.c_str());
	}

	return 1;
}

/*
		if (luaL_loadfile(L, fullPath.c_str()) == 0)
		{
			//Set the chunk's environment to a table
			lua_newtable(L);
			lua_pushvalue(L, -1);
			if (!lua_setfenv(L, -3))
			{
				assert(false && "Chunk must be loaded");
			}
			//Push the chunk to the top, required by pcall
			lua_pushvalue(L, -2);
			//Remove the old chunk
			lua_remove(L, -3);
			//Call the chunk, it then inserts any globals into the environment table
			if (lua_pcall(L, 0, 0, 0) == 0)
			{
				TOCInfo::StringCollection& savedVars = info.SavedVariables;
				for (TOCInfo::StringCollection::const_iterator it = savedVars.begin(), end = savedVars.end(); it != end; ++it)
				{
					lua_getfield(L, -1, it->c_str());
					if (lua_isnil(L, -1))
					{
						//Pop nil
						lua_pop(L, 1);
					}
					else
					{
						//Pop and place the value into the globals
						lua_setfield(L, LUA_GLOBALSINDEX, it->c_str());
					}
				}
				//Pop environment table
				lua_pop(L, 1);
				return;
			}
			error(L);
		}
}*/








//NS2_IO by fsfod

#include "stdafx.h"
#include "NS_IOModule.h"
#include "NSRootDir.h"
#include "StringUtil.h"
#include <algorithm>

using namespace  std;


vector<NSRootDir> RootDirs(0);
int RootDirCount = 0;
wstring NSRoot, GameString;

bool FindDirectoryRoots(lua_State* L){
	HMODULE hmodule = GetModuleHandleA("ns2.exe");

	if(hmodule == INVALID_HANDLE_VALUE){
		lua_pushstring(L, "NSIO failed to get the module handle to ns2.exe to find ns2 root directory");
		lua_error(L);
		return false;
	}

	NSRoot.resize(MAX_PATH, 0);

	int pathLen = GetModuleFileNameW(hmodule, (wchar_t*)NSRoot.c_str(), MAX_PATH);

	if(pathLen > MAX_PATH) {
		NSRoot.resize(pathLen);
		NSRoot = new  wchar_t[pathLen+1];

		pathLen = GetModuleFileNameW(hmodule, (wchar_t*)NSRoot.c_str(), pathLen);
	}
	
	for_each(NSRoot.begin(), NSRoot.end(), [](wchar_t& c){if(c == '\\')c = '/';});

	int index = NSRoot.find_last_of('/');
	
	if(index != -1) {
		NSRoot.resize(index, 0);
		NSRoot.push_back('/');
	}

	wchar_t* arg_postion = wcsstr(GetCommandLineW(), L"/game");

	if(arg_postion != NULL){
		wstring CommandLine(arg_postion+5);

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

				if((HasQuotedString && Found) || it == CommandLine.end()){
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
							lua_pushstring(L, "failed to parse game command line argument(path)");
							lua_error(L);
						 return false;
						}
						
						wstring GameStringPath = GameString.substr(0, index+1);
						wstring DirName = GameString.substr(index+1, GameString.size() - index+1);

						if(*--DirName.end() == '/'){
							DirName.resize(DirName.size()-1);
						}

						RootDirs.push_back(NSRootDir(GameStringPath, DirName.c_str()));
					}
					RootDirs.push_back(NSRootDir(NSRoot, L"core"));
					RootDirs.push_back(NSRootDir(NSRoot, L"ns2"));
				}else{
					lua_pushstring(L, "failed to parse game command line argument(formating2)");
					lua_error(L);
				 return false;
				}
		}else{
			lua_pushstring(L, "failed to parse game command line argument(formating1)");
			lua_error(L);
			return false;
		}
	}else{
		RootDirs.push_back(NSRootDir(NSRoot, L"core"));
		RootDirs.push_back(NSRootDir(NSRoot, L"ns2"));
	}

	return true;
}


bool ConvertAndValidatePath(lua_State* L, int ArgumentIndex, wstring& path){
	size_t StringSize = 0;
	const char* string = luaL_checklstring (L, ArgumentIndex, &StringSize);

	if(string == NULL){
		return false;
	}

	if(string[0] == '/' || string[0] == '\\'){
		string = string+ 1;
	}

	int result = UTF8ToWString(string, path);

	if(path.find(L"..") != -1){
		lua_pushstring(L, "The Path cannot contain the directory up string \"..\"");
		lua_error(L);
		return false;
	}

	return true;
}

LUALIB_API int FileExists(lua_State *L) {
	wstring path;
	if(!ConvertAndValidatePath(L, 1, path)){
		return 0;
	}

	bool FileFound = false;

	for(int i = 0; i < 2 ; i++){
		FileFound = RootDirs[i].FileExists(path);
		if(FileFound) break;
	}

	lua_pushboolean(L, FileFound);

	return 1;
}

LUALIB_API int FindFilesInDir(lua_State *L) {
	wstring SearchString;
	if(!ConvertAndValidatePath(L, 1, SearchString)){
		return 0;
	}

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

LUALIB_API int FindDirectorys(lua_State *L) {
	wstring SearchString;
	if(!ConvertAndValidatePath(L, 1, SearchString)){
		return 0;
	}
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

LUALIB_API int GetDirRootList(lua_State *L) {

	lua_createtable(L, RootDirCount, 0);
	int Table = lua_gettop(L);
	string Apath;

	for(unsigned int i = 0; i < RootDirs.size() ; i++){
		UTF16ToUTF8STLString(RootDirs[i].GetPath().c_str(), Apath);
	
		lua_pushstring(L, Apath.c_str());
		lua_rawseti(L, Table, i+1);
	}

	return 1;
}

LUALIB_API int GetGameString(lua_State *L) {
	string AGameString;
	
	if(GameString.empty()){
		lua_pushstring(L, "");
	}else{
		UTF16ToUTF8STLString(GameString.c_str(), AGameString);
		lua_pushstring(L, AGameString.c_str());
	}

	return 1;
}

static const luaL_reg NSIOlib[] = {
	{"GetGameString", GetGameString},
	{"Exists", FileExists},
	{"GetDirRootList", GetDirRootList},
	{"FindFiles", FindFilesInDir},
	{"FindDirectorys", FindDirectorys},
	{NULL, NULL}
};

extern "C" __declspec(dllexport) int luaopen_NS2_IO(lua_State* L){
	 luaL_register(L, "NS2_IO", NSIOlib);

	 FindDirectoryRoots(L);

	return 1;
}



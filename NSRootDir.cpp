#include "StdAfx.h"
#include "NSRootDir.h"
#include "StringUtil.h"
#include <sys/stat.h>

using namespace std;


void NSRootDir::SetupPathStrings(const PathString& nsroot, const PathString& path){
	PathLength = path.length()+nsroot.size()+1;

	RealPath.reserve(PathLength);
	RealPath = nsroot;
	RealPath.append(path);
	RealPath.push_back('/');

	GameFileSystemPath.push_back('/');
	GameFileSystemPath.append(path);
	GameFileSystemPath.push_back('/');
}

bool NSRootDir::FileExists(const PathString& FilePath){
	PathString fullpath = RealPath+FilePath;

	int result = GetFileAttributes(FilePath.c_str());

	return result != INVALID_FILE_ATTRIBUTES;
}

bool NSRootDir::FileSize(const PathString& FilePath, uint32_t& Filesize){
	PathString fullpath = RealPath+FilePath;

	WIN32_FILE_ATTRIBUTE_DATA AttributeData;

	int result = GetFileAttributesEx(fullpath.c_str(), GetFileExInfoStandard, &AttributeData);

	if(result == 0){
		return false;
	}

	Filesize = AttributeData.nFileSizeLow;

	return true;
}

bool NSRootDir::DirectoryExists(const PathString& DirectoryPath){
	PathString fullpath = RealPath+DirectoryPath;

	int result = GetFileAttributes(fullpath.c_str());

	return result != INVALID_FILE_ATTRIBUTES && (result&FILE_ATTRIBUTE_DIRECTORY) != 0;
}

int NSRootDir::FindFiles(const PathString SearchPath, FileSearchResult& FoundFiles){
	
	PathString fullpath = RealPath+SearchPath;

	WIN32_FIND_DATAW FindData;
	memset(&FindData, 0, sizeof(FindData));

	HANDLE FHandle = FindFirstFileEx(fullpath.c_str(), FindExInfoBasic, &FindData, FindExSearchNameMatch, NULL, 0);

	if(FHandle == INVALID_HANDLE_VALUE){
		return 0;
	}

	int count = 0;

	do{
		if(wcscmp(FindData.cFileName, L".") && wcscmp(FindData.cFileName, L"..") && (FindData.dwFileAttributes&FILE_ATTRIBUTE_DIRECTORY) == 0){
			count++;
			string FileName;
			UTF16ToUTF8STLString(FindData.cFileName, FileName);

			FoundFiles[FileName] = this;
		}
	}while(FindNextFile(FHandle, &FindData));

	return count;
}


int NSRootDir::FindDirectorys(const PathString SearchPath, FileSearchResult& FoundDirectorys){

	PathString fullpath = RealPath+SearchPath;

	WIN32_FIND_DATAW FindData;
	memset(&FindData, 0, sizeof(FindData));

	HANDLE FHandle = FindFirstFileEx(fullpath.c_str(), FindExInfoBasic, &FindData, (FINDEX_SEARCH_OPS)(FindExSearchNameMatch|FindExSearchLimitToDirectories), NULL, 0);

	if(FHandle == INVALID_HANDLE_VALUE){
		return 0;
	}

	int count = 0;

	do{
		if(wcscmp(FindData.cFileName, L".") && wcscmp(FindData.cFileName, L"..") && (FindData.dwFileAttributes&FILE_ATTRIBUTE_DIRECTORY) != 0){
			string FileName;
			UTF16ToUTF8STLString(FindData.cFileName, FileName);

			FoundDirectorys[FileName] = this;
			count++;
		}
	}while(FindNextFile(FHandle, &FindData));

	return count;
}


bool NSRootDir::GetModifiedTime(const PathString& FilePath, int32_t& Time){

	PathString fullpath = RealPath+FilePath;

#ifdef UnicodePathString
	struct _stat32 FileInfo;

	int result = _wstat(fullpath.c_str(), &FileInfo);
#else
	struct stat FileInfo;

	int result = stat(fullpath.c_str(), &FileInfo);
#endif

	if(result == 0){
		return false;
	}

	Time = FileInfo.st_mtime;

	return true;
}

void NSRootDir::LoadLuaFile(lua_State* L, const PathStringArg& FilePath){

#ifdef UnicodePathString
	std:string fullpath(0);

	WStringToUTF8STLString(RealPath+FilePath, fullpath);
#else
	std:string fullpath = RealPath+FilePath;
#endif
	
	//FIXME need to handle unicode paths
	int LoadResult = luaL_loadfile(L, fullpath.c_str());

	//loadfile should of pushed either a function or an error message on to the stack leave it there as the return value
	return;
}

////////////////////////////////////////////////////////////////////
////////////////////        LUA Wrappers        ////////////////////
////////////////////////////////////////////////////////////////////


int32_t NSRootDir::Wrapper_GetModifedTime(const PathStringArg& Path){
	int time = 0;
	
	if(this->GetModifiedTime(static_cast<PathString>(Path), time)){
		throw exception("RootDir::DateModifed cannot get the modified time of a file that does not exist");
	}

	return time;
}

bool NSRootDir::Wrapper_FileExists(const PathStringArg& path){
	return this->FileExists(static_cast<const PathString&>(path));
}

uint32_t NSRootDir::Wrapper_FileSize(const PathStringArg& Path){
	uint32_t size = 0;

	if(this->FileSize(static_cast<const PathString&>(Path), size)){
		throw exception("RootDir::FileSize cannot get the size of a file that does not exist");
	}

	return size;
}

luabind::object NSRootDir::Wrapper_FindFiles(lua_State* L, const PathStringArg& SearchString){

	FileSearchResult Results;
		this->FindFiles(SearchString, Results);


	lua_createtable(L, Results.size(), 0);
	luabind::object table = luabind::object(luabind::from_stack(L,-1));

	auto it = Results.begin();

	for(uint32_t i = 0; i < Results.size() ; i++,it++){
		table[i+1] =  it->first;
	}

	return table;
}

luabind::object NSRootDir::Wrapper_FindDirectorys(lua_State *L, const PathStringArg& SearchString){

	FileSearchResult Results;
	int count = this->FindDirectorys(SearchString, Results);

	lua_createtable(L, Results.size(), 0);
	luabind::object table = luabind::object(luabind::from_stack(L,-1));

	auto it = Results.begin();

	for(uint32_t i = 0; i < count ; i++,it++){
		table[i+1] = it->first.c_str();
	}

	return table;
}

luabind::scope NSRootDir::RegisterClass(){

	return luabind::class_<NSRootDir>("RootDirectory")
					.def("Exists", &NSRootDir::Wrapper_FileExists)
					.def("FileSize", &NSRootDir::Wrapper_FileSize)
					.def("DateModifed", &NSRootDir::Wrapper_GetModifedTime)
					.def("FindDirectorys", &NSRootDir::Wrapper_FindDirectorys)
					.def("FindFiles", &NSRootDir::Wrapper_FindFiles)
					.def("LoadLuaFile", &NSRootDir::LoadLuaFile);
}

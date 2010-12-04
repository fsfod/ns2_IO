#include "StdAfx.h"
#include "boost/filesystem.hpp"
#include "NSRootDir.h"
#include "StringUtil.h"


using namespace std;
namespace bfs = boost::filesystem ;

 
void NSRootDir::SetupPathStrings(const PathString& nsroot, const PathString& path){
	size_t PathLength = path.length()+nsroot.size()+1;

	RealPath.reserve(PathLength);
	RealPath = nsroot;
	RealPath.append(path);
	RealPath.push_back('/');

	GameFileSystemPath.push_back('/');
	GameFileSystemPath.append(path);
	GameFileSystemPath.push_back('/');
}

bool NSRootDir::FileExists(const PathString& FilePath){
	//PathString fullpath = RealPath+FilePath;

//	int result = GetFileAttributes(FilePath.c_str());

	return bfs::exists(RealPath+FilePath);//result != INVALID_FILE_ATTRIBUTES;
}

bool NSRootDir::FileSize(const PathString& FilePath, double& Filesize){
	PathString fullpath = RealPath+FilePath;

	if(!bfs::exists(fullpath)){
		return false;
	}else{
		Filesize = bfs::file_size(fullpath);
	 return true;
	}
}

bool NSRootDir::DirectoryExists(const PathString& DirectoryPath){
	PathString fullpath = RealPath+DirectoryPath;

	int result = GetFileAttributes(fullpath.c_str());

	return result != INVALID_FILE_ATTRIBUTES && (result&FILE_ATTRIBUTE_DIRECTORY) != 0;
}

int NSRootDir::FindFiles(const PathString& SearchPath, const PathString& NamePatten, FileSearchResult& FoundFiles){
	
	PathString fullpath = RealPath+SearchPath+NamePatten;

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

	FindClose(FHandle);

	return count;
}


int NSRootDir::FindDirectorys(const PathString& SearchPath, const PathString& NamePatten, FileSearchResult& FoundDirectorys){

	PathString fullpath = RealPath+SearchPath+NamePatten;

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

	FindClose(FHandle);

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
	std::string fullpath("");

	WStringToUTF8STLString(RealPath+FilePath, fullpath);
#else
	std::string fullpath = RealPath+FilePath;
#endif


	//FIXME need to handle unicode paths
	int LoadResult = luaL_loadfile(L, fullpath.c_str());

	//loadfile should of pushed either a function or an error message on to the stack leave it there as the return value
	return;
}

////////////////////////////////////////////////////////////////////
////////////////////        LUA Wrappers        ////////////////////
////////////////////////////////////////////////////////////////////


const PathStringArg&  NSRootDir::get_Path(){
	return reinterpret_cast<const PathStringArg&>(GameFileSystemPath);
}

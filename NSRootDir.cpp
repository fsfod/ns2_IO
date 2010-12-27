#include "StdAfx.h"
#include "NSRootDir.h"
#include "NS_IOModule.h"
#include "StringUtil.h"

extern PlatformPath NSRootPath;

using namespace std;

namespace boostfs = boost::filesystem ;


 
DirectoryFileSource::DirectoryFileSource(char* dirName){

#if defined(UNICODE)
	wstring DirectoryName;
	UTF8ToWString(dirName, DirectoryName);

	RealPath = NSRootPath/DirectoryName;
#else
	SetupPathStrings(nsroot, PathString(path));
#endif // UINCODE

	GameFileSystemPath = dirName;

	DirectoryName.insert(0, 1, '/');
	DirectoryName.push_back('/');
}

DirectoryFileSource::DirectoryFileSource(const PlatformPath& DirectoryPath, const PathString& GamePath)
 :GameFileSystemPath(GamePath), RealPath(DirectoryPath){
}

bool DirectoryFileSource::FileExists(const PathStringArg& FilePath){
	return boostfs::exists(FilePath.CreatePath(RealPath));
}

bool DirectoryFileSource::FileSize(const PathStringArg& path, double& Filesize ){
	auto fullpath = path.CreatePath(RealPath);

	if(!boostfs::exists(fullpath)){
		return false;
	}else{
		Filesize = (double)boostfs::file_size(fullpath);
	 return true;
	}
}

bool DirectoryFileSource::DirectoryExists( const PathStringArg& DirectoryPath ){
	auto fullpath = DirectoryPath.CreatePath(RealPath);

	int result = GetFileAttributes(fullpath.c_str());

	return result != INVALID_FILE_ATTRIBUTES && (result&FILE_ATTRIBUTE_DIRECTORY) != 0;
}

int DirectoryFileSource::FindFiles(const PathStringArg& SearchPath, const PathStringArg& NamePatten, FileSearchResult& FoundFiles){
	
	auto fullpath = SearchPath.CreateSearchPath(RealPath, NamePatten);

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


int DirectoryFileSource::FindDirectorys(const PathStringArg& SearchPath, const PathStringArg& NamePatten, FileSearchResult& FoundDirectorys )
{

	auto fullpath = SearchPath.CreateSearchPath(RealPath, NamePatten);

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


bool DirectoryFileSource::GetModifiedTime( const PathStringArg& Path, int32_t& Time ){

	auto fullpath = Path.CreatePath(RealPath);

#ifdef UNICODE
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

void DirectoryFileSource::LoadLuaFile( lua_State* L, const PathStringArg& FilePath ){

	auto path = RealPath/ConvertAndValidatePath(FilePath);


	LuaModule::LoadLuaFile(L, path, path.string().c_str());

	//loadfile should of pushed either a function or an error message on to the stack leave it there as the return value
	return;
}


////////////////////////////////////////////////////////////////////
////////////////////        LUA Wrappers        ////////////////////
////////////////////////////////////////////////////////////////////


const string& DirectoryFileSource::get_Path(){
	return GameFileSystemPath;
}

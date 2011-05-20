#include "StdAfx.h"
#include "NSRootDir.h"
#include "NS_IOModule.h"
#include "StringUtil.h"
#include <boost/algorithm/string.hpp>

extern PlatformPath NSRootPath;

//using namespace std;

namespace boostfs = boost::filesystem ;


 
DirectoryFileSource::DirectoryFileSource(char* dirName){

	RealPath = NSRootPath/dirName;
	GameFileSystemPath = dirName;

	IsRootSource = true;

	if(GameFileSystemPath.back() != '/'){
		GameFileSystemPath.push_back('/');
	}
}

DirectoryFileSource::DirectoryFileSource(const PlatformPath& DirectoryPath, const PathString& GamePath, bool isRootSource)
 :GameFileSystemPath(GamePath), RealPath(DirectoryPath), IsRootSource(isRootSource){

 if(!GameFileSystemPath.empty() && GameFileSystemPath.back() != '/'){
	 GameFileSystemPath.push_back('/');
 }
}

bool DirectoryFileSource::FileExists(const PathStringArg& FilePath){
	return boostfs::exists(FilePath.CreatePath(RealPath));
}

bool DirectoryFileSource::FileExists(const string& FilePath){
  return boostfs::exists(RealPath/FilePath);
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

	int result = _wstat32(fullpath.c_str(), &FileInfo);
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


const string luaExt(".lua");

void DirectoryFileSource::LoadLuaFile( lua_State* L, const PathStringArg& FilePath ){

	auto path = RealPath/ConvertAndValidatePath(FilePath);

  if(FilePath.GetExtension() != luaExt){
    throw std::runtime_error(FilePath.ToString() + " is not a lua file");
  }

  string chunkpath = path.string();
  //

  boost::replace(chunkpath, '/', '\\');


	LuaModule::LoadLuaFile(L, path, ('@'+chunkpath).c_str() );

	//loadfile should of pushed either a function or an error message on to the stack leave it there as the return value
	return;
}

int DirectoryFileSource::RelativeRequire(lua_State* L, const PathStringArg& ModuleName ){
  
  auto SearchPath = RealPath/ConvertAndValidatePath(ModuleName)/"?.dll";

  auto package = luabind::globals(L)["package"];

  //relative 
  try{
    string OriginalCPath = luabind::object_cast<string>(package["cpath"]);
    string OriginalPath = luabind::object_cast<string>(package["path"]);

    package["cpath"] = SearchPath.string();


    package["cpath"] = OriginalCPath;
    package["path"] = OriginalPath;
  }catch(luabind::cast_failed e){
    
  }

  return 0;
}

////////////////////////////////////////////////////////////////////
////////////////////        LUA Wrappers        ////////////////////
////////////////////////////////////////////////////////////////////


const string& DirectoryFileSource::get_Path(){
	return GameFileSystemPath;
}

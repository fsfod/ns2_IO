#include "StdAfx.h"
#include "NSRootDir.h"
#include "StringUtil.h"
#include <sys/stat.h>

using namespace std;


void NSRootDir::SetupPathStrings(const PathString& nsroot, const PathString& path){
	PathLength = path.length()+nsroot.size()+1;

	PathBuffer.reserve(PathLength);
	PathBuffer = nsroot;
	PathBuffer.append(path);
	PathBuffer.push_back('/');

	Path.push_back('/');
	Path.append(path);
	Path.push_back('/');
}

bool NSRootDir::FileExists(const PathString& path){
	PathBuffer.append(path);

	int result = GetFileAttributes(PathBuffer.c_str());

	ResetPathBuffer();

	return result != INVALID_FILE_ATTRIBUTES;
}

bool NSRootDir::FileSize(const PathString& path, uint32_t& Filesize){
	PathBuffer.append(path);

	WIN32_FILE_ATTRIBUTE_DATA AttributeData;

	int result = GetFileAttributesEx(PathBuffer.c_str(), GetFileExInfoStandard, &AttributeData);

	ResetPathBuffer();

	if(result == 0){
		return false;
	}

	Filesize = AttributeData.nFileSizeLow;

	return true;
}

bool NSRootDir::DirectoryExists(const PathString& path){
	PathBuffer.append(path);

	int result = GetFileAttributes(PathBuffer.c_str());

	ResetPathBuffer();

	return result != INVALID_FILE_ATTRIBUTES && (result&FILE_ATTRIBUTE_DIRECTORY) != 0;
}

int NSRootDir::FindFiles(const PathString SearchPath, FileSearchResult& FoundFiles){
	
	PathBuffer.append(SearchPath);

	WIN32_FIND_DATAW FindData;
	memset(&FindData, 0, sizeof(FindData));

	HANDLE FHandle = FindFirstFileEx(PathBuffer.c_str(), FindExInfoBasic, &FindData, FindExSearchNameMatch, NULL, 0);

	ResetPathBuffer();

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

	PathBuffer.append(SearchPath);

	WIN32_FIND_DATAW FindData;
	memset(&FindData, 0, sizeof(FindData));

	HANDLE FHandle = FindFirstFileEx(PathBuffer.c_str(), FindExInfoBasic, &FindData, (FINDEX_SEARCH_OPS)(FindExSearchNameMatch|FindExSearchLimitToDirectories), NULL, 0);

	ResetPathBuffer();

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


bool NSRootDir::GetModifedTime(const PathString& Path, uint32_t& Time){

	PathBuffer.append(Path);

#ifdef UnicodePathString
	struct _stat32 FileInfo;

	int result = _wstat(PathBuffer.c_str(), &FileInfo);
#else
	struct stat FileInfo;

	int result = stat(PathBuffer.c_str(), &FileInfo);
#endif

	ResetPathBuffer();

	if(result == 0){
		return false;
	}

	Time = FileInfo.st_mtime;

	return true;
}

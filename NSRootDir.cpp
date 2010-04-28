#include "StdAfx.h"
#include "NSRootDir.h"
#include "StringUtil.h"

using namespace std;


NSRootDir::NSRootDir(PathString nsroot, PathString path){
	PathLength = path.length()+nsroot.size()+1;

	PathBuffer.reserve(PathLength);
	PathBuffer = nsroot;
	PathBuffer.append(path);
	PathBuffer.push_back('/');


	Path.push_back('/');
	Path.append(path);
	Path.push_back('/');
}

NSRootDir::NSRootDir(){
	PathLength = 0;
}


bool NSRootDir::FileExists(PathString& path){
	PathBuffer.append(path);

	int result = GetFileAttributes(PathBuffer.c_str());

	ResetPathBuffer();

	return result != INVALID_FILE_ATTRIBUTES;
}

bool NSRootDir::FileSize(PathString& path, uint32_t& Filesize){
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

bool NSRootDir::DirectoryExists(PathString& path){
	PathBuffer.append(path);

	int result = GetFileAttributes(PathBuffer.c_str());

	ResetPathBuffer();

	return result != INVALID_FILE_ATTRIBUTES && (result&FILE_ATTRIBUTE_DIRECTORY) != 0;
}

int NSRootDir::FindFiles(PathString SearchPath, FileSearchResult& FoundFiles){
	
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


int NSRootDir::FindDirectorys(PathString SearchPath, FileSearchResult& FoundDirectorys){

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


bool NSRootDir::GetModifedTime(const PathString& Path, uint64_t& Time){

	PathBuffer.append(Path);

	WIN32_FILE_ATTRIBUTE_DATA AttributeData;

	int result = GetFileAttributesEx(PathBuffer.c_str(), GetFileExInfoStandard, &AttributeData);

	ResetPathBuffer();

	if(result == 0){
		return false;
	}


	return true;
}

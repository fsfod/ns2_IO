#include "StdAfx.h"
#include "NSRootDir.h"
#include "StringUtil.h"

using namespace std;

NSRootDir::NSRootDir(wstring nsroot, const wchar_t* path){
	PathLength = wcslen(path)+nsroot.size()+1;

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


bool NSRootDir::FileExists(wstring& path){
	PathBuffer.append(path);

	int result = GetFileAttributes(PathBuffer.c_str());

	ResetPathBuffer();

	return result != INVALID_FILE_ATTRIBUTES;
}

int NSRootDir::FindFiles(wstring SearchPath, FileSearchResult& FoundFiles){
	
	PathBuffer.append(SearchPath);

	WIN32_FIND_DATAW FindData;
	memset(&FindData, 0, sizeof(FindData));

	HANDLE FHandle = FindFirstFileExW(PathBuffer.c_str(), FindExInfoBasic, &FindData, FindExSearchNameMatch, NULL, 0);

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
	}while(FindNextFileW(FHandle, &FindData));

	return count;
}


int NSRootDir::FindDirectorys(wstring SearchPath, FileSearchResult& FoundDirectorys){

	WIN32_FIND_DATAW FindData;
	memset(&FindData, 0, sizeof(FindData));

	PathBuffer.append(SearchPath);

	HANDLE FHandle = FindFirstFileExW(PathBuffer.c_str(), FindExInfoBasic, &FindData, (FINDEX_SEARCH_OPS)(FindExSearchNameMatch|FindExSearchLimitToDirectories), NULL, 0);

	ResetPathBuffer();

	if(FHandle == INVALID_HANDLE_VALUE){
		return 0;
	}

	do{
		if(wcscmp(FindData.cFileName, L".") && wcscmp(FindData.cFileName, L"..") && (FindData.dwFileAttributes&FILE_ATTRIBUTE_DIRECTORY) != 0){
			string FileName;
			UTF16ToUTF8STLString(FindData.cFileName, FileName);

			FoundDirectorys[FileName] = this;
		}
	}while(FindNextFileW(FHandle, &FindData));

	return 0;
}

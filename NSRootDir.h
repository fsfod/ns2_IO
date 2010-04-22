#include "stdafx.h"
#include <unordered_map>

#pragma once

class NSRootDir;

typedef std::tr1::unordered_map<std::string,NSRootDir*> FileSearchResult;

class NSRootDir{

public:
	NSRootDir();
	NSRootDir(std::wstring nsroot, const wchar_t* path);
	~NSRootDir(){}

	bool FileExists(std::wstring& path);
	int FindFiles(std::wstring SearchPath, FileSearchResult& FoundFiles);
	int FindDirectorys(std::wstring SearchPath, FileSearchResult& FoundDirectorys);

	std::wstring& GetPath(){
		return Path;
	}

private:
	void ResetPathBuffer(){
		PathBuffer.resize(PathLength);
	}

	void CheckSpace(int newspareCapcity);
	
	std::wstring Path, PathBuffer;
	int PathLength;
};

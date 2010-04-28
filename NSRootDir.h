#include "stdafx.h"
#include <unordered_map>
#include "LuaErrorWrapper.h"
#include "StringUtil.h"

#pragma once

class NSRootDir;

typedef std::tr1::unordered_map<std::string,NSRootDir*> FileSearchResult;


class NSRootDir{

public:
	NSRootDir();
	NSRootDir(PathString nsroot, char* path){
#if defined(UNICODE)
			std::wstring wpath;
			UTF8ToWString(path, wpath);

			NSRootDir(nsroot, wpath);
#else
		NSRootDir(nsroot, PathString(path));
#endif // UINCODE
	}

	NSRootDir(PathString nsroot, PathString path);
	~NSRootDir(){}

	bool FileExists(PathString& path);
	bool DirectoryExists(PathString& path);
	int FindFiles(PathString SearchPath, FileSearchResult& FoundFiles);
	int FindDirectorys(PathString SearchPath, FileSearchResult& FoundDirectorys);
	bool FileSize(PathString& path, uint32_t& Filesize);
	bool GetModifedTime(const PathString& Path, uint64_t& Time);

	PathString& GetPath(){
		return Path;
	}

private:
	void ResetPathBuffer(){
		PathBuffer.resize(PathLength);
	}

	void CheckSpace(int newspareCapcity);
	
	PathString Path, PathBuffer;
	int32_t PathLength;
};

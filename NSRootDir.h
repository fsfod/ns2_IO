#include "stdafx.h"
#include <unordered_map>
#include "LuaErrorWrapper.h"
#include "StringUtil.h"

#pragma once

class NSRootDir;

typedef std::tr1::unordered_map<std::string,NSRootDir*> FileSearchResult;


class NSRootDir{

public:
		NSRootDir(){PathLength = 0;}
		NSRootDir(PathString nsroot, char* path){
#if defined(UNICODE)
		std::wstring wpath;
		UTF8ToWString(path, wpath);

		SetupPathStrings(nsroot, wpath);
#else
		SetupPathStrings(nsroot, PathString(path));
#endif // UINCODE
	}

	NSRootDir(PathString nsroot, PathString path){SetupPathStrings(nsroot, path);}
	~NSRootDir(){}


	bool FileExists(const PathString& path);
	bool DirectoryExists(const PathString& path);
	int FindFiles(const PathString SearchPath, FileSearchResult& FoundFiles);
	int FindDirectorys(const PathString SearchPath, FileSearchResult& FoundDirectorys);
	bool FileSize(const PathString& path, uint32_t& Filesize);
	bool GetModifedTime(const PathString& Path, uint32_t& Time);

	PathString& GetPath(){
		return Path;
	}

private:
	void ResetPathBuffer(){
		PathBuffer.resize(PathLength);
	}

	void CheckSpace(int newspareCapcity);
	void SetupPathStrings(const PathString& nsroot, const PathString& path);

	PathString Path, PathBuffer;
	int32_t PathLength;
};

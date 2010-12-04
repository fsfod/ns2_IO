#include "StdAfx.h"
#include "Archive.h"
#include "C7ZipLibrary.h"

#include "7zip/CPP/7zip/Archive/IArchive.h"
#include "7zip/CPP/Windows/PropVariant.h"
#include <boost/algorithm/string.hpp>  

using namespace std;

Archive::Archive(C7ZipLibrary* owner, PathString archivepath, IInArchive* reader){

	Reader = reader;

	uint32_t itemcount;
	Reader->GetNumberOfItems(&itemcount);
	
	vector<PathString> dirlist;

	for(uint32_t i = 0; i < itemcount ; i++){
		NWindows::NCOM::CPropVariant prop;

		if (Reader->GetProperty(i, kpidPath, &prop) != S_OK || prop.vt != VT_BSTR)continue;
		PathString itempath = prop.bstrVal;

		if (Reader->GetProperty(i, kpidIsDir, &prop) != S_OK)continue;

		bool isDir = prop.vt == VT_BOOL ? prop.boolVal : false;

		if(!isDir){
			Files[itempath] = FileEntry(i);
		}else{
			dirlist.push_back(itempath);
		}
	}

	BOOST_FOREACH(PathString& dirpath, dirlist){
		int index = dirpath.find('//');

	}

}


Archive::~Archive(void){

}

bool Archive::FileExists(const PathString& path){
	return Files.find(path) != Files.end();
}

bool Archive::DirectoryExists(const PathString& path){
	return true;
}

int Archive::FindFiles(const PathString& SearchPath, const PathString& NamePatten, FileSearchResult& FoundFiles)
{
	return 0;
}

int Archive::FindDirectorys(const PathString& SearchPath, const PathString& NamePatten, FileSearchResult& FoundDirectorys){
	return 0;
}

bool Archive::FileSize(const PathString& path, double& Filesize){
	NWindows::NCOM::CPropVariant prop;
	
	auto file = Files.find(path);

	if(file == Files.end())return false;

	if(Reader->GetProperty(file->second.FileIndex, kpidSize, &prop) != S_OK)throw std::exception("Unknown error while trying to get size of compressed file");

	switch (prop.vt){
		case VT_UI1: Filesize = prop.bVal;
		case VT_UI2: Filesize = prop.uiVal;
		case VT_UI4: Filesize = prop.ulVal;
		case VT_UI8: Filesize = prop.uhVal.QuadPart;

		default:
			throw std::exception("Unknown error while trying to get size of compressed file");
	}

}

bool Archive::GetModifiedTime(const PathString& Path, int32_t& Time){
	return true;
}

void Archive::LoadLuaFile(lua_State* L, const PathStringArg& FilePath){
	return;
}
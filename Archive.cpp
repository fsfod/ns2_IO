#include "StdAfx.h"
#include "Archive.h"
#include "SevenZip.h"

#include "7zip/CPP/7zip/Archive/IArchive.h"
#include "7zip/CPP/Windows/PropVariant.h"
#include "7zip/CPP/7zip/ICoder.h"
#include "7zip/CPP/7zip/IPassword.h"

#include "7zip/CPP/Common/MyCom.h"
#include "7zHelpers.h"

#include <boost/algorithm/string.hpp>
//#include "ResourceOverrider.h"
#include "SourceManager.h"

//using namespace std;
using namespace boost::iostreams;

Archive::Archive(PathString archivepath, IInArchive* reader) : MountedFileCount(0), LuaPointer(){

	Reader = reader;
	ArchivePath = archivepath;
	ArchiveName = ArchivePath.filename().string();

	uint32_t itemcount;
	Reader->GetNumberOfItems(&itemcount);
	
	for(uint32_t i = 0; i < itemcount ; i++){
		NWindows::NCOM::CPropVariant prop;
		
		if(Reader->GetProperty(i, kpidIsDir, &prop) != S_OK || prop.vt != VT_BOOL || prop.boolVal)continue;

		if(Reader->GetProperty(i, kpidPath, &prop) != S_OK || prop.vt != VT_BSTR)continue;
		string& itempath = NormalizedPath(prop.bstrVal);


		PathToFile[itempath] = FileEntry(i);
	}


	AddFilesToDirectorys();
}

Archive::~Archive(){
  Reader->Close();
}

boost::shared_ptr<FileSource> Archive::GetLuaPointer(){

  //recreate our pointer if the previous lua shared_ptr was GC'ed
  if(LuaPointer.use_count() == 0){
    boost::shared_ptr<FileSource> ptr(this, [](Archive* arc){ arc->CheckDelete(); });
    LuaPointer = ptr;

    return ptr;
  }else{
   return boost::shared_ptr<FileSource>(LuaPointer);
  }
}


Archive::SelfType* Archive::CreateDirectorysForPath(const std::string& path, vector<int>& SlashIndexs){

	SelfType* Directory;

	if(SlashIndexs.size() > 1){
		int CreatedCount;

		Directory = BuildDirectoryObjectsForPath(path, SlashIndexs, CreatedCount);

		//if more than 1 directory was created or 
		if(SlashIndexs.back() > 0 || CreatedCount != 1){
			auto CurrentDir = Directory->Parent;

			for(int i = SlashIndexs.size()-2; i != 0 ; --i){
				
				if(SlashIndexs[i] < 0){
					PathToDirectory[path.substr(0, -SlashIndexs[i])] = CurrentDir;
				}
				CurrentDir = CurrentDir->Parent;
			}
		}
	}else{
		Directory = CreateDirectory(path);
	}

	//store the directory we created
	PathToDirectory[path] = Directory;

	return Directory;
}

typedef std::hash_map<PathString, FileEntry>::_Paircc FileNode;


void Archive::AddFilesToDirectorys(){
	
	vector<int> SlashIndexs = vector<int>();
	
	BOOST_FOREACH(FileNode& node, PathToFile){
		const string& path = node.first;

		int index = path.find_last_of('/');

		if(index != PathString::npos){
			string ParentPath = path.substr(0, index);

			SelfType* Parent = PathToDirectory[ParentPath];

			if(Parent == NULL){
				int index = 0;

				while((index = path.find('/', index+1)) != PathString::npos){
					SlashIndexs.push_back(index);
				}

				if(SlashIndexs.size() > 1){
					Parent = CreateDirectorysForPath(ParentPath, SlashIndexs);
				}else{
					Parent = GetCreateDirectory(ParentPath);
					PathToDirectory[ParentPath] = Parent;
				}

				SlashIndexs.clear();
			}
			
			Parent->Files[path.substr(index+1)] = FileEntry(node.second);
		}
	}
}

bool Archive::FileExists(const PathStringArg& path){
	return PathToFile.find(path.GetNormalizedPath()) != PathToFile.end();
}

bool Archive::DirectoryExists(const PathStringArg& path){
	return PathToDirectory.find(path.GetNormalizedPath()) != PathToDirectory.end();
}

int Archive::FindFiles(const PathStringArg& SearchPath, const PathStringArg& NamePatten, FileSearchResult& FoundFiles){

  string search = SearchPath.GetNormalizedPath();

  const SourceDirectory* dir;

  if(search.empty()){
    dir = this;
  }else{
    auto result = PathToDirectory.find(search);

    if(result == PathToDirectory.end())return 0;

    dir = (*result).second;
  }

  int count = 0;

	string patten = NamePatten.ToString();

  BOOST_FOREACH(const FileNode& file, dir->Files){
    //FIXME replace PathMatchSpec with something better
		if(patten.empty() || PathMatchSpecExA(file.first.c_str(), patten.c_str(), PMSF_NORMAL) == S_OK){
			FoundFiles[file.first] = static_cast<FileSource*>(this);
			count++;
		}
	}

  return count;
}

int Archive::FindDirectorys(const PathStringArg& SearchPath, const PathStringArg& NamePatten, FileSearchResult& FoundDirectorys){

  string search = SearchPath.GetNormalizedPath();

  const SourceDirectory* dir;
  
  if(search.empty()){
    dir = this;
  }else{
   auto result = PathToDirectory.find(search);

   if(result == PathToDirectory.end())return 0;

   dir = (*result).second;
  }

  int count = 0;
  
  string patten = NamePatten.ToString();
  
  BOOST_FOREACH(const DirNode& entry, dir->Directorys){
  	if(patten.empty() || PathMatchSpecExA(entry.first.c_str(), patten.c_str(), PMSF_NORMAL) == S_OK){
  		FoundDirectorys[entry.first] = static_cast<FileSource*>(this);
  		count++;
  	}
  }
  return count;
}

bool Archive::FileSize(const PathStringArg& path, double& Filesize){

	auto file = PathToFile.find(path.GetNormalizedPath());

	if(file == PathToFile.end()){
		return false;
	}

	Filesize = GetFileSize(file->second.FileIndex);

	return true;
}

uint32_t Archive::GetFileSize(int FileIndex){
	
	NWindows::NCOM::CPropVariant prop;

	if(Reader->GetProperty(FileIndex, kpidSize, &prop) != S_OK){
		throw std::exception("Unknown error while trying to get size of compressed file");
	}

	int64_t Filesize;

	switch (prop.vt){
		case VT_UI1: 
			Filesize = prop.bVal;
		break;

		case VT_UI2: 
			Filesize = prop.uiVal;
		break;

		case VT_UI4: 
			Filesize = prop.ulVal;
		break;

		case VT_UI8: 
			Filesize = prop.uhVal.QuadPart;
		break;

		default:
			throw std::exception("Unexpected value type while getting size of compressed file");
	}
	

	_ASSERT(Filesize < INT32_MAX);

	return (uint32_t)Filesize;
}

int64_t Archive::GetModifiedTime(int FileIndex){
  NWindows::NCOM::CPropVariant prop;

  if(Reader->GetProperty(FileIndex, kpidMTime, &prop) != S_OK){
    throw std::exception("Archive Does not support Modified time");
  }

  if(prop.vt == VT_FILETIME){
    return prop.filetime.dwLowDateTime | ((UInt64)prop.filetime.dwHighDateTime << 32);
  }else{
    throw std::exception("Modified time of archive format was return in an unexpected format");
  }
}

bool Archive::GetModifiedTime(const PathStringArg& Path, int32_t& Time){
  
  int FileIndex = GetFileIndex(Path.GetNormalizedPath());

  if(FileIndex == -1)return false;

  int64_t ftime = GetModifiedTime(FileIndex);

  Time = (int32_t)((ftime-116444736000000000)/10000000);

	return true;
}

void Archive::ExtractFile(uint32_t FileIndex, const PlatformPath& FilePath){

  if(boostfs::exists(FilePath)){
    throw exception("Cannot extract over an existing file");
  }

  int size = GetFileSize(FileIndex);

  MemMappedOutSteam stream(FilePath, size);
  SingleExtractCallback extrackCb(FileIndex, &stream);

  Reader->Extract(&FileIndex, 1, false, &extrackCb);
  extrackCb.CheckResult();
}

void* Archive::ExtractFileToMemory(uint32_t FileIndex){

  int size = GetFileSize(FileIndex);

  SimpleOutSteam stream(size);
  SingleExtractCallback extrackCb(FileIndex, &stream);

  Reader->Extract(&FileIndex, 1, false, &extrackCb);
  extrackCb.CheckResult();

  return stream.TransferBufferOwnership();
}

int Archive::LoadLuaFile(lua_State* L, const PathStringArg& FilePath){

	auto file = PathToFile.find(FilePath.GetNormalizedPath());
	
	if(file == PathToFile.end()){
	  throw exception("cannot load a lua file that does not exist");
	}

	uint32_t index = file->second.FileIndex;

	int size = GetFileSize(file->second.FileIndex);
	 
  SimpleOutSteam stream(size);
  SingleExtractCallback extract(index, &stream);

	Reader->Extract(&index, 1, false, &extract);

  return stream.LoadChunk(L, (ArchiveName+FilePath.ToString()).c_str());
}

uint32_t Archive::GetFileCRC( int FileIndex ){
  NWindows::NCOM::CPropVariant CRCResult;

  if(Reader->GetProperty(FileIndex, kpidCRC, &CRCResult) != S_OK)throw exception("Failed to get crc of file");
  
  if(CRCResult.vt != VT_UI4)throw exception("File crc was not expected value type");

  return CRCResult.ulVal;
}

extern ResourceOverrider* OverrideSource;

void Archive::MountFile(const PathStringArg& FilePath, const PathStringArg& DestinationPath){

  int index = GetFileIndex(FilePath.GetNormalizedPath());

  if(index == -1)throw exception("Could not find the file specified in the archive");

 //boost::shared_ptr<void>(ExtractFileToMemory(index), [](void* data){delete data;}

  SourceManager::MountFile(DestinationPath.GetNormalizedPath(), MountedFile(this, index));
}

void Archive::MountFiles(const PathStringArg& BasePath, const PathStringArg& DestinationPath){
  
  vector<pair<string, int>> fileList;

  CreateMountList(BasePath.GetNormalizedPath(), DestinationPath.GetNormalizedPath(), fileList);

  SourceManager::MountFilesList(this, fileList, true);
}

void Archive::CreateMountList(const std::string& BasePath, const std::string& path, vector<pair<string, int>>& fileList){

  int pathStart = BasePath.size();

  if(pathStart != 0 && BasePath.back() != '/' && BasePath.back() != '\\')pathStart++;

  BOOST_FOREACH(const FileNode& file, PathToFile){
    const string& destpath = pathStart != 0 ? file.first.substr(pathStart)  : file.first;

    if(BasePath.empty() || file.first.compare(0, BasePath.size(),BasePath.c_str()) == 0){
      fileList.push_back(pair<string, int>(path.empty() ? destpath  : path+destpath, file.second.FileIndex));
    }
  }

}

M4::File* Archive::GetEngineFile(M4::Allocator* alc, const string& path){
  int FileIndex = GetFileIndex(path);

  if(FileIndex == -1)return NULL;

  return new (alc->AllocateAligned(sizeof(ArchiveFile), 4)) ArchiveFile(this, FileIndex);
}

M4::File* Archive::GetEngineFile(M4::Allocator* alc, int FileIndex){

  if(FileIndex < 0)throw std::exception("GetEngineFile: Invalid File index");

  return new (alc->AllocateAligned(sizeof(ArchiveFile), 4)) ArchiveFile(this, FileIndex);
}

std::int64_t Archive::GetFileModificationTime( const string& path ){
  int index = GetFileIndex(path);

  if(index == -1)return 0;

  return GetModifiedTime(index);
}

void Archive::CheckDelete(){

  if(MountedFileCount == 0 && LuaPointer.use_count() == 0){
    SourceManager::ArchiveClosed(this);
   delete this;
  }
}

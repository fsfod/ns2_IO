#include "StdAfx.h"
#include "ExtractCache.h"
#include <boost/system/system_error.hpp>
#include <boost/archive/xml_oarchive.hpp>
#include <boost/archive/xml_iarchive.hpp>

//using namespace std;

CacheEntry::CacheEntry(const PlatformPath& filepath, uint32_t CRC): FileCRC(CRC), ContainingArchive(NULL){
  FileInfo fileInfo;
  ExtractCache::GetFileInfo(filepath, fileInfo);

  FileSize = fileInfo.GetFileSize();
  ModifedTime = fileInfo.GetModifedTime();
}

static const char* ExtensionList[] = {".png", ".dds", ".model", ".material", ".cinematic"};

ExtractCache::ExtractCache(){
  ValidExtensions = std::map<string, string>();

  BOOST_FOREACH(const char* ext, ExtensionList){
    ValidExtensions[ext] = "";
  }

  ValidExtensions[".level"] = "maps";
  ValidExtensions[".hmp"] = "maps/overviews";
  ValidExtensions[".tga"] = "maps/overviews";
}


ExtractCache::~ExtractCache(){
  Save();
}

void ExtractCache::Save(){
  using namespace boost::archive;

  auto CacheRecordsPath = CacheDirectory/"CacheRecords.xml";

  std::ofstream CacheRecordSteam(CacheRecordsPath.c_str(), std::ofstream::trunc);

  xml_oarchive CacheRecords(CacheRecordSteam, no_header|track_never);

  CacheRecords << make_nvp("ExtractFileCache", *this);
}

void ExtractCache::Load( const PlatformPath& cacheDirectory,const PlatformPath& contentDirectory ){
  using namespace boost::archive;
  CacheDirectory = cacheDirectory;
  ContentDirectory = contentDirectory;

  if(!boostfs::exists(CacheDirectory)){
    boostfs::create_directory(CacheDirectory);
   return;
  }

  auto CacheRecordsPath = cacheDirectory/"CacheRecords.xml";

  if(!boostfs::exists(CacheRecordsPath))return;

  std::ifstream CacheRecordSteam(CacheRecordsPath.c_str());

  xml_iarchive CacheRecords(CacheRecordSteam, no_header|track_never);

  CacheRecords >> make_nvp("ExtractFileCache", *this);
}

void ExtractCache::GetFileInfo( const PlatformPath& Path, FileInfo& FileInfo )
{

 HANDLE result = FindFirstFileEx(Path.c_str(), FindExInfoStandard, &FileInfo, FindExSearchNameMatch, NULL, 0);

 if(result == INVALID_HANDLE_VALUE){
  throw boost::system::system_error(GetLastError(), boost::system::system_category(), "Error while getting file information");
 }

 FindClose(result);
}

bool ExtractCache::CacheEntryStillValid(const std::pair<const string, CacheEntry>& entry){

  PlatformPath entryPath = ContentDirectory/entry.first;

  if(!boostfs::exists(entryPath))return false;

  FileInfo fileInfo;
  GetFileInfo(entryPath, fileInfo);

  return entry.second == fileInfo;
}

void ExtractCache::RemoveExtractedResource(const PathStringArg& path){

  string NormPath = path.GetNormalizedPath();

  auto CacheInfo = FileList.find(NormPath);

  if(CacheInfo == FileList.end())throw exception("That file is not an extracted resource");

  if(!CacheEntryStillValid(*CacheInfo))throw exception("The current file at that path does not match the cache record for that path");

  boostfs::remove(NormPath);
}


void ExtractCache::ExtractResourceToPath(Archive* arch, std::string ArchiveFilePath, const PathStringArg& destinationPath){

  //boostfs::
  int index = arch->GetFileIndex(ArchiveFilePath);
  
  if(index == -1)throw exception("The file does not exist in the archive");

  PlatformPath output = destinationPath.CreatePath(ContentDirectory);
  string NormalizedDest = destinationPath.GetNormalizedPath();

  uint32_t CRC = arch->GetFileCRC(index);

  if(boostfs::exists(output)){
    auto CacheInfo = FileList.find(NormalizedDest);

    if(CacheInfo == FileList.end())throw exception("A file already exists with that name and there is no cache record for it");
     
     const CacheEntry& entry = CacheInfo->second;

     if(arch->GetFileSize(index) == entry.FileSize && CRC == entry.FileCRC){
      if(!CacheEntryStillValid(*CacheInfo))throw exception("the file at the destination path does not match the cache record for that path");

        //the file is already extracted
      return;
     }else{
       throw exception("the already extracted file at that path does not have the same crc as the file from the archive");
     }

  }else{
    auto extpath = ValidExtensions.find(destinationPath.GetExtension());

    if(extpath == ValidExtensions.end())throw exception("Destination has a non supported file extension");

    arch->ExtractFile(index, output);
    
    FileList[NormalizedDest] = CacheEntry(output, CRC);
  }

}
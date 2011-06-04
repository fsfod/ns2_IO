#pragma once

#include "stdafx.h"
#include "Archive.h"
#include "FileInfo.h"
#include <boost/serialization/map.hpp>

using namespace boost::serialization;


//#define make_nvp2(VarName) make_nvp(#VarName ,VarName)

class CacheArchiveRecord{

public:

  std::string Name;
  int FileCrc;
  int ModifedTime;
  int Filesize;
};

class CacheEntry{

friend class boost::serialization::access;

public:
  // When the class Archive corresponds to an output archive, the
  // & operator is defined similar to <<.  Likewise, when the class Archive
  CacheEntry(){}
  CacheEntry(const PlatformPath& filepath, uint32_t CRC);

  // is a type of input archive the & operator is defined similar to >>.
  template<class Archive>void serialize(Archive & ar, const unsigned int version){
    ar&BOOST_SERIALIZATION_NVP(PathInArchive);
    ar&BOOST_SERIALIZATION_NVP(FileCRC);
    ar&BOOST_SERIALIZATION_NVP(ModifedTime);
    ar&BOOST_SERIALIZATION_NVP(FileSize);
  }

  time_t GetModifedTimeFromFileInfo(const PlatformFileStat& fileinfo) const{
    
    return ((*(uint64_t*)&fileinfo.ftLastWriteTime)-116444736000000000)/10000000;
  }

  bool operator ==(const FileInfo& filestat) const {
    return FileSize == filestat.GetFileSize() && (ModifedTime == 0 || ModifedTime == filestat.GetModifedTime());
  }

  bool operator !=(const FileInfo& filestat) const {
    return !(*this == filestat);
  }

  CacheArchiveRecord* ContainingArchive; 

  std::string PathInArchive;
  uint32_t FileCRC;
  time_t ModifedTime;
  uint64_t FileSize;
};

class ExtractCache{
public:
  friend class boost::serialization::access;
  ExtractCache();
  ~ExtractCache();

  void Load(const PlatformPath& cacheDirectory,const PlatformPath& ContentDirectory);
  void Save();

  void ExtractResourceToPath(Archive* arch, std::string ArchiveFilePath, const PathStringArg& destinationPath);
  bool CacheEntryStillValid(const std::pair<const std::string, CacheEntry>& entry);
  static void GetFileInfo(const PlatformPath& Path, FileInfo& FileInfo);

  template<class Archive>void serialize(Archive & ar, const unsigned int version){
    using namespace boost::serialization;
    ar&BOOST_SERIALIZATION_NVP(FileList);
  }

  void RemoveExtractedResource(const PathStringArg& path);
  

private:
  PlatformPath CacheDirectory, ContentDirectory;
  std::map<std::string, std::string> ValidExtensions;
  std::map<std::string, CacheEntry> FileList;
};


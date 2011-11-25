#pragma once

#include "stdafx.h"
#include "StringUtil.h"

typedef WIN32_FIND_DATAW PlatformFileStat;


class FileInfo : public PlatformFileStat{

public:
  FileInfo(){}
  ~FileInfo(){}

  string GetName() const{
    return UTF16ToUTF8STLString(cFileName);
  }

  string GetNormlizedName() const{
    string path = UTF16ToUTF8STLString(reinterpret_cast<const wchar_t*>(&cFileName));
    boost::to_lower(path);

    return path;
  }

  string GetNormlizedNameWithSlash() const{
    int nameLength = wcslen(cFileName);
    string path;
    path.reserve(nameLength+2);

    UTF16ToUTF8STLString(reinterpret_cast<const wchar_t*>(&cFileName), nameLength, path);
    boost::to_lower(path);
    path.push_back('/');

    return path;
  }

  uint64_t GetFileSize() const{
    return nFileSizeLow | ((uint64_t)nFileSizeHigh << 32);
  }

  bool IsDirectory() const{
    return (dwFileAttributes&FILE_ATTRIBUTE_DIRECTORY) != 0;
  }

  uint32_t GetModifedTime() const{
    uint64_t ftime = ftLastWriteTime.dwLowDateTime | ((uint64_t)ftLastWriteTime.dwHighDateTime << 32);

    return (int32_t)((ftime-116444736000000000)/10000000);
  }

private:
  
  
};
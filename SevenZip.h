#pragma once
#include "stdafx.h"

class FileSource;
class C7ZipFormatInfo;

namespace NWindows{
	namespace NCOM{
		class CPropVariant;
	}
}

struct IInArchive;
struct IOutArchive;

class CArcInfoEx{

public:
  CArcInfoEx() :FormatIndex(-1), UpdateEnabled(false){
  }

  CArcInfoEx(int formatIndex);

  IInArchive* GetInArchive() const;
  IOutArchive* GetOutArchive() const;

public:
  string Name;

  uint32_t FormatIndex;
  GUID ClassID;
  bool UpdateEnabled;

  vector<string> extensions;
  string addExt;
};



class SevenZip{

public:
	static SevenZip* Init(PlatformPath& LibaryPath);

	SevenZip();
	~SevenZip();

	static HRESULT ReadProp(uint32_t index, uint32_t propID, NWindows::NCOM::CPropVariant &prop);
	static HRESULT ReadStringProp(uint32_t index, uint32_t propID, std::wstring &res);
  static HRESULT ReadUTF8StringProp(uint32_t index, uint32_t propID, std::string &res);
	static HRESULT ReadBoolProp(uint32_t index, uint32_t propID, bool &res);

  Archive* OpenArchive(const PlatformPath& ArchivePath);
  std::vector<std::string> GetSupportedFormats();

private:
	std::map<std::wstring, CArcInfoEx> ArchiveFormats;

	bool LoadFormats();
};

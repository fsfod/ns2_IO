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
  CArcInfoEx() :FormatIndex(-1){
  }

  CArcInfoEx(int formatIndex);

  IInArchive* GetInArchive() const;
  IOutArchive* GetOutArchive() const;

  bool SupportsUpdatingArchives();
  void GetExtensionList(vector<wstring>& extensions) const;

public:
  string Name;
  uint32_t FormatIndex;
  GUID ClassID;

  vector<string> extensions;
};

class Archive;

class SevenZip{

public:
	static void Init(PlatformPath& LibaryPath, bool enableAllFormats = false );

  static bool IsLoaded();

	static HRESULT ReadProp(uint32_t index, uint32_t propID, NWindows::NCOM::CPropVariant &prop);
	static HRESULT ReadStringProp(uint32_t index, uint32_t propID, std::wstring &res);
  static HRESULT ReadUTF8StringProp(uint32_t index, uint32_t propID, std::string &res);
	static HRESULT ReadBoolProp(uint32_t index, uint32_t propID, bool &res);

  static Archive* OpenArchive(const PlatformPath& ArchivePath);
  static std::vector<std::string> GetSupportedFormats();

private:
	static bool LoadFormats();
  static bool FulllyLoaded;
  static bool AllFormatsEnabled;
};

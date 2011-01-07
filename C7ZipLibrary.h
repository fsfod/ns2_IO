#pragma once
#include "stdafx.h"


typedef uint32_t (__stdcall *GetMethodPropertyFunc)(uint32_t index, uint32_t propID, void *value);
typedef uint32_t (__stdcall *GetNumberOfMethodsFunc)(uint32_t *numMethods);
typedef uint32_t (__stdcall *GetNumberOfFormatsFunc)(uint32_t *numFormats);
typedef uint32_t (__stdcall *GetHandlerPropertyFunc)(uint32_t propID, void *value);
typedef uint32_t (__stdcall *GetHandlerPropertyFunc2)(uint32_t index, uint32_t propID, void *value);
typedef uint32_t (__stdcall *CreateObjectFunc)(const void *clsID, const void *iid, void **outObject);
typedef uint32_t (__stdcall *SetLargePageModeFunc)();


class FileSource;
class C7ZipFormatInfo;

namespace NWindows{
	namespace NCOM{
		class CPropVariant;
	}
}

//adapted from http://code.google.com/p/lib7zip/
class C7ZipLibrary{

public:
	static C7ZipLibrary* Init(PlatformPath& LibaryPath);

	C7ZipLibrary(void* libaryHandle);
	~C7ZipLibrary();
	
	long ReadProp(uint32_t index, uint32_t propID, NWindows::NCOM::CPropVariant &prop);
	long ReadStringProp(uint32_t index, uint32_t propID, std::wstring &res);
	long ReadBoolProp(uint32_t index, uint32_t propID, bool &res);

	FileSource* OpenArchive(const PlatformPath& ArchivePath);

	GetMethodPropertyFunc GetMethodProperty;
	GetNumberOfMethodsFunc GetNumberOfMethods;
	GetNumberOfFormatsFunc GetNumberOfFormats;
	GetHandlerPropertyFunc GetHandlerProperty;
	GetHandlerPropertyFunc2 GetHandlerProperty2;
	CreateObjectFunc CreateObject;
	SetLargePageModeFunc SetLargePageMode;

private:
	void* LibaryHandle;
	std::map<std::wstring,C7ZipFormatInfo> ArchiveFormats;

	//const C7ZipObjectPtrArray & GetInternalObjectsArray() { return m_InternalObjectsArray; }
	bool LoadFormats();

};

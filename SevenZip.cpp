#include "stdafx.h"
#include "Archive.h"
#include "SevenZip.h"
#include "StringUtil.h"

#include "7zip/CPP/Windows/PropVariant.h"
#include "7zip/CPP/Common/MyCom.h"
#include "7zip/CPP/7zip/Archive/IArchive.h"
#include "7zip/CPP/7zip/Common/FileStreams.h"


#include <unordered_set>
#include <boost/algorithm/string/split.hpp>

using NWindows::NCOM::CPropVariant;

typedef uint32_t (__stdcall *GetMethodPropertyFunc)(uint32_t index, uint32_t propID, void *value);
typedef uint32_t (__stdcall *GetNumberOfMethodsFunc)(uint32_t *numMethods);
typedef uint32_t (__stdcall *GetNumberOfFormatsFunc)(uint32_t *numFormats);
typedef uint32_t (__stdcall *GetHandlerPropertyFunc)(uint32_t propID, void *value);
typedef uint32_t (__stdcall *GetHandlerPropertyFunc2)(uint32_t index, uint32_t propID, void *value);
typedef uint32_t (__stdcall *CreateObjectFunc)(const GUID& clsID, const GUID& iid, void **outObject);
typedef uint32_t (__stdcall *SetLargePageModeFunc)();

GetMethodPropertyFunc GetMethodProperty;
GetNumberOfMethodsFunc GetNumberOfMethods;
GetNumberOfFormatsFunc GetNumberOfFormats;
GetHandlerPropertyFunc GetHandlerProperty;
GetHandlerPropertyFunc2 GetHandlerProperty2;
CreateObjectFunc CreateObject;


std::map<std::wstring, CArcInfoEx*> ArchiveFormats;

//using namespace std;
namespace boostfs = boost::filesystem ;




bool SevenZip::FulllyLoaded = false;
bool SevenZip::AllFormatsEnabled = false;

void SevenZip::Init( PlatformPath& LibaryPath, bool enableAllFormat){

	HMODULE LibaryHandle = LoadLibrary(LibaryPath.c_str());

	if(LibaryHandle == NULL)throw std::exception("Failed to load 7zip library");


  GetMethodProperty = (GetMethodPropertyFunc)GetProcAddress(LibaryHandle, "GetMethodProperty");
  GetNumberOfMethods = (GetNumberOfMethodsFunc)GetProcAddress(LibaryHandle, "GetNumberOfMethods");
  GetNumberOfFormats = (GetNumberOfFormatsFunc)GetProcAddress(LibaryHandle, "GetNumberOfFormats");
  GetHandlerProperty = (GetHandlerPropertyFunc)GetProcAddress(LibaryHandle, "GetHandlerProperty");
  GetHandlerProperty2 = (GetHandlerPropertyFunc2)GetProcAddress(LibaryHandle, "GetHandlerProperty2");
  CreateObject = (CreateObjectFunc)GetProcAddress(LibaryHandle, "CreateObject");

  AllFormatsEnabled = enableAllFormat;

  if(!LoadFormats()){
    throw exception("Failed to load 7zip format list");
  }

  FulllyLoaded = true;

	return;
}

bool SevenZip::LoadFormats(){

	UInt32 numFormats = 1;
	
	if (GetNumberOfFormats != NULL){
		if(GetNumberOfFormats(&numFormats) != 0)return false;
	}
	
	if (GetHandlerProperty2 == NULL)numFormats = 1;

  NWindows::NCOM::CPropVariant prop;

  vector<wstring> extensions;

  for(int i = 0; i != numFormats ;i++){
    wstring name;

    if(SevenZip::ReadStringProp(i, NArchive::kName, name) != S_OK)continue;

    if(AllFormatsEnabled){
      boost::to_lower(name);
      auto entry = new CArcInfoEx(i);

      entry->GetExtensionList(extensions);

      BOOST_FOREACH(const wstring& ext, extensions){
        ArchiveFormats[ext] = entry;
      }

      extensions.clear();
    }else if(name == L"7z" || name == L"Rar" || name == L"zip"){

      boost::to_lower(name);

      ArchiveFormats[L'.'+name] = new CArcInfoEx(i);
    }
    
  }

	return true;
}

vector<string> SevenZip::GetSupportedFormats(){

  vector<string> FormatExtensions;

  BOOST_FOREACH(const wstring& format, ArchiveFormats|boost::adaptors::map_keys){
    FormatExtensions.push_back(WStringToUTF8STLString(format));
  }

  return FormatExtensions;
}

/*
CMyComPtr<IInArchive> archive;

if (createObjectFunc(&CLSID_CFormat7z, &IID_IInArchive, (void **)&archive) != S_OK){
throw exception("Can not get class object");
}

CInFileStream *fileSpec = new CInFileStream;
CMyComPtr<IInStream> file = fileSpec;

if (!fileSpec->Open(archiveName))
{
PrintError("Can not open archive file");
return 1;
}

{
CArchiveOpenCallback *openCallbackSpec = new CArchiveOpenCallback;
CMyComPtr<IArchiveOpenCallback> openCallback(openCallbackSpec);
openCallbackSpec->PasswordIsDefined = false;


if (archive->Open(file, 0, openCallback) != S_OK)
{
PrintError("Can not open archive");
return 1;
}
*/

Archive* SevenZip::OpenArchive(const PlatformPath& ArchivePath){

	auto ext = ArchivePath.extension();

	if(ext.empty()){
		throw exception("Archive file name has no extension");
	}

	auto reader = ArchiveFormats.find(ext.wstring());

	if(reader == ArchiveFormats.end()){
		throw exception("Unknown archive file extension");
	}

	const CArcInfoEx* format = (*reader).second;

	IInArchive* archive = format->GetInArchive();

	CInFileStream* inStream = new CInFileStream();
	 
	if(!inStream->Open(ArchivePath.c_str())){
		delete inStream;
    archive->Release();

		throw exception("Failed to open archive for reading");
	}


	if(archive->Open(inStream, 0, NULL) != S_OK){
    delete inStream;
    archive->Release();

		throw exception("Archive is either corrupt or 7zip was unable parse the archive header");
	}

	return new Archive(ArchivePath.string(), archive);

}

HRESULT SevenZip::ReadUTF8StringProp(uint32_t index, uint32_t propID, string &res){
  NWindows::NCOM::CPropVariant prop;

  RINOK(ReadProp(index, propID, prop));

  if(prop.vt == VT_EMPTY)res = string();
    
  if(prop.vt == VT_BSTR){
    UTF16ToUTF8STLString(prop.bstrVal, res);
  }else{
    return E_FAIL;
  }

  return S_OK;
}

HRESULT SevenZip::ReadStringProp(uint32_t index, uint32_t propID, wstring &res){
	NWindows::NCOM::CPropVariant prop;

	RINOK(ReadProp(index, propID, prop));

  if(prop.vt == VT_EMPTY)res = wstring();

  if(prop.vt == VT_BSTR){
    res = prop.bstrVal;
  }else{
    return E_FAIL;
  }
	
  return S_OK;
}

HRESULT SevenZip::ReadProp(uint32_t index, uint32_t propID, NWindows::NCOM::CPropVariant &prop){

	if(GetHandlerProperty2){
		return GetHandlerProperty2(index, propID, &prop);
	}else{
		return GetHandlerProperty(propID, &prop);
	}
}

HRESULT SevenZip::ReadBoolProp(uint32_t index, uint32_t propID, bool &res){
	NWindows::NCOM::CPropVariant prop;

	RINOK(ReadProp(index, propID, prop));

	if (prop.vt == VT_BOOL){
		res = (prop.boolVal != VARIANT_FALSE);
	}else if (prop.vt != VT_EMPTY){
		return E_FAIL;
	}

	return S_OK;
}

bool SevenZip::IsLoaded(){
  return FulllyLoaded;
}

CArcInfoEx::CArcInfoEx(int formatIndex) 
  :FormatIndex(formatIndex){

  CPropVariant prop;

  if(SevenZip::ReadUTF8StringProp(formatIndex, NArchive::kName, Name) != S_OK)return;

  if(SevenZip::ReadProp(formatIndex, NArchive::kClassID, prop) != S_OK || prop.vt != VT_BSTR)return;

  ClassID = *(const GUID *)prop.bstrVal;

  string extensions;

  if(SevenZip::ReadUTF8StringProp(formatIndex, NArchive::kExtension, extensions) != S_OK)return;
}

IInArchive* CArcInfoEx::GetInArchive() const{

  IInArchive* incArc = NULL;

  if(CreateObject(ClassID, IID_IInArchive, (void **)&incArc) != S_OK){
    throw exception("CArcInfoEx: Could not get IInArchive class object");
  }

  return incArc;
}

IOutArchive* CArcInfoEx::GetOutArchive() const{

  IOutArchive* outArc = NULL;

  if(CreateObject(ClassID, IID_IOutArchive, (void **)&outArc) != S_OK){
    throw exception("CArcInfoEx: Could not get IOutArchive class object");
  }

  return outArc;
}

void CArcInfoEx::GetExtensionList( vector<wstring>& extensions ) const
{
  wstring extString;

  using namespace boost::algorithm;

  if(SevenZip::ReadStringProp(FormatIndex, NArchive::kExtension, extString) != S_OK)return;

  if(extString.find(' ') == string::npos){
    extensions.push_back(extString);
  }else{
    split(extensions, extString, [](const wchar_t& c){ return c == ' ';});
  }
}


bool CArcInfoEx::SupportsUpdatingArchives(){

  bool result = false;

  if(SevenZip::ReadBoolProp(FormatIndex, NArchive::kUpdate, result) != S_OK )return false;

  return result;
}

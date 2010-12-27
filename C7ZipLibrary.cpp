#include "stdafx.h"
#include "Archive.h"
#include "C7ZipLibrary.h"


#include "7zip/CPP/Windows/PropVariant.h"
#include "7zip/CPP/Common/MyCom.h"
#include "7zip/CPP/7zip/Archive/IArchive.h"
#include "7zip/CPP/7zip/Common/FileStreams.h"
#include "7ZipFormatInfo.h"

#include <unordered_set>


C7ZipLibrary::C7ZipLibrary(void* libaryHandle){

	LibaryHandle = (HMODULE)libaryHandle;

	GetMethodProperty = (GetMethodPropertyFunc)GetProcAddress((HMODULE)LibaryHandle, "GetMethodProperty");
	GetNumberOfMethods = (GetNumberOfMethodsFunc)GetProcAddress((HMODULE)LibaryHandle, "GetNumberOfMethods");
	GetNumberOfFormats = (GetNumberOfFormatsFunc)GetProcAddress((HMODULE)LibaryHandle, "GetNumberOfFormats");
	GetHandlerProperty = (GetHandlerPropertyFunc)GetProcAddress((HMODULE)LibaryHandle, "GetHandlerProperty");
	GetHandlerProperty2 = (GetHandlerPropertyFunc2)GetProcAddress((HMODULE)LibaryHandle, "GetHandlerProperty2");
	CreateObject = (CreateObjectFunc)GetProcAddress((HMODULE)LibaryHandle, "CreateObject");
	SetLargePageMode = (SetLargePageModeFunc)GetProcAddress((HMODULE)LibaryHandle, "SetLargePageMode");

	LoadFormats();
}

using namespace std;
namespace boostfs = boost::filesystem ;


static const wchar_t* Extensions[] = {_T(".zip"), _T(".7z"), _T(".rar")};

unordered_set<wstring> ValidExtensions(0);

C7ZipLibrary* C7ZipLibrary::Init( PlatformPath& LibaryPath )
{

	BOOST_FOREACH(const wchar_t* s, Extensions){
		ValidExtensions.insert(wstring(s));
	}

#ifdef _WIN32
	void* pHandler = LoadLibrary(LibaryPath.c_str());
#else
	void* pHandler = dlopen("7z.so", RTLD_LAZY | RTLD_GLOBAL);
#endif

	if (pHandler == NULL)throw std::exception("Failed to load 7zip library");

	return new C7ZipLibrary(pHandler);
}

C7ZipLibrary::~C7ZipLibrary(void){
}

void SplitString(const wstring &srcString, vector<wstring> &destStrings){
	destStrings.clear();
	wstring s(L".");
	size_t len = srcString.length();
	if (len == 0)return;

	for(size_t i = 0; i < len; i++){
		wchar_t c = srcString[i];
		
		if(c == L' '){
			if(!s.empty()){
				destStrings.push_back(s);
				s.clear();
				s.push_back('.');
			}
		}else{
			s += c;
		}
	}

	if(!s.empty())destStrings.push_back(s);
}

bool C7ZipLibrary::LoadFormats(){

	UInt32 numFormats = 1;
	
	if (GetNumberOfFormats != NULL){
		if(GetNumberOfFormats(&numFormats) != 0)return false;
	}
	
	if (GetHandlerProperty2 == NULL)numFormats = 1;
	
	for(UInt32 i = 0; i < numFormats; i++){
		wstring name;
		bool updateEnabled = false;
		bool keepName = false;
		GUID classID;
		wstring ext, addExt;
	
		if(ReadStringProp(i, NArchive::kName, name) != S_OK)continue;
	
		if(ReadStringProp(i, NArchive::kExtension, ext) != S_OK)continue;

		vector<wstring> Exts(0);

		SplitString(ext, Exts);

		wstring* supportedFormat = NULL;
		

		BOOST_FOREACH(wstring& ext, Exts){
			if(ValidExtensions.find(ext) != ValidExtensions.end()){
				supportedFormat = &ext;
				break;
			}
		}

		if(!supportedFormat) continue;

		NWindows::NCOM::CPropVariant prop;
		if (ReadProp(i, NArchive::kClassID, prop) != S_OK)continue;

		if (prop.vt != VT_BSTR)continue;
	
		classID = *(const GUID *)prop.bstrVal;
	
		
	
		//if(ReadStringProp(i, NArchive::kAddExtension, addExt) != S_OK)continue;
	
		ReadBoolProp(i, NArchive::kUpdate, updateEnabled);
	
		if (updateEnabled){
			ReadBoolProp(i, NArchive::kKeepName, keepName);
		}
		
		C7ZipFormatInfo  pInfo;

		pInfo.m_Name = name;
		pInfo.m_KeepName = keepName;
		pInfo.m_ClassID = classID;
		pInfo.m_UpdateEnabled = updateEnabled;
		pInfo.FormatIndex = i;

		ArchiveFormats[*supportedFormat] = pInfo;
	}

	return true;
}

FileSource* C7ZipLibrary::OpenArchive( const PlatformPath& ArchivePath){

	auto ext = ArchivePath.extension();

	if(ext.empty()){
		throw exception("Archive file name has no extension");
	}

	auto reader = ArchiveFormats.find(ext.wstring());

	if(reader == ArchiveFormats.end()){
		throw exception("Unknown archive file extension");
	}

	const C7ZipFormatInfo& format = (*reader).second;

	IInArchive* archive;
	CreateObject(&format.m_ClassID, &IID_IInArchive, (void **)&archive);

	CInFileStream* inStream = new CInFileStream();
	 
	if(!inStream->Open(ArchivePath.c_str())){
		delete inStream;
		throw exception("Failed to open archive for reading");
	}


	if(archive->Open(inStream, 0, NULL) != S_OK){
		throw exception("Archive is either corrupt or 7zip was unable parse the archive header");
	}

	return new Archive(this, ArchivePath.string(), archive);

}

HRESULT C7ZipLibrary::ReadStringProp(uint32_t index, uint32_t propID, wstring &res){
	NWindows::NCOM::CPropVariant prop;

	RINOK(ReadProp(index, propID, prop));

	if (prop.vt == VT_BSTR){
		res = prop.bstrVal;
	}else if (prop.vt != VT_EMPTY){
		return E_FAIL;
	}
	return S_OK;

}

HRESULT C7ZipLibrary::ReadProp(uint32_t index, uint32_t propID, NWindows::NCOM::CPropVariant &prop){

	if(GetHandlerProperty2){
		return GetHandlerProperty2(index, propID, &prop);
	}else{
		return GetHandlerProperty(propID, &prop);
	}
}

HRESULT C7ZipLibrary::ReadBoolProp(uint32_t index, uint32_t propID, bool &res){
	NWindows::NCOM::CPropVariant prop;

	RINOK(ReadProp(index, propID, prop));

	if (prop.vt == VT_BOOL){
		res = (prop.boolVal == VARIANT_FALSE);
	}else if (prop.vt != VT_EMPTY){
		return E_FAIL;
	}
	return S_OK;
}
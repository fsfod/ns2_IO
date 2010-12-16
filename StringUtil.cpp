#include "StdAfx.h"
#include "StringUtil.h"
#include <boost/algorithm/string.hpp>

using namespace std;


void WStringToUTF8STLString(const wstring& wstr, string& StringResult){
	UTF16ToUTF8STLString(wstr.c_str(), wstr.size(), StringResult);
}

void UTF16ToUTF8STLString(const wchar_t* wString, string& StringResult){
	UTF16ToUTF8STLString(wString, wcslen(wString), StringResult);
}

void UTF16ToUTF8STLString(const wchar_t* wString, int WStringLength, string& StringResult){

	StringResult.resize(WStringLength);

	int result = WideCharToMultiByte(CP_UTF8, 0, wString, WStringLength, (char*)StringResult.c_str(), StringResult.capacity(), NULL, false);

	if(result == 0){
	 int lasterror = GetLastError();
		
		if(lasterror== ERROR_INSUFFICIENT_BUFFER){
			result = WideCharToMultiByte(CP_UTF8, 0, wString, WStringLength, (char*)StringResult.c_str(), 0, NULL, false);
			StringResult.resize(result+1, 0);

			result = WideCharToMultiByte(CP_UTF8, 0, wString, WStringLength, (char*)StringResult.c_str(), StringResult.capacity(), NULL, false);

			if(result == 0){
				throw exception("error while converting UTF8 to UTF16");
			}
		}else{
			throw exception("error while converting UTF16 to UTF8");
		}
	}
}

void UTF8StringToWString(const string& stlstr, wstring& StringResult){
	UTF8ToWString(stlstr.c_str(), stlstr.size(), StringResult);
}

void UTF8ToWString(const char* String, wstring& StringResult){
	UTF8ToWString(String, strlen(String), StringResult);
}

void UTF8ToWString(const char* String, int StringLength, wstring& StringResult){

	StringResult.resize(StringLength);

	int size = MultiByteToWideChar(CP_UTF8,0, String, StringLength, (wchar_t*)StringResult.c_str(), StringLength+1);

	if(size == 0){
		int lasterror = GetLastError();

		if(lasterror== ERROR_INSUFFICIENT_BUFFER){
			size = MultiByteToWideChar(CP_UTF8,0, String, StringLength, (wchar_t*)StringResult.c_str(), 0);
			StringResult.resize(size+1, 0);

			size = MultiByteToWideChar(CP_UTF8,0, String, StringLength, (wchar_t*)StringResult.c_str(), size);

			if(size == 0){
				throw exception("error while converting UTF8 to UTF16");
			}
		}else{
			throw exception("error while converting UTF8 to UTF16");
		}
	}
}


void InplaceNormalizePath(PathString& path){
	boost::to_lower(path);
	replace(path, '\\', '/');
}

std::string NormalizedPath(BSTR s){
	
	int len = wcslen(s);

	if(s[0]  == '/' || s[0]  == '\\'){
		s = s+1;
		--len;
	}
	
	//
	if(s[len-1]  == '/' || s[len-1]  == '\\'){
		--len;
	}

	std::string path;

	UTF16ToUTF8STLString(s, len, path);

	boost::to_lower(path);

	replace(path, '\\', '/');

	return path;
}

string NormalizedPath(const char* s, int len){

	if(s[0]  == '/' || s[0]  == '\\'){
		s = s+1;
		--len;
	}

	//
	if(s[len-1]  == '/' || s[len-1]  == '\\'){
		--len;
	}

	std::string path(s, len);

	boost::to_lower(path);

	replace(path, '\\', '/');

	return path;
}

std::wstring AsciiToWString(const char* s, int len){
	
	wstring StringResult;
	StringResult.resize(len);


	for(int i = 0; i < len ; i++){
		StringResult[i] = s[i];
	}

	return StringResult;
}

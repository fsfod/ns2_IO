#include "StdAfx.h"
#include "StringUtil.h"

using namespace std;

int UTF16ToUTF8STLString(const wchar_t* wString, string& StringResult){

	int WStringLength = wcslen(wString);

	StringResult.resize(WStringLength);

	int result = WideCharToMultiByte(CP_UTF8, 0, wString, WStringLength, (char*)StringResult.c_str(), StringResult.capacity(), NULL, false);

	if(result == 0 && GetLastError() == ERROR_INSUFFICIENT_BUFFER){
		result = WideCharToMultiByte(CP_UTF8, 0, wString, WStringLength, (char*)StringResult.c_str(), 0, NULL, false);
		StringResult.resize(result+1, 0);

		result = WideCharToMultiByte(CP_UTF8, 0, wString, WStringLength, (char*)StringResult.c_str(), StringResult.capacity(), NULL, false);
	}

	return result;
}

int UTF8ToWString(const char* String, wstring& StringResult){

	int StringLength = strlen(String);
	StringResult.resize(StringLength);

	int size = MultiByteToWideChar(CP_UTF8,0, String, StringLength, (wchar_t*)StringResult.c_str(), StringLength+1);

	if(size == 0 && GetLastError() == ERROR_INSUFFICIENT_BUFFER){
		size = MultiByteToWideChar(CP_UTF8,0, String, StringLength, (wchar_t*)StringResult.c_str(), 0);
		StringResult.resize(size+1, 0);

		size = MultiByteToWideChar(CP_UTF8,0, String, StringLength, (wchar_t*)StringResult.c_str(), size);
	}

	return size;
}
#pragma once
#include "stdafx.h"



void UTF8StringToWString(const std::string& stlstr, std::wstring& StringResult);
void UTF8ToWString(const char* String, std::wstring& StringResult);
void UTF8ToWString(const char* String, int slength, std::wstring& StringResult);

void UTF16ToUTF8STLString(const wchar_t* wString, std::string& StringResult);
void WStringToUTF8STLString(const std::wstring& wstr, std::string& StringResult);
void UTF16ToUTF8STLString(const wchar_t* wString, int slength, std::string& StringResult);
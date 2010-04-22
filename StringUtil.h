#pragma once
#include "stdafx.h"

int UTF16ToUTF8STLString(const wchar_t* wString, std::string& StringResult);
int UTF8ToWString(const char* String, std::wstring& StringResult);

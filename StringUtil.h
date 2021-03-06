#pragma once
#include "stdafx.h"
#include <locale>
#include <cvt/wstring> 
#include <codecvt>


static const wchar_t* InvalidPathChars = _T(":%|<>\"$");
static const wchar_t* InvalidFileNameChars = _T("[]:%|<>\"'$/\\");

static const char* InvalidFileNameCharsA = "[]:%|<>\"'$/\\";

std::wstring AsciiToWString(const char* s, int len);

//maybe switch to std::wstring_convert<std::codecvt_utf16<wchar_t> > to convert strings
//std::wstring_convert<std::codecvt_utf8_utf16<wchar_t> > ft;
//std::wstring_convert<> myconv;
//std::codecvt<wchar_t, char, std::mbstate_t> ft;

void UTF8StringToWString(const std::string& stlstr, std::wstring& StringResult);
//std::wstring UTF8StringToWString(const std::string& stlstr);

static inline std::wstring UTF8StringToWString(const std::string& stlstr){
  wstring result;
  UTF8StringToWString(stlstr, result);
  return result;
}

void UTF8ToWString(const char* String, std::wstring& StringResult);
void UTF8ToWString(const char* String, int slength, std::wstring& StringResult);


void UTF16ToUTF8STLString(const wchar_t* wString, std::string& StringResult);
void UTF16ToUTF8STLString(const wchar_t* wString, int slength, std::string& StringResult);

static string UTF16ToUTF8STLString(const wchar_t* wString){
  string result;
    UTF16ToUTF8STLString(wString, result);
  return result;
}

void UTF16ToUTF8STLString(const wchar_t* wString, int slength, std::string& StringResult);

void WStringToUTF8STLString(const std::wstring& wstr, std::string& StringResult);

std::string WStringToUTF8STLString(const std::wstring& wstr);

//void CheckPathString(const PathString& path);
//void InplaceNormalizePath(PathString& path);
std::string NormalizePath(const wchar_t* s);
std::string NormalizePath(const char* s, int len);

static inline std::string NormalizePath(const char* s){
  return NormalizePath(s, strlen(s));
}

static inline std::string NormalizePath(const std::string& s){
  return NormalizePath(s.c_str(), s.size());
}
#include "StdAfx.h"
#include "PathStringArg.h"
#include <boost\algorithm\string\case_conv.hpp>

//using namespace std;

PathStringArg::~PathStringArg(void){
}

//Normalized
string PathStringArg::GetNormalizedPath() const{
	return NormalizedPath(s, len);
}

string PathStringArg::GetExtension() const{

  if(len == 0 || s[0] == 0)return string();

    const char* dot =  strchr(s, '.');

    if(dot == NULL){
      return string();
    }
    
  int ExtIndex = (int)(dot-s);

  string ext(s+ExtIndex, len-ExtIndex);

  boost::to_lower(ext);

  return ext;
}

const PlatformString& PathStringArg::GetConvertedNativePath()const{
	//the lua interfacing functions should be calling ConvertAndValidate on us before they pass it onto the real function
	_ASSERT(CachedConvertedString.empty() == false || len == 0);

	return CachedConvertedString;
}

PlatformPath PathStringArg::CreatePath(const PlatformPath& Root)const{
	//the lua interfacing functions should be calling ConvertAndValidate on us before they pass it onto the real function
	_ASSERT(CachedConvertedString.empty() == false || len == 0);

	return Root/CachedConvertedString;
}

PlatformPath PathStringArg::CreateSearchPath(const PlatformPath& Root, const PathStringArg& SearchPath)const{
	//the lua interfacing functions should be calling ConvertAndValidate on us before they pass it onto the real function
	_ASSERT(CachedConvertedString.empty() == false || len == 0);

  const PlatformPath& Path = CachedConvertedString.empty() ? Root : Root/CachedConvertedString;

	if(SearchPath.len == 0){
		return Path/_T("*");
	}else{
		return Path/SearchPath.GetConvertedNativePath();
	}
}

const PlatformString& PathStringArg::ConvertAndValidate(const char* VaribleName){

#ifdef UTF8Paths
	UTF8ToWString(s, len, CachedConvertedString);
#else
	CachedConvertedString = AsciiToWString(s, len);
#endif

	if(CachedConvertedString.find_first_of(InvalidPathChars) != PathString::npos){
		throw std::runtime_error(string(VaribleName)+" cannot contain any of these chars :%|<>\"");
	}

  //size_t dotindex = CachedConvertedString.find('.');

  //check thats theres not more than 1 dot in the string
  if(CachedConvertedString.find(L"..") != string::npos){
		throw std::runtime_error(string(VaribleName)+" cannot contain the directory up string \"..\"");
	}

	return CachedConvertedString;
}
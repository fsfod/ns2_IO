#include "StdAfx.h"
#include "PathStringArg.h"
#include <boost\algorithm\string\case_conv.hpp>

using namespace std;

PathStringArg::~PathStringArg(void){
}

//Normalized
std::string PathStringArg::GetNormalizedPath() const
{
	return NormalizedPath(s, len);
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

	if(SearchPath.len == 0){
		return Root/CachedConvertedString/_T("*");
	}else{
		return Root/CachedConvertedString/SearchPath.GetConvertedNativePath();
	}
}

const PlatformString& PathStringArg::ConvertAndValidate(const char* VaribleName){

#ifdef UTF8Paths
	UTF8ToWString(s, len, CachedConvertedString);
#else
	CachedConvertedString = AsciiToWString(s, len);
#endif

	if(CachedConvertedString.find_first_of(InvalidPathChars) != PathString::npos){
		throw runtime_error(string(VaribleName)+" cannot contain any of these chars :%|<>\"");
	}

	if(CachedConvertedString.find(L"..") != -1){
		throw runtime_error(string(VaribleName)+" cannot contain the directory up string \"..\"");
	}

	return CachedConvertedString;
}
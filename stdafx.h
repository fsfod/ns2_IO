// stdafx.h : include file for standard system include files,
// or project specific include files that are used frequently, but
// are changed infrequently
//

#pragma once
#define _USE_32BIT_TIME_T
#include "targetver.h"


#define WIN32_LEAN_AND_MEAN             // Exclude rarely-used stuff from Windows headers
// Windows Header Files:
#include <windows.h>

extern "C" {
	#include "lauxlib.h"
}

#include <tchar.h>

#include <string>
#include <vector>
#include <cstdint>

#if defined(UNICODE)
	#define UnicodePathString

	typedef	std::wstring PathString;

	struct PathStringArg : public PathString{};
#else
	typedef	std::string PathString;
#endif

typedef PathString::value_type PathChar;

#include <luabind/luabind.hpp>

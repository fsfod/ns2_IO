// stdafx.h : include file for standard system include files,
// or project specific include files that are used frequently, but
// are changed infrequently
//

#pragma once
#define _USE_32BIT_TIME_T
#include "targetver.h"

//#define NOCOMM
//#define NOATOM
//#define NOGDI
//#define NOMSG

/*
#define NOGDICAPMASKS
#define NOVIRTUALKEYCODES
#define NOWINMESSAGES
#define NOWINSTYLES
#define NOSYSMETRICS
#define NOMENUS
#define NOICONS
#define NOKEYSTATES
#define NOSYSCOMMANDS
#define NORASTEROPS
#define NOSHOWWINDOW
#define OEMRESOURCE
//#define NOATOM
#define NOCLIPBOARD
#define NOCOLOR
#define NOCTLMGR
#define NODRAWTEXT
//#define NOGDI
#define NOKERNEL
#define NOUSER
#define NOMB
#define NOMEMMGR
#define NOMETAFILE
#define NOMINMAX
//#define NOMSG




*/
#define NOOPENFILE
#define NOSCROLL
#define NOSERVICE
#define NOSOUND
#define NOTEXTMETRIC
#define NOWH
#define NOWINOFFSETS
#define NOKANJI
#define NOHELP
#define NOPROFILER
#define NODEFERWINDOWPOS
#define NOOPENFILE
#define NOSCROLL
#define NOSERVICE
#define NOSOUND
#define NOTEXTMETRIC
#define NOWH
#define NOWINOFFSETS
#define NOKANJI
#define NOHELP
#define NOPROFILER
#define NODEFERWINDOWPOS
#define NOMCX

#include <INITGUID.H>

#include <Windows.h>
#include <shlwapi.h>

extern "C" {
	#include "lauxlib.h"
}

#include <cstdint>
#include <tchar.h>

#include <string>
#include <vector>
#include <map>
#include <hash_map>
#include <boost/foreach.hpp>

#define BOOST_FILESYSTEM_VERSION 3
#define BOOST_FILESYSTEM_NO_DEPRECATED

#include <boost/filesystem.hpp>
#include <boost/range/algorithm.hpp>

namespace boostfs = boost::filesystem;

#if defined(UNICODE)
	#define UnicodePathString
	typedef boostfs::path PlatformPath;
	typedef	std::string PathString;
#else
	typedef boostfs::path PlatformPath;
	typedef	std::string PathString;
#endif

using boost::replace;


//see PathStringConverter.h for all the magic of PathStringArg
//and also http://www.rasterbar.com/products/luabind/docs.html#adding-converters-for-user-defined-types

typedef PathString::value_type PathChar;

typedef std::wstring PlatformString;

#include "PathStringArg.h"

extern std::string DumpStack(lua_State *L);

#pragma warning( disable: 4251)

#define LUABIND_NO_RTTI
#define LUABIND_DYNAMIC_LINK
#include <luabind/luabind.hpp>

//just include our convert here instead of in the cpp files that expose functions to lua that use PathStringArg args
#include "PathStringConverter.h"
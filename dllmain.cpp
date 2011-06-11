// dllmain.cpp : Defines the entry point for the DLL application.
#include "stdafx.h"
#include "NS_IOModule.h"
#include "SavedVariables.h"
#include <luabind/tag_function.hpp>
#include <luabind/adopt_policy.hpp>
#include "PathStringConverter.h"
#include <boost/system/system_error.hpp>
#include "SourceManager.h"
#include <delayimp.h>

HMODULE LibaryHandle = 0;

#pragma comment(lib, "delayimp")

void* WINAPI DelayLoadHook(unsigned dliNotify, PDelayLoadInfo  pdli){
  
  if(dliNotify != dliNotePreLoadLibrary || _stricmp(pdli->szDll, "luabind.dll") != 0)return NULL;

  LibaryHandle = GetModuleHandleA("NS2_IO.dll");

  if(LibaryHandle == NULL)return NULL;

  const int BuffSize = 255;
  wchar_t PathBuf[BuffSize];
  wchar_t* path = PathBuf;
  int PathSize = GetModuleFileNameW(LibaryHandle, PathBuf, BuffSize);
  
  if(PathSize == 0)return NULL;


  if(PathSize >= BuffSize-12){
   int CurrentSize = 300;
   int result;
   
   do{
     path = new wchar_t[CurrentSize];

     result = GetModuleFileNameW(LibaryHandle, path, CurrentSize);

     if(result >= CurrentSize-12){
       CurrentSize *= 2;
       delete path;
     }else{
       if(result == 0){
         delete path;
         return NULL;
       }else{
       	break;
       }
     }
   }while(true);
  }

  wchar_t* pos = wcsrchr(path, '\\');
  if(pos == NULL)return NULL;

  wcscpy(pos+1, L"luabind.dll");

  HMODULE hmod = LoadLibraryW(path);

  if(path != PathBuf)delete path;

 return (void*)hmod;
}

PfnDliHook __pfnDliNotifyHook2 = (PfnDliHook)&DelayLoadHook;


BOOL APIENTRY DllMain(HMODULE hModule,DWORD  ul_reason_for_call, LPVOID lpReserved){

	LibaryHandle = hModule;

	return TRUE;
}


extern "C" __declspec(dllexport) int luaopen_NS2_IO(lua_State* L){
	
	LuaModule::Initialize(L);

	using namespace luabind;

	open(L);

	module(L,"NS2_IO")[
		def("FileExists", &LuaModule::FileExists),
    def("DirectoryExists", &LuaModule::DirectoryExists),
		def("FileSize", &LuaModule::GetFileSize),
		def("FindFiles", &LuaModule::FindFiles),
		def("FindDirectorys", &LuaModule::FindDirectorys),
		def("DateModified",  &LuaModule::GetDateModified),
		def("GetGameString",  &LuaModule::GetGameString),
		def("GetDirRootList",  &LuaModule::GetDirRootList),
		def("GetCommandLineRaw", &LuaModule::GetCommandLineRaw),
    
    def("GetSupportedArchiveFormats", &LuaModule::GetSupportedArchiveFormats),
		def("OpenArchive", &LuaModule::OpenArchive),
		def("OpenArchive", &LuaModule::OpenArchive2),
		def("IsRootFileSource", &LuaModule::IsRootFileSource),
    def("LoadLuaDllModule", &LuaModule::LoadLuaDllModule),

    def("MountMapArchive", &LuaModule::MountMapArchive),
    def("UnMountMapArchive", &LuaModule::UnMountMapArchive),
    def("MountSource", &SourceManager::MountSource),
    def("OpenModsFolder", &LuaModule::OpenModsFolder),
    //def("ExtractResourceToPath", &LuaModule::ExtractResource),
		
		FileSource::RegisterClass(),
    class_<DirectoryFileSource, FileSource>("DirectoryFileSource")
    .def("CreateChildSource", &DirectoryFileSource::CreateChildSource),

    class_<Archive, FileSource>("ArchiveFileSource")
		//class_<NSRootDir, bases<FileSource> >("RootDirectory")
	];

	SavedVariables::RegisterObjects(L);

	//push our module onto the stack tobe our return value
	lua_getglobal(L, "NS2_IO");

	//LuaModule

	return 1;
}
// dllmain.cpp : Defines the entry point for the DLL application.
#include "stdafx.h"
#include "NS_IOModule.h"

BOOL APIENTRY DllMain(HMODULE hModule,DWORD  ul_reason_for_call, LPVOID lpReserved){
	switch (ul_reason_for_call){
		case DLL_PROCESS_ATTACH:
		case DLL_THREAD_ATTACH:
		case DLL_THREAD_DETACH:
		case DLL_PROCESS_DETACH:
		break;
	}
	return TRUE;
}


static const luaL_reg NSIOlib[] = {
	{"GetGameString", LuaModule::GetGameString},
	{"Exists", LuaModule::FileExists},
	{"GetDirRootList", LuaModule::GetDirRootList},
	{"FindFiles", LuaModule::FindFilesInDir},
	{"FindDirectorys", LuaModule::FindDirectorys},
	{"FileSize", LuaModule::GetFileSize},
	{"DateModifed", LuaModule::GetDateModifed},
	{NULL, NULL}
};


extern "C" __declspec(dllexport) int luaopen_NS2_IO(lua_State* L){

	try{
		LuaModule::Initialize(L);

		luaL_register(L, "NS2_IO", NSIOlib);

		return 1;
	}catch(LuaErrorWrapper e){
		e.DoLuaError(L);
		return 0;
	}
}

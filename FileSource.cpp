#include "StdAfx.h"
#include "FileSource.h"

using namespace std;

int32_t FileSource::Lua_GetModifedTime(const PathStringArg& Path){
	int time = 0;

	if(this->GetModifiedTime(static_cast<PathString>(Path), time)){
		throw exception("RootDir::DateModifed cannot get the modified time of a file that does not exist");
	}

	return time;
}

bool FileSource::Lua_FileExists(const PathStringArg& path){
	return this->FileExists(static_cast<const PathString&>(path));
}

double FileSource::Lua_FileSize( const PathStringArg& Path ){
	double size = 0;

	if(this->FileSize(static_cast<const PathString&>(Path), size)){
		throw exception("RootDir::FileSize cannot get the size of a file that does not exist");
	}

	return size;
}

luabind::object FileSource::Lua_FindFiles( lua_State* L, const PathStringArg& SearchPath, const PathStringArg& NamePatten )
{

	FileSearchResult Results;
	this->FindFiles(SearchPath, NamePatten, Results);

	lua_createtable(L, Results.size(), 0);
	luabind::object table = luabind::object(luabind::from_stack(L,-1));

	auto it = Results.begin();

	for(uint32_t i = 0; i < Results.size() ; i++,it++){
		table[i+1] =  it->first;
	}

	return table;
}

luabind::object FileSource::Lua_FindDirectorys(lua_State *L, const PathStringArg& SearchPath, const PathStringArg& NamePatten){

	FileSearchResult Results;
	int count = this->FindDirectorys(SearchPath, NamePatten, Results);

	lua_createtable(L, Results.size(), 0);
	luabind::object table = luabind::object(luabind::from_stack(L,-1));

	auto it = Results.begin();

	for(int i = 0; i < count ; i++,it++){
		table[i+1] = it->first.c_str();
	}

	return table;
}

luabind::scope FileSource::RegisterClass(){

	return luabind::class_<FileSource>("FileSource")
		.def_readonly("Path", &FileSource::Lua_get_Path)
		.def("FileExists", &FileSource::Lua_FileExists)
		.def("FileSize", &FileSource::Lua_FileSize)
		.def("DateModifed", &FileSource::Lua_GetModifedTime)
		.def("FindDirectorys", &FileSource::Lua_FindDirectorys)
		.def("FindFiles", &FileSource::Lua_FindFiles)
		.def("LoadLuaFile", &FileSource::LoadLuaFile);
}
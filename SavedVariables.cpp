#include "StdAfx.h"
#include "SavedVariables.h"
#include "NS_IOModule.h"
#include "StringUtil.h"


PathString SavedVariables::SavedVariablesFolderPath(_T(""));

using namespace std;

SavedVariables::SavedVariables(lua_State* L, const string& fname,  luabind::table<luabind::object> const& table){

	SavedVariableFile = NULL;

	if(luabind::type(table) == LUA_TTABLE){
		auto end = luabind::iterator();

		try{
			for(auto it = luabind::iterator(table); it != end; it++){
				string TableName = luabind::object_cast<string>(*it);
				VariableNames.push_back(TableName);
			}

		}catch(luabind::cast_failed e){
			throw exception("Found an entry in the TableNames list that was not a string");
		}
	}

	UTF8StringToWString(fname, FileName);

	for_each(FileName.begin(), FileName.end(), 
		[this](const PathString::value_type& c){
			if(c == '.' || c == '/'|| c == '\\'){
				FileName.clear();
				throw exception("the SavedVariables name cannot contain any of these characters \"\\\"./");
			}
		}
	);

	CreateShutdownCallback(L);
	FileName.append(_T(".lua"));
}

 
SavedVariables::~SavedVariables(){

	if(ShutdownClosureThisPointer != NULL){
		*ShutdownClosureThisPointer = NULL;
	}
	
	if(SavedVariableFile != NULL){
		fclose(SavedVariableFile);
		SavedVariableFile = NULL;
	}
}

/*
void SavedVariables::RegisterVariable(const string& varname){
	
	VariableNames.push_back(varname);
	
	int ArgumentCount = lua_gettop(L);

	for(int i = 0; i < ArgumentCount ; i++){
		size_t StringSize = 0;
		const char* str = luaL_checklstring(L, i, &StringSize);

		if(str != NULL){
			VariableNames.push_back(string(str));
		}
	}

	
}
*/

int SavedVariables::ShutdownCallback(lua_State *L){
	SavedVariables** savedvars = static_cast<SavedVariables**>(lua_touserdata(L, lua_upvalueindex(1)));

	try{
		//check to make sure our SavedVariables object has not been destroyed already
		if(savedvars[0] != NULL){
			(*savedvars)->ShutdownClosureThisPointer = NULL;
			(*savedvars)->Save(L);
		}
	}catch(...){
		delete savedvars;
		return 0;
	}

	delete savedvars;
	return 0;
}

void SavedVariables::CreateShutdownCallback(lua_State *L){
	
	lua_getglobal(L, "Script");
	lua_getfield(L, -1, "AddShutdownFunction");
	lua_remove(L, -2);

	ShutdownClosureThisPointer = new void*[1];
	ShutdownClosureThisPointer[0] = this;

	lua_pushlightuserdata(L, ShutdownClosureThisPointer);
	lua_pushcclosure(L, &ShutdownCallback, 1);
	lua_call(L,1, 0);
}

int SavedVariables::Save(lua_State *L){

	CheckCreateSVDir();

	PathString FilePath = static_cast<PathString>(SavedVariablesFolderPath+FileName);

#ifdef UNICODE
	SavedVariableFile = _wfopen(FilePath.c_str(),	_T("w"));

#else
	SavedVariableFile = fopen(FilePath.c_str(),	"w");
#endif
	
	if(SavedVariableFile == NULL){
		throw exception("failed to open SavedVariable file for writing");
	}

	for(int i = 0; i < VariableNames.size() ; i++){
		lua_getglobal(L, VariableNames[i].c_str());

		int CurrentStackPos = lua_gettop(L);

		if(lua_istable(L, -1)){
			fprintf(SavedVariableFile, "%s = {\n", VariableNames[i].c_str());
				SaveTable(L);
			fprintf(SavedVariableFile, "}\n");
		}

		_ASSERT_EXPR(lua_gettop(L) == CurrentStackPos, _T("Stack Position does not the position before the call to SaveTable"));

		lua_pop(L, 1);
	}


	fflush(SavedVariableFile);
	fclose(SavedVariableFile);

	SavedVariableFile = NULL;

	return 0;
}

void SavedVariables::SaveTable(lua_State *L){

	lua_pushnil(L);
	
	IndentString.push_back('\t');

	while(lua_next(L, -2) != 0){

		switch(lua_type(L, -2)){
			case LUA_TNUMBER:
				fprintf(SavedVariableFile, "%s[%g] = ",  IndentString.c_str(), lua_tonumber(L, -2));
			break;

			case LUA_TSTRING:
				fprintf(SavedVariableFile, "%s[\"%s\"] = ",  IndentString.c_str(), lua_tostring(L, -2));
			break;
		}

		switch(lua_type(L, -1)){
			case LUA_TNUMBER:
				{
				double num = lua_tonumber(L, -1);
				
					fprintf(SavedVariableFile, "%g,\n", num);
				};
			break;

			case LUA_TSTRING:
				fprintf(SavedVariableFile, "\"%s\",\n", lua_tostring(L, -1));
			break;

			case LUA_TBOOLEAN:
				fprintf(SavedVariableFile, "%s,\n", lua_tostring(L, -1));
			break;

			case LUA_TTABLE:
				fprintf(SavedVariableFile, "{\n");
				SaveTable(L);
				fprintf(SavedVariableFile, "%s},\n", IndentString.c_str());
			break;

			case LUA_TUSERDATA:
				if(SerializeCustomType(L, -1)){
					fprintf(SavedVariableFile, ",\n");
				}else{
					fprintf(SavedVariableFile, "nil, --[[Skipping userdata thats not a Vector or Angles]]\n");
				}
			break;
		}

		lua_pop(L, 1);
	}

	IndentString.pop_back();
}

bool SavedVariables::IsaWrapper(lua_State *L, int index, const char* type){
	
	lua_getfield(L, index, "isa");

	if(lua_isnil(L,-1)){
			lua_pop(L, 1);
		return false;
	}
	lua_pushvalue(L, index);
	lua_pushstring(L, type);
	lua_call(L, 2, 1);

	bool Result =  lua_toboolean(L, -1) != 0;
	lua_pop(L, 1);

	return Result;
}

bool SavedVariables::SerializeCustomType(lua_State *L, int index){

	bool Result = false;
		int ObjectIndex = (lua_gettop(L)-1)-index;

		if(IsaWrapper(L, ObjectIndex, "Vector")){
			lua_getfield(L, ObjectIndex, "x");
			lua_getfield(L, ObjectIndex, "y");
			lua_getfield(L, ObjectIndex, "z");

			fprintf(SavedVariableFile, "Vector(%g,%g,%g)", lua_tonumber(L, -3), lua_tonumber(L, -2), lua_tonumber(L, -1));

			Result = true;
			lua_pop(L, 3);
		}else{

			if(IsaWrapper(L, ObjectIndex, "Angles")){
				lua_getfield(L, ObjectIndex, "pitch");
				lua_getfield(L, ObjectIndex, "yaw");
				lua_getfield(L, ObjectIndex, "roll");

				fprintf(SavedVariableFile, "Angles(%g,%g,%g)", lua_tonumber(L, -3), lua_tonumber(L, -2), lua_tonumber(L, -1));
				Result = true;
				lua_pop(L, 3);
			}
		}

	return Result;
}

void SavedVariables::Load(lua_State *L){
	PathString FilePath = LuaModule::NSRoot+FileName;
	
#ifdef UNICODE
	string fullPath;
	WStringToUTF8STLString(FilePath, fullPath);
	//SavedVariableFile = _wfopen(FilePath.c_str(),	_T("w"));
#else
//	SavedVariableFile = fopen(FilePath.c_str(),	"w");
#endif

//adapted from 
		if(luaL_loadfile(L, fullPath.c_str()) == 0){//FIXME need to handle unicode paths
			//Set the chunk's environment to a table
			lua_newtable(L);

			lua_getfield(L, LUA_GLOBALSINDEX, "Vector");
			lua_setfield(L, -2, "Vector");
			lua_getfield(L, LUA_GLOBALSINDEX, "Angles");
			lua_setfield(L, -2, "Angles");

			lua_pushvalue(L, -1);
			if (!lua_setfenv(L, -3)){
				assert(false && "Chunk must be loaded");
			}
			//Push the chunk to the top, required by pcall
			lua_pushvalue(L, -2);
			//Remove the old chunk
			lua_remove(L, -3);
			//Call the chunk, it then inserts any globals into the environment table
			if (lua_pcall(L, 0, 0, 0) == 0){
				for (auto it = VariableNames.begin(), end = VariableNames.end(); it != end; ++it){
					lua_getfield(L, -1, it->c_str());
					if (lua_isnil(L, -1)){
						//Pop nil
						lua_pop(L, 1);
					}else{
						//Pop and place the value into the globals
						lua_setfield(L, LUA_GLOBALSINDEX, it->c_str());
					}
				}
				//Pop environment table
				lua_pop(L, 1);
				return;
			}
			lua_error(L);
		}
}

void SavedVariables::CheckCreateSVDir(){

	if(GetFileAttributes(SavedVariablesFolderPath.c_str()) == INVALID_FILE_ATTRIBUTES){
		CreateDirectory(SavedVariablesFolderPath.c_str(), NULL);
	}
}

void SavedVariables::RegisterObjects(lua_State *L){
	
	using namespace luabind;

	module(L)[class_<SavedVariables>("SavedVariables")
		.def(constructor<lua_State*, const string&, const luabind::table<luabind::object>&>())
		.def("Save", &SavedVariables::Save)
		.def("Load", &SavedVariables::Load)
	];

	/*
	luabind::object vec = call_function<luabind::object>(L, "Vector", 0, 0, 0);
	vec["isa"].push(L);
	isaFunction = lua_topointer(L, -1);
	lua_pop(L, 1);
	*/

	SavedVariablesFolderPath = LuaModule::NSRoot+_T("SavedVariables/");
}

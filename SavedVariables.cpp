#include "StdAfx.h"
#include "SavedVariables.h"
#include "NS_IOModule.h"
#include "StringUtil.h"
namespace boostfs = boost::filesystem;


PlatformPath SavedVariables::SavedVariablesFolderPath(_T(""));

//using namespace std;

SavedVariables::SavedVariables(lua_State* L, const string& fname, luabind::table<luabind::object> const& tableNames, luabind::table<> const& containerTable) :
	ExitAutoSave(true) {

	ContainerTable = containerTable;
	Init(L, fname, tableNames);
}

SavedVariables::SavedVariables(lua_State* L, const string& fname, luabind::table<luabind::object> const& tableNames)
	 :ExitAutoSave(true) {

	Init(L, fname, tableNames);
}

void SavedVariables::Init(lua_State* L, const string& fname, luabind::table<luabind::object> const& tableNames){	SavedVariableFile = NULL;

	if(luabind::type(tableNames) == LUA_TTABLE){
		auto end = luabind::iterator();

		try{
			for(auto it = luabind::iterator(tableNames); it != end; it++){
				string TableName = luabind::object_cast<string>(*it);
				VariableNames.push_back(TableName);
			}
		}catch(luabind::cast_failed e){
			throw exception("Found an entry in the TableNames list that was not a string");
		}
	}

	FileName = fname;

	if(FileName.wstring().find_first_of(InvalidFileNameChars) != string::npos){
			throw exception("the SavedVariables name cannot contain any of these characters  \\ \" . ");
	}

	CreateShutdownCallback(L);
	FileName.replace_extension(".lua");
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

char buf[1000];

string DumpStack(lua_State *L){
	
	memset(buf, 0, sizeof(buf));

	int count = lua_gettop(L);

	int pos = sprintf(buf,  "Stack Size = %i: ", count);


	for(int i = 1; i < count+1 ; i++){
		pos += sprintf(buf+pos,  "%i = %s, ", i, lua_typename(L, lua_type(L, i)) );
	}

	return buf;
}

void SavedVariables::Load(lua_State *L){
	auto FilePath = boostfs::absolute(FileName, SavedVariablesFolderPath);

	//just silently return if nothing has been saved before. The container table should have default values set in it by the owner
	if(!boostfs::exists(FilePath)){
		return;
	}


	int ContainerIndex = LUA_GLOBALSINDEX;

	//add these 2 early to the stack so we don't have to deal with fun shifting stack indexs
	if(ContainerTable.is_valid()) {
		ContainerTable.push(L);
		ContainerIndex = lua_gettop(L);
	}

	lua_newtable(L);

	int EnvTable = lua_gettop(L); 


	//adapted from http://danwellman.com/vws.html
	if(LuaModule::LoadLuaFile(L,FilePath, "") == 0){
		//Set the chunk's environment to a table

		//import the constructors for Vector and Angles
		lua_getfield(L, LUA_GLOBALSINDEX, "Vector");
		lua_setfield(L, EnvTable, "Vector");
		lua_getfield(L, LUA_GLOBALSINDEX, "Angles");
		lua_setfield(L, EnvTable, "Angles");

		lua_pushvalue(L, EnvTable);
		if (!lua_setfenv(L, -2)){
			assert(false && "Chunk must be loaded");
		}

		//Push the chunk to the top, required by pcall
		lua_pushvalue(L, EnvTable+1);
		//Remove the old chunk
		lua_remove(L, EnvTable+1);

		//Call the chunk, it then inserts any globals into the environment table
		if (lua_pcall(L, 0, 0, 0) == 0){
			for (auto it = VariableNames.begin(), end = VariableNames.end(); it != end; ++it){
				const char* FieldName = it->c_str();

				lua_getfield(L, EnvTable, FieldName);

				if (lua_isnil(L, -1)){
					lua_pop(L, 1);
				}else{
					lua_setfield(L, ContainerIndex, FieldName);
				}
			}
			
			if(ContainerTable.is_valid()) lua_pop(L, 1);

			//Pop environment table
			lua_pop(L, 1);

		 return;
		}
	}

	//we got an error while either parsing the saved variables file or when running it an error string should of been pushed onto the stack
	size_t len = 0;
	const char* s = lua_tolstring(L, -1, &len);

	if(s != NULL){
		throw exception(s);
	}else{
		throw std::runtime_error(string("Unknowned error while loading saved variables from ")+FileName.string());
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
	SavedVariables** savedvars = static_cast<SavedVariables**>(lua_touserdata(L, lua_upvalueindex(1)));;

	try{
		SavedVariables* SVObject = *savedvars;

		//check to make sure our SavedVariables object has not been destroyed already
		if(SVObject != NULL){
			SVObject->ShutdownClosureThisPointer = NULL;
			
			if(SVObject->ExitAutoSave){
				SVObject->Save(L);
			}
		}
	}catch(exception e){
		LuaModule::PrintMessage(L, "Error while auto saving SavedVariables:");
		LuaModule::PrintMessage(L, e.what());
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

	auto FilePath = SavedVariablesFolderPath/FileName;

#ifdef UNICODE
	SavedVariableFile = _wfopen(FilePath.c_str(),	_T("w"));

#else
	SavedVariableFile = fopen(FilePath.c_str(),	"w");
#endif
	
	if(SavedVariableFile == NULL){
		throw exception("failed to open SavedVariable file for writing");
	}

	int ContainerIndex = LUA_GLOBALSINDEX;

	if(ContainerTable.is_valid()) {
		ContainerTable.push(L);
		ContainerIndex = lua_gettop(L);
	}

	BOOST_FOREACH(const string& VariableName, VariableNames){
		lua_getfield(L, ContainerIndex, VariableName.c_str());

		int CurrentStackPos = lua_gettop(L);

		if(lua_type(L, -1) == LUA_TNIL){
			lua_pop(L, 1);
			continue;
		}

		fprintf(SavedVariableFile, "%s = ", VariableName.c_str());

		switch(lua_type(L, -1)){
			case LUA_TNUMBER:
				fprintf(SavedVariableFile, "%Lg\n", lua_tonumber(L, -1));
			break;

			case LUA_TSTRING:
				fprintf(SavedVariableFile, "\"%s\"\n", lua_tostring(L, -1));
			break;

			case LUA_TBOOLEAN:
				fprintf(SavedVariableFile, lua_toboolean(L, -1) ? "true\n" : "false\n");
			break;

			case LUA_TTABLE:
				fprintf(SavedVariableFile, "{\n");
					SaveTable(L);
				fprintf(SavedVariableFile, "}\n");
			break;

			case LUA_TUSERDATA:
				if(SerializeCustomType(L, -1)){
					fprintf(SavedVariableFile, "\n");
				}else{
					fprintf(SavedVariableFile, "nil --[[Skipping userdata thats not a Vector or Angles]]\n");
				}
			break;

			default:
				fprintf(SavedVariableFile, "nil --Skipping a %s value that cannot be serialized]]\n", lua_typename(L, lua_type(L, -1)));
			break;
		}

		//WriteValue(L);

		_ASSERT_EXPR(lua_gettop(L) == CurrentStackPos, _T("Stack Position is not the position before the call to SaveTable"));

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
	
	int CurrentTableIndex = 0;

	while(lua_next(L, -2) != 0){

		switch(lua_type(L, -2)){
			case LUA_TNUMBER:
			{
				double FloatIndex = lua_tonumber(L, -2);
					//leave out the index if we have a pure array
					//if we hit a nil hole we just revert back to bracketed indexes 
					if(CurrentTableIndex != -1){
						int index ;

						if(FloatIndex == (index = (int)FloatIndex) && index == CurrentTableIndex+1){
							CurrentTableIndex = index;
							fprintf(SavedVariableFile, IndentString.c_str());
						 break;
						}else{
							CurrentTableIndex = -1;
						}
					}

					fprintf(SavedVariableFile, "%s[%Lg] = ",  IndentString.c_str(), FloatIndex);
				}
			break;

			case LUA_TSTRING:
				fprintf(SavedVariableFile, "%s[\"%s\"] = ",  IndentString.c_str(), lua_tostring(L, -2));
			break;
		}

		switch(lua_type(L, -1)){
			case LUA_TNUMBER:
				fprintf(SavedVariableFile, "%Lg,\n", lua_tonumber(L, -1));
			break;

			case LUA_TSTRING:
				fprintf(SavedVariableFile, "\"%s\",\n", lua_tostring(L, -1));
			break;

			case LUA_TBOOLEAN:
				fprintf(SavedVariableFile, lua_toboolean(L, -1) ? "true,\n" : "false,\n");
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

/*
bool SavedVariables::EscapeString(const char* s, int length){

	const char* end = s+length, *start = s;

	vector<int> CharsToEscape;

	do{
		if(*s == 0 || *s == '\n' || *s == '\r'){
			CharsToEscape.push_back(s-start);
		}else if(*s == '\"'){

		}

	}while(s++ != end);

}
*/
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


void SavedVariables::CheckCreateSVDir(){

	if(!boostfs::exists(SavedVariablesFolderPath.c_str())){
		boostfs::create_directory(SavedVariablesFolderPath.c_str());
	}
}

void SavedVariables::SetExitAutoSaving(bool AutoSave){
	ExitAutoSave = AutoSave;
}

extern PlatformPath NSRootPath;

void SavedVariables::RegisterObjects(lua_State *L){
	
	using namespace luabind;

	module(L)[class_<SavedVariables>("SavedVariables")
		.def(constructor<lua_State*, const string&, const luabind::table<luabind::object>&>())
		.def(constructor<lua_State*, const string&, const luabind::table<luabind::object>&, const luabind::table<luabind::object>&>())
		.def("Save", &SavedVariables::Save)
		.def("Load", &SavedVariables::Load)
		.def_readwrite("ExitAutoSave", &SavedVariables::ExitAutoSave)
	];

	/*
	luabind::object vec = call_function<luabind::object>(L, "Vector", 0, 0, 0);
	vec["isa"].push(L);
	isaFunction = lua_topointer(L, -1);
	lua_pop(L, 1);
	*/

	SavedVariablesFolderPath = NSRootPath/"SavedVariables/";
}

#include "StdAfx.h"
#include "SavedVariables.h"
#include "NS_IOModule.h"
#include "StringUtil.h"
#include "SQLite3Database.h"

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

extern SQLiteDatabase clientDB;

void SavedVariables::Init(lua_State* L, const string& fname, luabind::table<luabind::object> const& tableNames){	SavedVariableFile = NULL;

  db = &clientDB;

	if(luabind::type(tableNames) == LUA_TTABLE){
		
    tableNames.push(L);
    lua_pushnil(L);

    while(lua_next(L, -2) != 0){
      int keyType = lua_type(L, -2);
      int valueType = lua_type(L, -1);

      if(valueType == LUA_TSTRING){
        VariableNames.push_back(lua_tostring(L, -1));
      }else{
        if(keyType == LUA_TSTRING){
          TableEntrys.push_back(SQLTableSerializer(L, lua_tostring(L, -2), -1, this));
        }
      }
      lua_pop(L, 1);
    }
  }

	//throw exception("Found an entry in the TableNames list that was not a string");

	FileName = fname;

	if(FileName.wstring().find_first_of(InvalidFileNameChars) != string::npos){
	  throw exception("the SavedVariables name cannot contain any of these characters  \\ \" . ");
	}
  
  Name = fname;

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

struct SQLLuaType{
  enum SQLLuaType_t :int{
    //SQLite compactly stores the numbers 0 and 1 and NULL values
    Boolean = 0,
    Table = 1,
  };
};

class SQLLoadHelper{

public:
  SQLLoadHelper(lua_State *L,int startStack) : StackStartIndex(startStack){
    lua_createtable(L, 250, 0);
    OverFlowTable = lua_gettop(L);
    
    StackStartIndex = startStack+1;
  }

  void PushTable(lua_State *L, int tableId){
    lua_rawgeti(L, OverFlowTable, tableId);

    if(lua_type(L, -1) == LUA_TNIL){
      lua_pop(L, 1);
      lua_newtable(L);

      //duplicate the table since rawset pops it from the stack and we need to leave it on the stack for the caller
      lua_pushvalue(L, -1);

      lua_rawseti(L, tableId, OverFlowTable);
    }
  }

  template<int columnOffset> bool LoadSQLValue(lua_State *L, SQLitePreparedStatment& LoadStatement, int typeTypeField, int valueType){

    int size;

    if(typeTypeField == SQLITE_NULL){
      switch(valueType){
      case SQLITE_INTEGER:
      case SQLITE_FLOAT:
        lua_pushnumber(L, LoadStatement.GetDouble(6));
        break;

      case SQLITE_TEXT:
        lua_pushlstring(L, (const char*)LoadStatement.GetString(6, size), size);
      break;

      case SQLITE_BLOB:
        lua_pushlstring(L, (const char*)LoadStatement.GetBlob(6, size), size);
        break;

      default:
        return false;
      }
    }else{
      switch(typeTypeField){
      case SQLLuaType::Boolean:
        lua_pushboolean(L, LoadStatement.GetInt32(5));
        break;

      case SQLLuaType::Table:
        PushTable(L, LoadStatement.GetInt32(5));
      break;

      default:
        return false;
      }
    }

    return true;
  }

private:
  int OverFlowTable;
  int StackStartIndex;
};


int SavedVariables::LoadSQL(lua_State *L){
  if(db == NULL){      
   return 0;
  }

  TableCount = 0;
  
  if(!db->TableExists(Name.c_str()) ){
    return 0;
  }

  if(ContainerTable.is_valid())return 0;

  SQLLoadHelper tableList(L, lua_gettop(L));

  ContainerTable.push(L);
  int ContainerIndex = lua_gettop(L);

  SQLitePreparedStatment LoadStatement = db->CreateStatmentFmt("SELECT * From [%s] ORDER BY TID, EntryID", Name.c_str());

  int currentArrayIndex = 1;

  while(LoadStatement.Step()){
    int keyType = LoadStatement.GetRowValueType(4);
    int keyTypeType = LoadStatement.GetRowValueType(3);
    
    int tableid = LoadStatement.GetInt32(2);

    if(keyTypeType == SQLITE_NULL && keyType == SQLITE_NULL){
      lua_pushnumber(L, currentArrayIndex++);
    }else{
      tableList.LoadSQLValue<3>(L, LoadStatement, keyTypeType, keyType);
    }

    tableList.LoadSQLValue<5>(L, LoadStatement, LoadStatement.GetRowValueType(5), LoadStatement.GetRowValueType(6));
 
    if(tableid == 0) {
      int size = 0;
      const char* FieldName = (const char*)LoadStatement.GetBlob(4, size);

      lua_setfield(L, ContainerIndex, FieldName);
    }
  }
}

void SavedVariables::Load(lua_State *L){
	auto FilePath = boostfs::absolute(FileName, SavedVariablesFolderPath);

  BOOST_FOREACH(SQLTableSerializer& entry, TableEntrys){
    entry.Load(L);
  }

	//just silently return if nothing has been saved before. The container table should have default values set in it by the owner
	if(!boostfs::exists(FilePath)){
		return;
	}
  
  LoadSQL(L);

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
		throw std::runtime_error(string("Unknown error while loading saved variables from ")+FileName.string());
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
	}catch(exception& e){
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

template<int luaIndex, int columnOffset> inline bool SavedVariables::SaveLuaValue(lua_State *L, SQLitePreparedStatment& stmt, int& saveTableindex){

  int FieldType = -1;

  switch(lua_type(L, luaIndex)){
    case LUA_TNUMBER:{
      double num = lua_tonumber(L, luaIndex);
      int value = (int)num;

      if(num == value){
        stmt.BindValue(columnOffset+1, value);
      }else{
        stmt.BindValue(columnOffset+1, num);
      }
    }break;

    case LUA_TSTRING:{
      size_t size = 0;
      stmt.BindLuaString(columnOffset+1,  L, luaIndex);
    break;}

    case LUA_TBOOLEAN:
      FieldType = SQLLuaType::Boolean;
      stmt.BindValue(columnOffset+1, lua_toboolean(L, luaIndex));
    break;

    case LUA_TTABLE:{
      FieldType = SQLLuaType::Table;
      bool saveTable;
      int index = GetTableId(lua_topointer(L, luaIndex), saveTable);

      if(saveTable){
        saveTableindex = index;
      }

      stmt.BindValue(columnOffset+1, index);
    break;}

    default:
      return false;
   }

  if(FieldType == -1){
    //just insert a null since this is a type we can automatically deduce from the SQL type
    stmt.BindValue(columnOffset, nullptr);
  }else{
    stmt.BindValue(columnOffset, FieldType);
  }

  return true;
}

int SavedVariables::GetTableId(const void* table, bool& notCreatedYet){

  auto it = FoundTables.find(table);

  if(it == FoundTables.end()){
    int index = ++TableCount;
    FoundTables[table] = index;
    
    notCreatedYet = true;

    return index;
  }else{
    notCreatedYet = false;
    return it->second;
  }
}

void SavedVariables::SaveSQL_Table(lua_State *L, int tableIndex, SQLitePreparedStatment& InsertStatement){
  lua_pushnil(L);

  int InArray = 0;

  int KeyTableIndex = -1, ValueTableIndex = -1;
  bool SaveKeyTable = false;

  while(lua_next(L, -2) != 0){
    bool validValue = false;

    int FieldType = -1;

    switch(lua_type(L, -2)){
      case LUA_TNUMBER:{
        double num = lua_tonumber(L, -2);
        int value = (int)num;
      
        if(num == value){
          if(InArray != -1){
            if(InArray == value-1){
              InsertStatement.BindNullValue(3);
             break;
            }else{
              InArray = -1;
            }
          }

          InsertStatement.BindValue(3, value);
        }else{
          InsertStatement.BindValue(3, num);
        }
      }break;

      case LUA_TSTRING:
        InsertStatement.BindLuaString(3, L, -2);
      break;

      case LUA_TTABLE:
        FieldType = SQLLuaType::Table;
        KeyTableIndex= GetTableId(lua_topointer(L, -2), SaveKeyTable);
        InsertStatement.BindValue(5, KeyTableIndex);
      break;

      default:
        validValue = false;
      break;
    }

    validValue = SaveLuaValue<-1, 4>(L, InsertStatement, ValueTableIndex);

    if(validValue){
      InsertStatement.BindValues(tableIndex, nullptr);

      if(FieldType == -1){
        //just insert a null since this is a type we can automatically deduce from the SQL type
        InsertStatement.BindValue(2, nullptr);
      }else{
        InsertStatement.BindValue(2, FieldType);
      }

      InsertStatement.ExecuteAndReset();
    }

    if(SaveKeyTable){
      lua_pushvalue(L, -2);
        SaveSQL_Table(L, KeyTableIndex, InsertStatement);
      lua_pop(L, 1);
      SaveKeyTable = false;
    }

    if(ValueTableIndex != -1){
      SaveSQL_Table(L, ValueTableIndex, InsertStatement);
      ValueTableIndex = -1;
    }

    lua_pop(L, 1);
  }
}

int SavedVariables::SaveSQL(lua_State *L){

  if(db == NULL){      
    return 0;
  }

  TableCount = 0;
  
  bool TableExists = db->TableExists(Name.c_str());

  if(!TableExists){
    db->ExecuteSQLNoResultsFmt("create table if not exists [%s] (EntryID INTEGER PRIMARY KEY, TID integer, KT, Key, VT, Val)", Name.c_str());
  }

  SQLitePreparedStatment InsertStatement = db->CreateStatmentFmt("INSERT into [%s] values (NULL,?,?,?,?,?);", Name.c_str());

  SQLiteTransaction Transaction(*db);
   
  //if the table already exists clear out all the old values 
  //SQlite has optimization when there is no WHERE clause in a DELETE FROM it will just truncate the table instead of clearing out each row
  if(TableExists){
    db->ExecuteSQLNoResults(("delete from "+Name).c_str());
  }
  
  int ContainerIndex = LUA_GLOBALSINDEX;

  if(ContainerTable.is_valid()) {
    ContainerTable.push(L);
    ContainerIndex = lua_gettop(L);
  }

  int TableIndex;
  bool SaveTable = false;

  BOOST_FOREACH(const string& VariableName, VariableNames){
    lua_getfield(L, ContainerIndex, VariableName.c_str());

    int CurrentStackPos = lua_gettop(L);

    int type = lua_type(L, -1);

    if(type == LUA_TNIL){
      lua_pop(L, 1);
      continue;
    }

    bool invalid = false;

    if(SaveLuaValue<-1, 4>(L, InsertStatement, TableIndex)){
      //table index will always be 0 for root values and the field type and value will always be string
      InsertStatement.BindValues(0, nullptr, VariableName.c_str());

      InsertStatement.ExecuteAndReset();
    }

    if(TableIndex != -1){
      SaveSQL_Table(L, TableIndex, InsertStatement);
      TableIndex = -1;
    }

    _ASSERT_EXPR(lua_gettop(L) == CurrentStackPos, _T("Stack Position is not the position before the call to SaveTable"));

    lua_pop(L, 1);
  }


  Transaction.Commit();

  return 0;
}

int SavedVariables::Save(lua_State *L){

	CheckCreateSVDir();

  BOOST_FOREACH(SQLTableSerializer& entry, TableEntrys){
    entry.Save(L);
  }
  

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

  //SaveSQL(L);

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

struct VecAngle_UserData{
  int type;
  float x, y, z;
};

bool SavedVariables::SerializeCustomType(lua_State *L, int index){

	bool Result = false;
  int ObjectIndex = (lua_gettop(L)-1)-index;

  const char* fmtString = NULL;

	if(IsaWrapper(L, ObjectIndex, "Vector")){
    fmtString = "Vector(%g,%g,%g)";
  }else if(IsaWrapper(L, ObjectIndex, "Angles")){
    fmtString = "Angles(%g,%g,%g)";
  }
    
  if(fmtString != NULL){
    VecAngle_UserData* vector = (VecAngle_UserData*)lua_touserdata(L, ObjectIndex);

	  fprintf(SavedVariableFile, fmtString, vector->x, vector->y, vector->z);

		Result = true;
  	lua_pop(L, 3);
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

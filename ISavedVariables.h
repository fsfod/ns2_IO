#include "stdafx.h"

class luabind::object;

typedef luabind::table<luabind::object>  LuabindTable;

class ISavedVariables{

public:
  virtual ~ISavedVariables(){}

  virtual void Setup(lua_State* L, const std::string& fname, LuabindTable const& tableNames, LuabindTable const& containerTable) = 0;

  virtual void Save(lua_State *L) = 0;
  virtual void Load(lua_State *L) = 0;

  virtual void SetExitAutoSaving(bool AutoSave) = 0;

protected:
  std::string Name;
  bool ExitAutoSave;
};
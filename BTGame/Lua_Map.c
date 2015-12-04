#include "Lua.h"

#include "WorldView.h"
#include "Managers.h"
#include "Console.h"
#include "Map.h"
#include "GridManager.h"

#include "liblua/lauxlib.h"
#include "liblua/lualib.h"

static int slua_mapLoad(lua_State *L);
static int slua_mapSave(lua_State *L);
static int slua_mapSetSchemas(lua_State *L);

void luaLoadMapLibrary(lua_State *L) {
   lua_newtable(L);
   luaPushFunctionTable(L, "load", &slua_mapLoad);
   luaPushFunctionTable(L, "save", &slua_mapSave);
   luaPushFunctionTable(L, "setSchemas", &slua_mapSetSchemas);
   lua_setglobal(L, LLIB_MAP);
}

int slua_mapLoad(lua_State *L) {
   WorldView *view = luaGetWorldView(L);
   const char *path = luaL_checkstring(L, 1);
   Map *map = mapLoad(path);

   if (!map) {
      lua_pushliteral(L, "Failed to read map file.");
      lua_error(L);
   }

   gridManagerLoadMap(view->managers->gridManager, map);
   return 0;
}
int slua_mapSave(lua_State *L) {
   WorldView *view = luaGetWorldView(L);
   const char *path = luaL_checkstring(L, 1);
   Map *map = gridManagerGetMap(view->managers->gridManager);

   if (!map) {
      lua_pushliteral(L, "No map currently loaded?");
      lua_error(L);
   }

   if (mapSave(map, path)) {
      lua_pushliteral(L, "Failed to save map file.");
      lua_error(L);
   }
   
   return 0;
}
int slua_mapSetSchemas(lua_State *L) {
   WorldView *view = luaGetWorldView(L);
   int len, i;
   luaL_checktype(L, 1, LUA_TTABLE);
   
   lua_len(L, 1);
   len = lua_tointeger(L, -1);
   lua_pop(L, 1);

   gridManagerClearSchemas(view->managers->gridManager);

   for (i = 0; i < len; ++i) {
      TileSchema *schema = gridManagerGetSchema(view->managers->gridManager, i);

      int j;

      lua_pushinteger(L, i + 1);
      lua_gettable(L, -2);//push the schema table

      lua_pushliteral(L, "imgCount");
      lua_gettable(L, -2);
      schema->imgCount = MIN(3, lua_tointeger(L, -1));
      lua_pop(L, 1);

      lua_pushliteral(L, "occlusion");
      lua_gettable(L, -2);
      schema->occlusion = lua_tointeger(L, -1);
      lua_pop(L, 1);

      lua_pushliteral(L, "img");
      lua_gettable(L, -2);
      luaL_checktype(L, -1, LUA_TTABLE);

      for (j = 0; j < schema->imgCount; ++j) {
         lua_pushinteger(L, j + 1);
         lua_gettable(L, -2);
         schema->img[j] = lua_tointeger(L, -1);
         lua_pop(L, 1);
      }

      lua_pop(L, 2);//pop the img table and the schema
   }

   return 0;
}

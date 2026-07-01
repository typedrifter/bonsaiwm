#include "lua.h"
#include "config.h"
#include <libinput.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/wait.h>
#include <unistd.h>
#include <wlr/types/wlr_keyboard.h>
#include <xkbcommon/xkbcommon.h>

lua_State *L = NULL;

lua_State *lua_init(void) {
  L = luaL_newstate();
  if (!L)
    return NULL;

  luaL_openlibs(L);
  lua_newtable(L);
  lua_setglobal(L, "bonsaiwm");

  return L;
}

void lua_load_config(const char *path) {

  if (luaL_dofile(L, path) != LUA_OK) {
    fprintf(stderr, "bonsaiwm: lua error in %s: %s\n", path,
            lua_tostring(L, -1));
    lua_pop(L, 1);

    return;
  }
}

void get_config_int(lua_State *L, const char *group, const char *field,
                    int *target) {
  lua_getglobal(L, group);
  lua_getfield(L, -1, field);
  if (lua_isnumber(L, -1)) {
    int val = (int)lua_tonumber(L, -1);
    fprintf(stderr, "%s.%s = %i\n", group, field, val);
    *target = val;
  }
  lua_pop(L, 2);
}

void get_config_uint(lua_State *L, const char *group, const char *field,
                     unsigned int *target) {
  lua_getglobal(L, group);
  lua_getfield(L, -1, field);
  if (lua_isnumber(L, -1)) {
    int val = (int)lua_tonumber(L, -1);
    fprintf(stderr, "%s.%s = %i\n", group, field, val);
    *target = val;
  }
  lua_pop(L, 2);
}

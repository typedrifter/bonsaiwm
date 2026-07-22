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

/* name -> enum value pairs for the bonsaiwm.action sub-table. Must stay in
 * sync with the enum in config.h. */
static const struct {
  const char *name;
  int value;
} action_names[] = {
    {"none", ActNone},
    {"chvt", ActChvt},
    {"defaultgaps", ActDefaultGaps},
    {"focusmon", ActFocusMon},
    {"focusstack", ActFocusStack},
    {"incgaps", ActIncGaps},
    {"incnmaster", ActIncNmaster},
    {"killclient", ActKillClient},
    {"load_config", ActLoadConfig},
    {"quit", ActQuit},
    {"setlayout", ActSetLayout},
    {"setmfact", ActSetMfact},
    {"spawn", ActSpawn},
    {"tag", ActTag},
    {"tagmon", ActTagMon},
    {"togglefloating", ActToggleFloating},
    {"togglefullscreen", ActToggleFullscreen},
    {"togglegaps", ActToggleGaps},
    {"toggletag", ActToggleTag},
    {"toggleview", ActToggleView},
    {"view", ActView},
    {"zoom", ActZoom},
};

lua_State *lua_init(void) {
  L = luaL_newstate();
  if (!L)
    return NULL;

  luaL_openlibs(L);
  lua_newtable(L);

  /* populate bonsaiwm.action so config.lua can refer to actions by name
   * instead of raw enum integer */
  lua_newtable(L);
  for (size_t i = 0; i < sizeof(action_names) / sizeof(action_names[0]); i++) {
    lua_pushinteger(L, action_names[i].value);
    lua_setfield(L, -2, action_names[i].name);
  }
  lua_setfield(L, -2, "action");

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

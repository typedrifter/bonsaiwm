#include "lua.h"
#include "config.h"
#include <libinput.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/wait.h>
#include <unistd.h>
#include <wlr/types/wlr_keyboard.h>
#include <wlr/util/log.h>
#include <xkbcommon/xkbcommon.h>

lua_State *L = NULL;

/* trampoline for lua-defined keymap actions. Dispatches the registry ref in
 * arg->i. No reentrancy guard needed: callbacks run synchronously inside
 * keybinding() on the single wayland thread, and the lua VM exposes no C
 * bindings, so a callback cannot reenter keybinding() or load_config(). The
 * NULL L guard covers the brief teardown window in load_config() where the
 * old VM has been closed and the new one is not yet built; today no event
 * source can fire there, but the guard is cheap insurance against a future
 * yield inside reload. */
void lua_action(const Arg *arg) {
  if (!L)
    return;
  lua_rawgeti(L, LUA_REGISTRYINDEX, arg->i);
  if (lua_pcall(L, 0, 0, 0) != LUA_OK) {
    wlr_log(WLR_ERROR, "lua_action callback error: %s", lua_tostring(L, -1));
    lua_pop(L, 1);
  }
}

/* name -> enum value pairs for the bonsaiwm.action sub-table. Generated from
 * actions.def alongside the enum and dispatch tables, so all four sites
 * share one source of truth and cannot drift. */
static const struct {
  const char *name;
  int value;
} action_names[] = {
#define X(ACT, NAME, FUNC, ARGT) {NAME, ACT},
#include "actions.def"
#undef X
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
    wlr_log(WLR_ERROR, "lua error in %s: %s", path, lua_tostring(L, -1));
    lua_pop(L, 1);

    return;
  }
}

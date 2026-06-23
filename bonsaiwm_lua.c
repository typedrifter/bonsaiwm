#include "bonsaiwm_lua.h"
#include "bonsaiwm.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

lua_State *L = NULL;

/* Whether the config has been loaded at least once, so exec_once
 * knows to skip on subsequent reloads. */
static int config_loaded = 0;

/* Lua callbacks receive their Lua state as the first argument by convention.
 * These are registered with luaL_setfuncs and called by the Lua VM, which
 * always passes L. Using a parameter named L would shadow the global, so
 * we use luaL (lua state, local) instead. */

/* Set outer/inner gaps on selmon. Mirrors the setgaps() C keybinding. */
static int bonsaiwm_set_gaps(lua_State *luaL) {
  int oh = luaL_checkinteger(luaL, 1);
  int ov = luaL_checkinteger(luaL, 2);
  int ih = luaL_checkinteger(luaL, 3);
  int iv = luaL_checkinteger(luaL, 4);
  setgaps(oh, ov, ih, iv);
  return 0;
}

/* Log to stderr. */
static int bonsaiwm_log(lua_State *luaL) {
  const char *msg = luaL_checkstring(luaL, 1);
  fprintf(stderr, "[bonsaiwm] %s\n", msg);
  return 0;
}

/* Run a shell command. Fires on every config reload. */
static int bonsaiwm_exec(lua_State *luaL) {
  const char *cmd = luaL_checkstring(luaL, 1);
  system(cmd);
  return 0;
}

/* Run a shell command, but only on the first load. Skipped on reloads
 * so we don't respawn waybar/bgs etc. */
static int bonsaiwm_exec_once(lua_State *luaL) {
  const char *cmd = luaL_checkstring(luaL, 1);
  if (!config_loaded)
    system(cmd);
  return 0;
}

/* Register a Lua callback for event. Replaces any existing hook.
 * Events are fired from C via bonsaiwm_lua_hook(). */
static int bonsaiwm_on(lua_State *luaL) {
  const char *event = luaL_checkstring(luaL, 1);
  luaL_checktype(luaL, 2, LUA_TFUNCTION);

  /* Push the hooks table from the registry */
  lua_getfield(luaL, LUA_REGISTRYINDEX, "_bonsaiwm_hooks");

  /* Copy the function and store it: hooks_table[event] = fn */
  lua_pushvalue(luaL, 2);
  lua_setfield(luaL, -2, event);

  /* Clean up: pop the hooks table */
  lua_pop(luaL, 1);

  return 0;
}

void bonsaiwm_lua_hook(const char *event, int nargs) {
  /* Look up hooks_table[event] from the registry */
  lua_getfield(L, LUA_REGISTRYINDEX, "_bonsaiwm_hooks");

  lua_getfield(L, -1, event);

  if (lua_isnil(L, -1)) {
    /* No hook registered for this event, clean up and return */
    lua_pop(L, 2 + nargs);
    return;
  }

  /* We need the function below the arguments for lua_pcall.
   * Stack is: [...args..., hooks_table, fn]
   * We want: [fn, ...args...] */
  lua_insert(L, -(nargs + 2));

  /* Remove the hooks_table from the stack */
  lua_pop(L, 1);

  if (lua_pcall(L, nargs, 0, 0) != LUA_OK) {
    fprintf(stderr, "bonsaiwm: hook '%s' error: %s\n", event,
            lua_tostring(L, -1));
    lua_pop(L, 1);
  }
}

lua_State *bonsaiwm_lua_init(void) {
  L = luaL_newstate();
  if (!L)
    return NULL;

  luaL_openlibs(L);

  /* Create the hooks table in the registry. This is a single
   * table keyed by event name that holds all registered hooks,
   * replacing the old per-event static variables. */
  lua_newtable(L);
  lua_setfield(L, LUA_REGISTRYINDEX, "_bonsaiwm_hooks");

  lua_newtable(L);
  luaL_setfuncs(L,
                 (const luaL_Reg[]){{"set_gaps", bonsaiwm_set_gaps},
                                    {"log", bonsaiwm_log},
                                    {"exec", bonsaiwm_exec},
                                    {"exec_once", bonsaiwm_exec_once},
                                    {"on", bonsaiwm_on},
                                    {NULL, NULL}},
                 0);
  lua_setglobal(L, "bonsaiwm");

  return L;
}

void bonsaiwm_lua_load_config(const char *path) {
  int old_hooks_ref;

  /* Save the current hooks table before loading the new config.
   * We store it under an auto-generated integer key so the new
   * config doesn't clobber it. */
  lua_getfield(L, LUA_REGISTRYINDEX, "_bonsaiwm_hooks");
  old_hooks_ref = luaL_ref(L, LUA_REGISTRYINDEX);

  /* Create a fresh empty hooks table so the new config writes
   * into it, not the old one. This is crucial for atomicity:
   * if the config fails partway, we can discard the partial
   * table and restore the old one untouched. */
  lua_newtable(L);
  lua_setfield(L, LUA_REGISTRYINDEX, "_bonsaiwm_hooks");

  if (luaL_dofile(L, path) != LUA_OK) {
    fprintf(stderr, "bonsaiwm: lua error in %s: %s\n", path,
            lua_tostring(L, -1));
    lua_pop(L, 1);

    /* Config failed — restore the old hooks so we don't end up
     * with missing or partial hook state. */
    lua_rawgeti(L, LUA_REGISTRYINDEX, old_hooks_ref);
    lua_setfield(L, LUA_REGISTRYINDEX, "_bonsaiwm_hooks");
    luaL_unref(L, LUA_REGISTRYINDEX, old_hooks_ref);
  } else {
    /* Config loaded successfully — the old hooks table is no
     * longer needed, let Lua garbage-collect it. */
    luaL_unref(L, LUA_REGISTRYINDEX, old_hooks_ref);
  }

  config_loaded = 1;
}

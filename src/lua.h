#ifndef LUA_H
#define LUA_H

#include "config.h"
#include <lauxlib.h>
#include <lua.h>
#include <lualib.h>

/* extern, not static: static in a header creates a separate variable per .c
 * file */
extern lua_State *L;

/* init the Lua VM. Returns NULL on OOM */
lua_State *lua_init(void);

/* load and run path as the config file */
void lua_load_config(const char *path);

/* trampoline for lua-defined keymap actions. Key.func points here when the
 * config entry's `action` field is a lua function; arg->i holds the
 * LUA_REGISTRYINDEX ref. Pure-lua callbacks only — no compositor API. */
void lua_action(const Arg *arg);

#endif /* LUA_H */

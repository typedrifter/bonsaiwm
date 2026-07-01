#ifndef LUA_H
#define LUA_H

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

#endif /* LUA_H */

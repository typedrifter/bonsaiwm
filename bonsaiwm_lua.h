#ifndef BONSAIWM_LUA_H
#define BONSAIWM_LUA_H

#include <lauxlib.h>
#include <lua.h>
#include <lualib.h>

/* extern, not static: static in a header creates a separate variable per .c
 * file */
extern lua_State *L;

/* Init the Lua VM. Returns NULL on OOM. */
lua_State *bonsaiwm_lua_init(void);

/* Load and run path as the config file.
 * Keeps old hooks if the file fails to load. */
void bonsaiwm_lua_load_config(const char *path);

/* Fire hook event. Push args onto the Lua stack before calling. */
void bonsaiwm_lua_hook(const char *event, int nargs);

#endif /* BONSAIWM_LUA_H */

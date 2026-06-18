#ifndef BONSAIWM_LUA_H
#define BONSAIWM_LUA_H

#include <lauxlib.h>
#include <lua.h>
#include <lualib.h>

/* extern, not static: static in a header creates a separate variable per .c
 * file */
extern lua_State *L;

lua_State *bonsaiwm_lua_init(void);
void bonsaiwm_lua_load_config(const char *path);
void bonsaiwm_lua_hook(const char *event, int nargs);

#endif /* BONSAIWM_LUA_H */
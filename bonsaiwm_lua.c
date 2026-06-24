#include "bonsaiwm_lua.h"
#include "bonsaiwm.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/wait.h>
#include <unistd.h>
#include <wlr/types/wlr_keyboard.h>
#include <xkbcommon/xkbcommon.h>

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

/* Adjust all gaps by delta pixels. Mirrors the adjustgaps() C function. */
static int bonsaiwm_adjust_gaps(lua_State *luaL) {
  int delta = luaL_checkinteger(luaL, 1);
  adjustgaps(delta);
  return 0;
}

/* Reset gaps to config defaults. Mirrors the resetgaps() C function. */
static int bonsaiwm_default_gaps(lua_State *luaL) {
  resetgaps();
  return 0;
}

/* Set border width on all clients. Mirrors the setborderwidth() C keybinding. */
static int bonsaiwm_set_border_width(lua_State *luaL) {
  unsigned int px = luaL_checkinteger(luaL, 1);
  setborderwidth(px);
  return 0;
}

/* Set border color on all clients. Mirrors the setbordercolor() C keybinding. */
static int bonsaiwm_set_border_color(lua_State *luaL) {
  float a = luaL_checknumber(L, 4);
  float r = luaL_checknumber(L, 1) / 255.0f * a;
  float g = luaL_checknumber(L, 2) / 255.0f * a;
  float b = luaL_checknumber(L, 3) / 255.0f * a;
  setbordercolor(r, g, b, a);
  return 0;
}

/* Set focused border color. Mirrors the setfocuscolor() C function. */
static int bonsaiwm_set_focus_color(lua_State *luaL) {
  float a = luaL_checknumber(L, 4);
  float r = luaL_checknumber(L, 1) / 255.0f * a;
  float g = luaL_checknumber(L, 2) / 255.0f * a;
  float b = luaL_checknumber(L, 3) / 255.0f * a;
  setfocuscolor(r, g, b, a);
  return 0;
}

/* Set urgent border color. Mirrors the seturgentcolor() C function. */
static int bonsaiwm_set_urgent_color(lua_State *luaL) {
  float a = luaL_checknumber(L, 4);
  float r = luaL_checknumber(L, 1) / 255.0f * a;
  float g = luaL_checknumber(L, 2) / 255.0f * a;
  float b = luaL_checknumber(L, 3) / 255.0f * a;
  seturgentcolor(r, g, b, a);
  return 0;
}

/* Set desktop background color. Mirrors the setrootcolor() C function. */
static int bonsaiwm_set_root_color(lua_State *luaL) {
  float a = luaL_checknumber(L, 4);
  float r = luaL_checknumber(L, 1) / 255.0f * a;
  float g = luaL_checknumber(L, 2) / 255.0f * a;
  float b = luaL_checknumber(L, 3) / 255.0f * a;
  setrootcolor(r, g, b, a);
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

/* Spawn a long-lived process via /bin/sh -c. Forks, detaches the session,
 * and routes child stdout to stderr so it can't block the event loop.
 * Mirrors the C spawn() action's fork+setsid pattern. */
static int bonsaiwm_spawn(lua_State *luaL) {
  const char *cmd = luaL_checkstring(luaL, 1);

  if (fork() == 0) {
    dup2(STDERR_FILENO, STDOUT_FILENO);
    setsid();
    execl("/bin/sh", "/bin/sh", "-c", cmd, (char *)NULL);
    /* If execl returned, it failed. fprintf works because we're the child. */
    fprintf(stderr, "bonsaiwm: spawn failed: %s\n", cmd);
    _exit(EXIT_FAILURE); /* don't flush the compositor's stdio */
  }
  return 0;
}

/* Register a keybinding. Calls fn (no args) when (mod, keyname)
 * is pressed. keyname is resolved via xkb_keysym_from_name at bind time.
 * The (mod, keysym) pair is packed into a 64-bit integer key and
 * stored in the _bonsaiwm_keybinds registry table. */
static int bonsaiwm_bind_key(lua_State *luaL) {
  uint32_t mod = luaL_checkinteger(luaL, 1);
  const char *name = luaL_checkstring(luaL, 2);
  luaL_checktype(luaL, 3, LUA_TFUNCTION);

  xkb_keysym_t sym = xkb_keysym_from_name(name, XKB_KEYSYM_NO_FLAGS);
  if (sym == XKB_KEY_NoSymbol)
    return luaL_error(luaL, "unknown key name: %s", name);

  mod = CLEANMASK(mod);
  sym = xkb_keysym_to_lower(sym);
  lua_Integer key = ((lua_Integer)mod << 32) | (lua_Integer)sym;

  /* Same pattern as bonsaiwm_on: get table, store fn under key, pop table */
  lua_getfield(luaL, LUA_REGISTRYINDEX, "_bonsaiwm_keybinds");
  lua_pushvalue(luaL, 3);     /* copy the fn */
  lua_rawseti(luaL, -2, key); /* t[key] = fn (raw, skips metamethods) */
  lua_pop(luaL, 1);
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

/* Look up the keybind registry under the packed (mod, keysym) key.
 * If a callback is registered, pcall it with 0 args. Returns 1 if
 * a callback ran (and the keypress is considered handled), 0 otherwise.
 * Mirrors the dispatch pattern of bonsaiwm_lua_hook. */
int bonsaiwm_lua_keybind(uint32_t mod, xkb_keysym_t sym) {
  /* L can be NULL if bonsaiwm_lua_init failed to allocate a state; treat as
   * unhandled rather than crashing the compositor on an unbound keypress. */
  if (!L)
    return 0;

  lua_Integer key = ((lua_Integer)mod << 32) | (lua_Integer)sym;

  lua_getfield(L, LUA_REGISTRYINDEX, "_bonsaiwm_keybinds");
  lua_rawgeti(L, -1, key); /* t[key] — pushes nil if absent */

  if (lua_isnil(L, -1)) {
    lua_pop(L, 2); /* pop nil + table */
    return 0;
  }

  lua_remove(L, -2); /* drop the table, leave fn on top */

  if (lua_pcall(L, 0, 0, 0) != LUA_OK) {
    fprintf(stderr, "bonsaiwm: keybind error: %s\n", lua_tostring(L, -1));
    lua_pop(L, 1);
    return 0; /* error counts as unhandled so the key isn't swallowed */
  }
  return 1;
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
  lua_setfield(L, LUA_REGISTRYINDEX, "_bonsaiwm_keybinds");

  lua_newtable(L);
  luaL_setfuncs(
      L,
      (const luaL_Reg[]){{"set_gaps", bonsaiwm_set_gaps},
                         {"adjust_gaps", bonsaiwm_adjust_gaps},
                         {"default_gaps", bonsaiwm_default_gaps},
                         {"set_border_width", bonsaiwm_set_border_width},
                         {"set_border_color", bonsaiwm_set_border_color},
                         {"set_focus_color", bonsaiwm_set_focus_color},
                         {"set_urgent_color", bonsaiwm_set_urgent_color},
                         {"set_root_color", bonsaiwm_set_root_color},
                         {"bind_key", bonsaiwm_bind_key},
                         {"log", bonsaiwm_log},
                         {"exec", bonsaiwm_exec},
                         {"exec_once", bonsaiwm_exec_once},
                         {"spawn", bonsaiwm_spawn},
                         {"on", bonsaiwm_on},
                         {NULL, NULL}},
      0);

  /* Expose the mod constants as bonsaiwm.mod.{shift,caps,ctrl,alt,mod2,logo} */
  lua_newtable(L);
  lua_pushinteger(L, WLR_MODIFIER_SHIFT);
  lua_setfield(L, -2, "shift");
  lua_pushinteger(L, WLR_MODIFIER_CAPS);
  lua_setfield(L, -2, "caps");
  lua_pushinteger(L, WLR_MODIFIER_CTRL);
  lua_setfield(L, -2, "ctrl");
  lua_pushinteger(L, WLR_MODIFIER_ALT);
  lua_setfield(L, -2, "alt");
  lua_pushinteger(L, WLR_MODIFIER_MOD2);
  lua_setfield(L, -2, "mod2");
  lua_pushinteger(L, WLR_MODIFIER_LOGO);
  lua_setfield(L, -2, "logo");
  lua_setfield(L, -2, "mod");

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

  /* Clear the keybind registry too, so a reloaded config starts fresh.
   * Same atomic-replace rationale as for hooks. */
  lua_newtable(L);
  lua_setfield(L, LUA_REGISTRYINDEX, "_bonsaiwm_keybinds");

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

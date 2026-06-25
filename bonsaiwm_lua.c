#include "bonsaiwm_lua.h"
#include "bonsaiwm.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/wait.h>
#include <unistd.h>
#include <libinput.h>
#include <wlr/types/wlr_keyboard.h>
#include <xkbcommon/xkbcommon.h>

lua_State *L = NULL;

/* Whether the config has been loaded at least once, so exec_once
 * knows to skip on subsequent reloads. */
static int config_loaded = 0;

/* Runtime-state-dependent setters are loaded early now, before outputs
 * exist. Calling them at the top level of config.lua is almost always a
 * mistake (the user wants bonsaiwm.config.* instead). This macro raises a
 * Lua error that includes the offending file:line and triggers the normal
 * config-load rollback. */
#define REQUIRE_BACKEND_OR_RAISE(luaL, name)                                   \
  do {                                                                         \
    if (!selmon) {                                                             \
      luaL_where((luaL), 1);                                                   \
      lua_pushfstring((luaL), "%s " name                                       \
                             " called before backend_start; "                  \
                             "use bonsaiwm.config.* fields instead",          \
                      lua_tostring((luaL), -1));                               \
      lua_concat((luaL), 2);                                                   \
      return luaL_error((luaL), "%s", lua_tostring((luaL), -1));               \
    }                                                                          \
  } while (0)

/* Lua callbacks receive their Lua state as the first argument by convention.
 * These are registered with luaL_setfuncs and called by the Lua VM, which
 * always passes L. Using a parameter named L would shadow the global, so
 * we use luaL (lua state, local) instead. */

/* Set outer/inner gaps on selmon. Mirrors the setgaps() C keybinding. */
static int bonsaiwm_set_gaps(lua_State *luaL) {
  int oh, ov, ih, iv;
  REQUIRE_BACKEND_OR_RAISE(luaL, "set_gaps");
  oh = luaL_checkinteger(luaL, 1);
  ov = luaL_checkinteger(luaL, 2);
  ih = luaL_checkinteger(luaL, 3);
  iv = luaL_checkinteger(luaL, 4);
  setgaps(oh, ov, ih, iv);
  return 0;
}

/* Adjust all gaps by delta pixels. Mirrors the adjustgaps() C function. */
static int bonsaiwm_adjust_gaps(lua_State *luaL) {
  int delta;
  REQUIRE_BACKEND_OR_RAISE(luaL, "adjust_gaps");
  delta = luaL_checkinteger(luaL, 1);
  adjustgaps(delta);
  return 0;
}

/* Reset gaps to config defaults. Mirrors the resetgaps() C function. */
static int bonsaiwm_default_gaps(lua_State *luaL) {
  REQUIRE_BACKEND_OR_RAISE(luaL, "default_gaps");
  resetgaps();
  return 0;
}

/* Set mfact absolutely (0.1 to 0.9). Mirrors the setmfact_val() C function. */
static int bonsaiwm_set_mfact(lua_State *luaL) {
  float f;
  REQUIRE_BACKEND_OR_RAISE(luaL, "set_mfact");
  f = luaL_checknumber(luaL, 1);
  if (setmfact_val(f) == -1) {
    fprintf(stderr, "bonsaiwm: set_mfact(%g) out of range (0.1-0.9)\n", f);
  }
  return 0;
}

/* Adjust mfact by delta. Mirrors the adjustmfact() C function. */
static int bonsaiwm_adjust_mfact(lua_State *luaL) {
  float delta;
  REQUIRE_BACKEND_OR_RAISE(luaL, "adjust_mfact");
  delta = luaL_checknumber(luaL, 1);
  if (adjustmfact(delta) == -1) {
    fprintf(stderr, "bonsaiwm: adjust_mfact(%g) would exceed 0.1-0.9\n",
            delta);
  }
  return 0;
}

/* Set nmaster absolutely. Mirrors the setnmaster() C function. */
static int bonsaiwm_set_nmaster(lua_State *luaL) {
  int n;
  REQUIRE_BACKEND_OR_RAISE(luaL, "set_nmaster");
  n = luaL_checkinteger(luaL, 1);
  setnmaster(n);
  return 0;
}

/* Adjust nmaster by delta. Mirrors the adjustnmaster() C function. */
static int bonsaiwm_adjust_nmaster(lua_State *luaL) {
  int delta;
  REQUIRE_BACKEND_OR_RAISE(luaL, "adjust_nmaster");
  delta = luaL_checkinteger(luaL, 1);
  adjustnmaster(delta);
  return 0;
}

/* Toggle focus-follows-mouse. Mirrors the setsloppyfocus() C function. */
static int bonsaiwm_set_sloppy_focus(lua_State *luaL) {
  int v = luaL_checkinteger(luaL, 1);
  setsloppyfocus(v);
  return 0;
}

/* Toggle smart gaps. Mirrors the setsmartgaps() C function. */
static int bonsaiwm_set_smart_gaps(lua_State *luaL) {
  int v;
  REQUIRE_BACKEND_OR_RAISE(luaL, "set_smart_gaps");
  v = luaL_checkinteger(luaL, 1);
  setsmartgaps(v);
  return 0;
}

/* Set border width on all clients. Mirrors the setborderwidth() C keybinding. */
static int bonsaiwm_set_border_width(lua_State *luaL) {
  unsigned int px;
  REQUIRE_BACKEND_OR_RAISE(luaL, "set_border_width");
  px = luaL_checkinteger(luaL, 1);
  setborderwidth(px);
  return 0;
}

/* Set border color on all clients. Mirrors the setbordercolor() C keybinding. */
static int bonsaiwm_set_border_color(lua_State *luaL) {
  float a, r, g, b;
  REQUIRE_BACKEND_OR_RAISE(luaL, "set_border_color");
  a = luaL_checknumber(luaL, 4);
  r = luaL_checknumber(luaL, 1) / 255.0f * a;
  g = luaL_checknumber(luaL, 2) / 255.0f * a;
  b = luaL_checknumber(luaL, 3) / 255.0f * a;
  setbordercolor(r, g, b, a);
  return 0;
}

/* Set focused border color. Mirrors the setfocuscolor() C function. */
static int bonsaiwm_set_focus_color(lua_State *luaL) {
  float a, r, g, b;
  REQUIRE_BACKEND_OR_RAISE(luaL, "set_focus_color");
  a = luaL_checknumber(luaL, 4);
  r = luaL_checknumber(luaL, 1) / 255.0f * a;
  g = luaL_checknumber(luaL, 2) / 255.0f * a;
  b = luaL_checknumber(luaL, 3) / 255.0f * a;
  setfocuscolor(r, g, b, a);
  return 0;
}

/* Set urgent border color. Mirrors the seturgentcolor() C function. */
static int bonsaiwm_set_urgent_color(lua_State *luaL) {
  float a, r, g, b;
  REQUIRE_BACKEND_OR_RAISE(luaL, "set_urgent_color");
  a = luaL_checknumber(luaL, 4);
  r = luaL_checknumber(luaL, 1) / 255.0f * a;
  g = luaL_checknumber(luaL, 2) / 255.0f * a;
  b = luaL_checknumber(luaL, 3) / 255.0f * a;
  seturgentcolor(r, g, b, a);
  return 0;
}

/* Set desktop background color. Mirrors the setrootcolor() C function. */
static int bonsaiwm_set_root_color(lua_State *luaL) {
  float a = luaL_checknumber(luaL, 4);
  float r = luaL_checknumber(luaL, 1) / 255.0f * a;
  float g = luaL_checknumber(luaL, 2) / 255.0f * a;
  float b = luaL_checknumber(luaL, 3) / 255.0f * a;
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

/* Request a config reload from Lua. Mirrors the C reloadconfig keybinding. */
static int bonsaiwm_reload(lua_State *luaL) {
  (void)luaL;
  bonsaiwm_request_reload();
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
                         {"set_mfact", bonsaiwm_set_mfact},
                         {"adjust_mfact", bonsaiwm_adjust_mfact},
                         {"set_nmaster", bonsaiwm_set_nmaster},
                         {"adjust_nmaster", bonsaiwm_adjust_nmaster},
                         {"set_sloppy_focus", bonsaiwm_set_sloppy_focus},
                         {"set_smart_gaps", bonsaiwm_set_smart_gaps},
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
                          {"reload", bonsaiwm_reload},
                          {NULL, NULL}},
      0);

  /* Expose constants under bonsaiwm.const.* so the API namespace stays
   * clean as more enum/flag vocabularies are added. */
  lua_newtable(L);

  /* bonsaiwm.const.mod.{shift,caps,ctrl,alt,mod2,logo} */
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

  /* bonsaiwm.const.accel_profile.{adaptive,flat} */
  lua_newtable(L);
  lua_pushinteger(L, LIBINPUT_CONFIG_ACCEL_PROFILE_ADAPTIVE);
  lua_setfield(L, -2, "adaptive");
  lua_pushinteger(L, LIBINPUT_CONFIG_ACCEL_PROFILE_FLAT);
  lua_setfield(L, -2, "flat");
  lua_setfield(L, -2, "accel_profile");

  lua_setfield(L, -2, "const");

  lua_setglobal(L, "bonsaiwm");

  return L;
}

/* Read helpers for bonsaiwm.config.* fields. Each helper:
 *  - pushes bonsaiwm.config.<group>.<field>
 *  - if the value is present and has a compatible type, writes it to *target
 *  - if the value is nil/missing, leaves *target unchanged (incremental override)
 *  - cleans up the Lua stack
 */
static void get_int(lua_State *L, const char *group, const char *field,
                    int *target) {
  lua_getglobal(L, "bonsaiwm");
  lua_getfield(L, -1, "config");
  lua_getfield(L, -1, group);
  lua_getfield(L, -1, field);
  if (lua_isnumber(L, -1))
    *target = (int)lua_tointeger(L, -1);
  lua_pop(L, 4);
}

static void get_uint(lua_State *L, const char *group, const char *field,
                     unsigned int *target) {
  lua_getglobal(L, "bonsaiwm");
  lua_getfield(L, -1, "config");
  lua_getfield(L, -1, group);
  lua_getfield(L, -1, field);
  if (lua_isnumber(L, -1))
    *target = (unsigned int)lua_tointeger(L, -1);
  lua_pop(L, 4);
}

static void get_bool(lua_State *L, const char *group, const char *field,
                     int *target) {
  lua_getglobal(L, "bonsaiwm");
  lua_getfield(L, -1, "config");
  lua_getfield(L, -1, group);
  lua_getfield(L, -1, field);
  if (lua_isboolean(L, -1))
    *target = lua_toboolean(L, -1);
  lua_pop(L, 4);
}

static void get_double(lua_State *L, const char *group, const char *field,
                       double *target) {
  lua_getglobal(L, "bonsaiwm");
  lua_getfield(L, -1, "config");
  lua_getfield(L, -1, group);
  lua_getfield(L, -1, field);
  if (lua_isnumber(L, -1))
    *target = lua_tonumber(L, -1);
  lua_pop(L, 4);
}

/* Positional RGBA as used in bonsaiwm.config.appearance.colors.*.
 * Components are r,g,b in 0-255 and a in 0-1. The stored values are
 * premultiplied by alpha, matching the existing set_border_color-style
 * setter behavior. */
static void get_rgba(lua_State *L, const char *group, const char *table,
                     const char *field, float target[4]) {
  int i, ok;
  float buf[4] = {0}, a;

  lua_getglobal(L, "bonsaiwm");
  lua_getfield(L, -1, "config");
  lua_getfield(L, -1, group);
  lua_getfield(L, -1, table);
  lua_getfield(L, -1, field);

  if (lua_istable(L, -1)) {
    ok = 1;
    for (i = 0; i < 4 && ok; i++) {
      lua_rawgeti(L, -1, i + 1);
      if (lua_isnumber(L, -1))
        buf[i] = (float)lua_tonumber(L, -1);
      else
        ok = 0;
      lua_pop(L, 1);
    }
    if (ok) {
      a = buf[3];
      target[0] = buf[0] / 255.0f * a;
      target[1] = buf[1] / 255.0f * a;
      target[2] = buf[2] / 255.0f * a;
      target[3] = a;
    }
  }

  lua_pop(L, 5);
}

/* Assign one xkb_rule_names field, freeing a previous dynamically allocated
 * value if any. target->* are const char *, but we own the memory we set. */
static void set_xkb_field(struct xkb_rule_names *target, int idx,
                          const char *value) {
  switch (idx) {
  case 0:
    free((void *)target->rules);
    target->rules = value;
    break;
  case 1:
    free((void *)target->model);
    target->model = value;
    break;
  case 2:
    free((void *)target->layout);
    target->layout = value;
    break;
  case 3:
    free((void *)target->variant);
    target->variant = value;
    break;
  case 4:
    free((void *)target->options);
    target->options = value;
    break;
  }
}

static void get_xkb_rules(lua_State *L, struct xkb_rule_names *target) {
  static const char *fields[] = {"rules", "model", "layout", "variant",
                                 "options"};
  int i;
  const char *s;

  lua_getglobal(L, "bonsaiwm");
  lua_getfield(L, -1, "config");
  lua_getfield(L, -1, "keyboard");
  lua_getfield(L, -1, "xkb_rules");

  if (lua_istable(L, -1)) {
    for (i = 0; i < 5; i++) {
      lua_getfield(L, -1, fields[i]);
      if (lua_isstring(L, -1) && lua_rawlen(L, -1) > 0) {
        s = lua_tostring(L, -1);
        set_xkb_field(target, i, strdup(s));
      } else if (lua_isnil(L, -1)) {
        set_xkb_field(target, i, NULL);
      }
      lua_pop(L, 1);
    }
  }

  lua_pop(L, 4);
}

/* Sink bonsaiwm.config.* fields into the C globals read by createmon,
 * createkeyboardgroup, createpointer, and setup. Fields that are absent
 * leave the current C value intact, so config.h remains the boot-safe
 * fallback for anything not overridden in config.lua. */
static void apply_config(lua_State *L) {
  int tmp = accel_profile;

  get_uint(L, "appearance", "border_width", &borderpx);
  get_bool(L, "appearance", "sloppy_focus", &sloppyfocus);
  get_rgba(L, "appearance", "colors", "root", rootcolor);
  get_rgba(L, "appearance", "colors", "border", bordercolor);
  get_rgba(L, "appearance", "colors", "focus", focuscolor);
  get_rgba(L, "appearance", "colors", "urgent", urgentcolor);
  get_rgba(L, "appearance", "colors", "fullscreen", fullscreen_bg);

  get_bool(L, "gaps", "enabled", &enablegaps);
  get_bool(L, "gaps", "smart", &smartgaps);
  get_uint(L, "gaps", "outer_h", &gappoh);
  get_uint(L, "gaps", "outer_v", &gappov);
  get_uint(L, "gaps", "inner_h", &gappih);
  get_uint(L, "gaps", "inner_v", &gappiv);

  get_int(L, "keyboard", "repeat_rate", &repeat_rate);
  get_int(L, "keyboard", "repeat_delay", &repeat_delay);
  get_xkb_rules(L, &xkb_rules);

  get_bool(L, "input", "tap_to_click", &tap_to_click);
  get_bool(L, "input", "natural_scrolling", &natural_scrolling);
  get_bool(L, "input", "disable_while_typing", &disable_while_typing);
  get_int(L, "input", "accel_profile", &tmp);
  accel_profile = (enum libinput_config_accel_profile)tmp;
  get_double(L, "input", "accel_speed", &accel_speed);
  get_bool(L, "input", "left_handed", &left_handed);
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
     * with missing or partial hook state. C globals keep their
     * previous config.h or last-successful-load values. */
    lua_rawgeti(L, LUA_REGISTRYINDEX, old_hooks_ref);
    lua_setfield(L, LUA_REGISTRYINDEX, "_bonsaiwm_hooks");
    luaL_unref(L, LUA_REGISTRYINDEX, old_hooks_ref);
    return;
  }

  /* Config loaded successfully: sink declarative fields into C globals,
   * then drop the old hooks table so Lua can GC it. */
  apply_config(L);
  luaL_unref(L, LUA_REGISTRYINDEX, old_hooks_ref);

  config_loaded = 1;
}

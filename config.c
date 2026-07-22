/* See LICENSE file for copyright and license details. */

#include <limits.h>
#include <linux/input-event-codes.h>
#include <stdlib.h>
#include <string.h>
#include <strings.h>
#include <unistd.h>
#include <wlr/types/wlr_output_layout.h>
#include <wlr/util/log.h>

#include "config.h"
#include "lua.h"
#include "util.h"

#define LENGTH(X) (sizeof X / sizeof X[0])

/* appearance */
const int bypass_surface_visibility =
    0; /* 1 means idle inhibitors will disable idle tracking even if its
          surface isn't visible  */
float rootcolor[] = COLOR(0x222222ff);
float bordercolor[] = COLOR(0x444444ff);
float focuscolor[] = COLOR(0x005577ff);
float urgentcolor[] = COLOR(0xff0000ff);
float fullscreen_bg[] = {0.0f, 0.0f, 0.0f,
                         1.0f}; /* You can also use glsl colors */

Config config = {
    .enablegaps = 1,  /* 1 = gaps enabled by default */
    .smartgaps = 1,   /* 1 = no outer gap when only one window */
    .gappih = 10,     /* horiz inner gap between windows */
    .gappiv = 10,     /* vert inner gap between windows */
    .gappoh = 20,     /* horiz outer gap between windows and screen edge */
    .gappov = 20,     /* vert outer gap between windows and screen edge */
    .sloppyfocus = 1, /* focus follows mouse */
    .borderpx = 1,    /* border pixel of windows */
    .repeat_rate = 25,
    .repeat_delay = 600,
};

/* runtime-mutable config fields mirrored on the `bonsaiwm` Lua table.
   Adding a field = add a default above + one row here. */
static const struct {
  const char *lua_key;
  int *as_int;
  unsigned int *as_uint;
  float *as_color;
} config_schema[] = {
    {"enablegaps", &config.enablegaps, NULL, NULL},
    {"smartgaps", &config.smartgaps, NULL, NULL},
    {"gappoh", NULL, &config.gappoh, NULL},
    {"gappov", NULL, &config.gappov, NULL},
    {"gappih", NULL, &config.gappih, NULL},
    {"gappiv", NULL, &config.gappiv, NULL},
    {"sloppyfocus", &config.sloppyfocus, NULL, NULL},
    {"borderpx", NULL, &config.borderpx, NULL},
    {"repeat_rate", &config.repeat_rate, NULL, NULL},
    {"repeat_delay", &config.repeat_delay, NULL, NULL},
    {"rootcolor", NULL, NULL, rootcolor},
    {"bordercolor", NULL, NULL, bordercolor},
    {"focuscolor", NULL, NULL, focuscolor},
    {"urgentcolor", NULL, NULL, urgentcolor},
    {"fullscreen_bg", NULL, NULL, fullscreen_bg},
};

/* window rules: empty by default, populated from config.lua in load_config() */
Rule *rules = NULL;
size_t rules_count = 0;

/* an intentionally do-nothing action, so users can write
 * action = bonsaiwm.action.none in config.lua to shadow (i.e. disable) a
 * default binding. The dispatch loop keybinding() already checks for a NULL
 * k->func to skip dead entries, so a noop must be a real function for the
 * entry to be considered "matched and consumed". */
static void noop(const Arg *arg) { (void)arg; }

void (*const arrangefn[])(Monitor *) = {
    [LtTile] = tile,
    [LtFloat] = NULL,
    [LtMonocle] = monocle,
};

/* dispatch + arg-type tables. Both are generated from actions.def so they
 * cannot drift from the enum or from each other. _Static_assert below still
 * guards against anyone editing actions.def without recompiling. */
void (*const actionfn[ActCount])(const Arg *) = {
#define X(ACT, NAME, FUNC, ARGT) [ACT] = FUNC,
#include "actions.def"
#undef X
};

const int action_arg_type[ActCount] = {
#define X(ACT, NAME, FUNC, ARGT) [ACT] = ARGT,
#include "actions.def"
#undef X
};

_Static_assert(LENGTH(actionfn) == ActCount, "actionfn table out of sync");
_Static_assert(LENGTH(action_arg_type) == ActCount,
               "action_arg_type table out of sync");

/* layout(s) */
Layout *layouts = NULL;
size_t layouts_count = 0;

/* monitors */
/* (x=-1, y=-1) is reserved as an "autoconfigure" monitor position indicator
 * WARNING: negative values other than (-1, -1) cause problems with Xwayland
 * clients due to https://gitlab.freedesktop.org/xorg/xserver/-/issues/899 */
const MonitorRule monrules[] = {
    /* name        mfact  nmaster scale layout       rotate/reflect x    y
     * example of a HiDPI laptop monitor:
     { "eDP-1",    0.5f,  1,      2,    LtTile, WL_OUTPUT_TRANSFORM_NORMAL,
     -1,  -1 }, */
    {NULL, 0.55f, 1, 1, LtTile, WL_OUTPUT_TRANSFORM_NORMAL, -1, -1},
    /* default monitor rule: can be changed but cannot be eliminated; at least
       one monitor rule must exist */
};

/* keyboard */
const struct xkb_rule_names xkb_rules = {
    /* can specify fields: rules, model, layout, variant, options */
    /* example:
    .options = "ctrl:nocaps",
    */
    .options = NULL,
};

/* Trackpad */
const int tap_to_click = 1;
const int tap_and_drag = 1;
const int drag_lock = 1;
const int natural_scrolling = 0;
const int disable_while_typing = 1;
const int left_handed = 0;
const int middle_button_emulation = 0;
/* You can choose between:
LIBINPUT_CONFIG_SCROLL_NO_SCROLL
LIBINPUT_CONFIG_SCROLL_2FG
LIBINPUT_CONFIG_SCROLL_EDGE
LIBINPUT_CONFIG_SCROLL_ON_BUTTON_DOWN
*/
const enum libinput_config_scroll_method scroll_method =
    LIBINPUT_CONFIG_SCROLL_2FG;

/* You can choose between:
LIBINPUT_CONFIG_CLICK_METHOD_NONE
LIBINPUT_CONFIG_CLICK_METHOD_BUTTON_AREAS
LIBINPUT_CONFIG_CLICK_METHOD_CLICKFINGER
*/
const enum libinput_config_click_method click_method =
    LIBINPUT_CONFIG_CLICK_METHOD_BUTTON_AREAS;

/* You can choose between:
LIBINPUT_CONFIG_SEND_EVENTS_ENABLED
LIBINPUT_CONFIG_SEND_EVENTS_DISABLED
LIBINPUT_CONFIG_SEND_EVENTS_DISABLED_ON_EXTERNAL_MOUSE
*/
const uint32_t send_events_mode = LIBINPUT_CONFIG_SEND_EVENTS_ENABLED;

/* You can choose between:
LIBINPUT_CONFIG_ACCEL_PROFILE_FLAT
LIBINPUT_CONFIG_ACCEL_PROFILE_ADAPTIVE
*/
const enum libinput_config_accel_profile accel_profile =
    LIBINPUT_CONFIG_ACCEL_PROFILE_ADAPTIVE;
const double accel_speed = 0.0;

/* You can choose between:
LIBINPUT_CONFIG_TAP_MAP_LRM -- 1/2/3 finger tap maps to left/right/middle
LIBINPUT_CONFIG_TAP_MAP_LMR -- 1/2/3 finger tap maps to left/middle/right
*/
const enum libinput_config_tap_button_map button_map =
    LIBINPUT_CONFIG_TAP_MAP_LRM;

/* escape-hatch bindings, always present regardless of config.lua. These are
 * the *only* defaults; all other bindings (focus, tags, spawn, layouts, etc.)
 * must come from bonsaiwm.keymaps. Deliberately minimal so that a broken
 * config.lua still leaves the user with a way to reload or switch to a TTY
 * and recover, without needing to maintain a parallel list of "user facing"
 * defaults in C. */
static const Key keys_defaults[] = {
    {MODKEY | WLR_MODIFIER_SHIFT, XKB_KEY_r, load_config, {0}},
    CHVT(1),
    CHVT(2),
    CHVT(3),
    CHVT(4),
    CHVT(5),
    CHVT(6),
    CHVT(7),
    CHVT(8),
    CHVT(9),
    CHVT(10),
    CHVT(11),
    CHVT(12),
};

const Button buttons[] = {
    {MODKEY, BTN_LEFT, moveresize, {.ui = CurMove}},
    {MODKEY, BTN_MIDDLE, togglefloating, {0}},
    {MODKEY, BTN_RIGHT, moveresize, {.ui = CurResize}},
};

const size_t monrules_count = LENGTH(monrules);

/* merged key bindings: lua entries first, then keys_defaults. Rebuilt by
 * keys_load() on every config load (Mod-Shift-R). */
Key *keys = NULL;
size_t keys_count = 0;

const size_t buttons_count = LENGTH(buttons);

static void rules_free(void) {
  for (size_t i = 0; i < rules_count; i++) {
    free((char *)rules[i].id);
    free((char *)rules[i].title);
  }
  free(rules);
  rules = NULL;
  rules_count = 0;
}

static void layouts_free(void) {
  for (size_t i = 0; i < layouts_count; i++) {
    free((char *)layouts[i].symbol);
  }
  free(layouts);
  layouts = NULL;
  layouts_count = 0;
}

static void rules_load_from_lua(void) {
  lua_getglobal(L, "bonsaiwm");
  lua_getfield(L, -1, "rules");
  if (!lua_istable(L, -1)) {
    wlr_log(WLR_INFO, "no rules table in config.lua, rules empty");
    lua_pop(L, 2);
    return;
  }
  rules_count = lua_rawlen(L, -1);
  if (rules_count == 0) {
    lua_pop(L, 2);
    return;
  }
  rules = ecalloc(rules_count, sizeof(Rule));
  for (size_t i = 0; i < rules_count; i++) {
    Rule *r = &rules[i];
    lua_rawgeti(L, -1, i + 1);
    lua_getfield(L, -1, "id");
    r->id = lua_isstring(L, -1) ? strdup(lua_tostring(L, -1)) : NULL;
    lua_pop(L, 1);
    lua_getfield(L, -1, "title");
    r->title = lua_isstring(L, -1) ? strdup(lua_tostring(L, -1)) : NULL;
    lua_pop(L, 1);
    lua_getfield(L, -1, "tags");
    r->tags = lua_isinteger(L, -1) ? (uint32_t)lua_tointeger(L, -1) : 0;
    lua_pop(L, 1);
    lua_getfield(L, -1, "isfloating");
    r->isfloating = lua_isinteger(L, -1) ? (int)lua_tointeger(L, -1) : 0;
    lua_pop(L, 1);
    lua_getfield(L, -1, "monitor");
    r->monitor = lua_isinteger(L, -1) ? (int)lua_tointeger(L, -1) : -1;
    lua_pop(L, 1);
    lua_pop(L, 1);
  }
  lua_pop(L, 2);
}

static void layouts_load_defaults(void) {
  layouts = ecalloc(2, sizeof(Layout));
  layouts[0].symbol = strdup("Defloat");
  layouts[0].arrange = LtFloat;
  layouts[1].symbol = strdup("Deftile");
  layouts[1].arrange = LtTile;
  layouts_count = 2;
}

static void layouts_load_from_lua(void) {
  lua_getglobal(L, "bonsaiwm");
  lua_getfield(L, -1, "layouts");
  if (!lua_istable(L, -1) || lua_rawlen(L, -1) == 0) {
    wlr_log(WLR_INFO, "no layouts configured in config.lua, using defaults");
    lua_pop(L, 2);
    layouts_load_defaults();
    return;
  }
  layouts_count = lua_rawlen(L, -1);
  layouts = ecalloc(layouts_count, sizeof(Layout));
  for (size_t i = 0; i < layouts_count; i++) {
    Layout *lyt = &layouts[i];
    lua_rawgeti(L, -1, i + 1);
    lua_getfield(L, -1, "symbol");
    lyt->symbol = lua_isstring(L, -1) ? strdup(lua_tostring(L, -1)) : NULL;
    lua_pop(L, 1);
    lua_getfield(L, -1, "arrange");
    int arrange = lua_isinteger(L, -1) ? (int)lua_tointeger(L, -1) : LtTile;
    lua_pop(L, 1);
    if (arrange < 0 || (size_t)arrange >= LENGTH(arrangefn)) {
      wlr_log(WLR_ERROR,
              "layouts[%zu].arrange=%d out of range [0,%zu), falling back to "
              "LtTile",
              i, arrange, LENGTH(arrangefn));
      arrange = LtTile;
    }
    lyt->arrange = arrange;
    lua_pop(L, 1);
  }
  lua_pop(L, 2);
}

static void keys_free(void) {
  /* free the heap argv arrays attached to spawn entries. Only spawn uses
   * ArgV; for any other action, arg.v reads as garbage from the union
   * overlapping arg.i/arg.ui (e.g. a view entry with arg.ui = 1 would look
   * like arg.v = (void *)1 and free() would crash). Keying on func == spawn
   * is the only reliable signal. */
  for (size_t i = 0; i < keys_count; i++) {
    if (keys[i].func == spawn) {
      /* arg.v is a heap-allocated NULL-terminated argv array of strdup'd
       * strings; free the strings first, then the array itself. */
      char **argv = (char **)keys[i].arg.v;
      if (argv) {
        for (size_t j = 0; argv[j]; j++) {
          free(argv[j]);
        }
        free(argv);
      }
    }
  }
  free(keys);
  keys = NULL;
  keys_count = 0;
}

/* parse a "Alt+Shift" / "Ctrl" / "none" style modifier string into a
 * WLR_MODIFIER_* bitmask. Case-insensitive. On an unrecognized token,
 * returns false (the caller treats this as a malformed entry and skips it,
 * rather than binding to a useless keycombo). On success writes the bitmask
 * to *out and returns true. */
static bool parse_mod(const char *s, uint32_t *out) {
  if (!s || !*s || strcasecmp(s, "none") == 0) {
    *out = 0;
    return true;
  }
  uint32_t mods = 0;
  /* strtok modifies the string, so work on a copy */
  char buf[256];
  strncpy(buf, s, sizeof(buf) - 1);
  buf[sizeof(buf) - 1] = '\0';
  for (char *tok = strtok(buf, "+"); tok; tok = strtok(NULL, "+")) {
    if (strcasecmp(tok, "alt") == 0)
      mods |= WLR_MODIFIER_ALT;
    else if (strcasecmp(tok, "ctrl") == 0)
      mods |= WLR_MODIFIER_CTRL;
    else if (strcasecmp(tok, "shift") == 0)
      mods |= WLR_MODIFIER_SHIFT;
    else if (strcasecmp(tok, "super") == 0 || strcasecmp(tok, "logo") == 0)
      mods |= WLR_MODIFIER_LOGO;
    else {
      wlr_log(WLR_ERROR, "keymap: unknown modifier token \"%s\"", tok);
      return false;
    }
  }
  *out = mods;
  return true;
}

/* translate a lua integer arg for ArgUI actions. Tag actions (view, tag,
 * toggleview, toggletag) take a 1..TAGCOUNT tag number and convert it to the
 * 1<<(n-1) bitmask those C actions expect — asking users to write 1<<8 in
 * lua would be hostile. Other ArgUI actions (currently just chvt) pass
 * their value through unchanged. */
static bool coerce_arg_ui(int action, lua_Integer v, uint32_t *out) {
  switch (action) {
  case ActView:
  case ActToggleView:
  case ActTag:
  case ActToggleTag:
    if (v < 1 || v > TAGCOUNT) {
      wlr_log(WLR_ERROR, "keymap: tag action arg %lld out of range [1,%d]",
              (long long)v, TAGCOUNT);
      return false;
    }
    *out = 1u << (v - 1);
    return true;
  default:
    *out = (uint32_t)v;
    return true;
  }
}

/* build one Key from a lua table at the top of the stack. Returns false and
 * leaves the lua stack untouched on any malformed field (caller skips the
 * entry); on success, fills *out and returns true. The caller is responsible
 * for freeing out->arg.v (it's heap-allocated for ArgV actions). */
static bool keys_parse_entry(Key *out) {
  memset(out, 0, sizeof(*out));

  lua_getfield(L, -1, "mod");
  const char *mod_str = lua_isstring(L, -1) ? lua_tostring(L, -1) : NULL;
  uint32_t mod;
  bool ok = parse_mod(mod_str, &mod);
  lua_pop(L, 1);
  if (!ok)
    return false;

  lua_getfield(L, -1, "key");
  const char *key_str = lua_isstring(L, -1) ? lua_tostring(L, -1) : NULL;
  lua_pop(L, 1);
  if (!key_str) {
    wlr_log(WLR_ERROR, "keymap: missing or non-string \"key\" field");
    return false;
  }
  xkb_keysym_t keysym = xkb_keysym_from_name(key_str, XKB_KEYSYM_NO_FLAGS);
  if (keysym == XKB_KEY_NoSymbol) {
    wlr_log(WLR_ERROR, "keymap: unknown keysym \"%s\"", key_str);
    return false;
  }

  lua_getfield(L, -1, "action");
  if (!lua_isinteger(L, -1)) {
    wlr_log(WLR_ERROR, "keymap: missing or non-integer \"action\" field");
    lua_pop(L, 1);
    return false;
  }
  int action = (int)lua_tointeger(L, -1);
  lua_pop(L, 1);
  if (action < 0 || action >= ActCount) {
    wlr_log(WLR_ERROR, "keymap: action %d out of range [0,%d)", action,
            ActCount);
    return false;
  }

  /* coerce the lua-provided arg according to the action's declared arg type */
  Arg arg = {0};
  lua_getfield(L, -1, "arg");
  switch (action_arg_type[action]) {
  case ArgNone:
    break;
  case ArgI:
    arg.i = lua_isnumber(L, -1) ? (int)lua_tointeger(L, -1) : 0;
    break;
  case ArgUI: {
    lua_Integer v = lua_isnumber(L, -1) ? lua_tointeger(L, -1) : 0;
    if (!coerce_arg_ui(action, v, &arg.ui)) {
      lua_pop(L, 1);
      return false;
    }
    break;
  }
  case ArgF:
    arg.f = lua_isnumber(L, -1) ? (float)lua_tonumber(L, -1) : 0.0f;
    break;
  case ArgV: {
    const char *s = lua_isstring(L, -1) ? lua_tostring(L, -1) : NULL;
    if (!s) {
      wlr_log(WLR_ERROR, "keymap: action %d requires a string arg", action);
      lua_pop(L, 1);
      return false;
    }
    /* spawn() expects an argv array; build {"cmd", NULL} on the heap */
    char **argv = ecalloc(2, sizeof(char *));
    argv[0] = strdup(s);
    arg.v = argv;
    break;
  }
  }
  lua_pop(L, 1);

  out->mod = mod;
  out->keysym = keysym;
  out->func = actionfn[action];
  out->arg = arg;
  return true;
}

/* rebuild keys[] as: [lua-defined entries][defaults]. Lua entries come
 * first so they shadow defaults with the same (mod, keysym) — the dispatch
 * loop returns on first match. Defaults serve as the always-available
 * fallback (Mod-Shift-R reload, Ctrl-Alt-Fn VT switch) when config.lua is
 * broken or empty. Defaults never use ArgV, so keys_free() can identify
 * heap-allocated spawn argv arrays just by checking func == spawn. */
static void keys_load(void) {
  lua_getglobal(L, "bonsaiwm");
  lua_getfield(L, -1, "keymaps");

  size_t lua_count = 0;
  bool has_lua_keymaps = lua_istable(L, -1);
  if (has_lua_keymaps)
    lua_count = lua_rawlen(L, -1);

  /* count valid lua entries by parsing each one. Invalid entries are logged
   * and skipped — partial loads are better than a total loss of bindings. */
  Key *lua_keys = NULL;
  size_t valid = 0;
  if (lua_count > 0) {
    lua_keys = ecalloc(lua_count, sizeof(Key));
    for (size_t i = 0; i < lua_count; i++) {
      lua_rawgeti(L, -1, (lua_Integer)(i + 1));
      if (!lua_istable(L, -1)) {
        wlr_log(WLR_ERROR, "keymap: entry %zu is not a table", i + 1);
        lua_pop(L, 1);
        continue;
      }
      if (keys_parse_entry(&lua_keys[valid]))
        valid++;
      lua_pop(L, 1);
    }
  }

  /* allocate the merged array: lua entries first, then defaults */
  keys_count = valid + LENGTH(keys_defaults);
  keys = ecalloc(keys_count, sizeof(Key));

  /* copy lua entries; their ArgV pointers transfer ownership to keys[] */
  for (size_t i = 0; i < valid; i++) {
    keys[i] = lua_keys[i];
  }
  free(lua_keys);

  /* copy defaults verbatim. Defaults use no ArgV (no spawn bindings), so
   * their arg is a pure value copy and there's nothing to heap-duplicate. */
  for (size_t i = 0; i < LENGTH(keys_defaults); i++) {
    keys[valid + i] = keys_defaults[i];
  }

  lua_pop(L, 2);
}

/* search order for config.lua, per XDG base directory spec:
 * 1. $XDG_CONFIG_HOME/bonsaiwm/config.lua   (if set and absolute)
 * 2. $HOME/.config/bonsaiwm/config.lua       (if HOME set)
 * first candidate that exists wins. a broken lua in that file still wins —
 * broken user config must not silently fall through to another file. */
static const char *resolve_config_path(void) {
  static char path[PATH_MAX];

  const char *xdg = getenv("XDG_CONFIG_HOME");
  if (xdg && xdg[0] == '/') {
    snprintf(path, sizeof path, "%s/bonsaiwm/config.lua", xdg);
    if (access(path, R_OK) == 0)
      return path;
  }

  const char *home = getenv("HOME");
  if (home && home[0]) {
    snprintf(path, sizeof path, "%s/.config/bonsaiwm/config.lua", home);
    if (access(path, R_OK) == 0)
      return path;
  }

  return NULL;
}

void load_config() {
  /* free heap state and close previously running lua runtime to prevent
   * memory leaks when reloading config */
  layouts_free();
  rules_free();
  keys_free();
  if (L) {
    wlr_log(WLR_INFO, "restarting lua runtime");
    lua_close(L);
    L = NULL;
  }
  wlr_log(WLR_INFO, "loading lua config");
  L = lua_init();

  const char *path = resolve_config_path();
  if (path) {
    wlr_log(WLR_INFO, "loading lua config from %s", path);
    lua_load_config(path);
  } else {
    wlr_log(WLR_INFO,
             "no config.lua found in $XDG_CONFIG_HOME/bonsaiwm or "
             "~/.config/bonsaiwm; using builtin defaults");
  }

  wlr_log(WLR_DEBUG, "lua config loaded, applying values");
  lua_getglobal(L, "bonsaiwm");
  for (size_t i = 0; i < LENGTH(config_schema); i++) {
    lua_getfield(L, -1, config_schema[i].lua_key);
    if (lua_isnumber(L, -1)) {
      int v = (int)lua_tonumber(L, -1);
      if (config_schema[i].as_int)
        *config_schema[i].as_int = v;
      if (config_schema[i].as_uint)
        *config_schema[i].as_uint = (unsigned)v;
    } else if (lua_isstring(L, -1) && config_schema[i].as_color) {
      if (hex_to_rgba(lua_tostring(L, -1), config_schema[i].as_color) < 0)
        wlr_log(WLR_ERROR, "invalid color for %s: %s", config_schema[i].lua_key,
                lua_tostring(L, -1));
    }
    lua_pop(L, 1);
  }
  lua_pop(L, 1);
  rules_load_from_lua();
  layouts_load_from_lua();
  keys_load();
  reload_monitor_layouts();
}

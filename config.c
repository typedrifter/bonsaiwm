/* See LICENSE file for copyright and license details. */

#include <linux/input-event-codes.h>
#include <stdlib.h>
#include <string.h>
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

void (*const arrangefn[])(Monitor *) = {
    [LtTile] = tile,
    [LtFloat] = NULL,
    [LtMonocle] = monocle,
};

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

/* commands */
const char *termcmd[] = {"foot", NULL};
const char *menucmd[] = {"wmenu-run", NULL};

const Key keys[] = {
    /* Note that Shift changes certain key codes: 2 -> at, etc. */
    /* modifier                  key                  function          argument
     */
    {MODKEY, XKB_KEY_p, spawn, {.v = menucmd}},
    {MODKEY | WLR_MODIFIER_SHIFT, XKB_KEY_Return, spawn, {.v = termcmd}},
    {MODKEY | WLR_MODIFIER_SHIFT, XKB_KEY_r, load_config, {0}},
    {MODKEY, XKB_KEY_j, focusstack, {.i = +1}},
    {MODKEY, XKB_KEY_k, focusstack, {.i = -1}},
    {MODKEY, XKB_KEY_i, incnmaster, {.i = +1}},
    {MODKEY, XKB_KEY_d, incnmaster, {.i = -1}},
    {MODKEY, XKB_KEY_h, setmfact, {.f = -0.05f}},
    {MODKEY, XKB_KEY_l, setmfact, {.f = +0.05f}},
    {MODKEY, XKB_KEY_Return, zoom, {0}},
    {MODKEY, XKB_KEY_Tab, view, {0}},
    {MODKEY | WLR_MODIFIER_SHIFT, XKB_KEY_c, killclient, {0}},
    {MODKEY, XKB_KEY_t, setlayout, {.i = 1}},
    {MODKEY, XKB_KEY_f, setlayout, {.i = 0}},
    {MODKEY, XKB_KEY_m, setlayout, {.i = 2}},
    {MODKEY, XKB_KEY_space, setlayout, {.i = -1}}, /* just toggle lt[0]/lt[1] */
    {MODKEY | WLR_MODIFIER_SHIFT, XKB_KEY_space, togglefloating, {0}},
    {MODKEY, XKB_KEY_e, togglefullscreen, {0}},
    {MODKEY, XKB_KEY_0, view, {.ui = ~0}},
    {MODKEY | WLR_MODIFIER_SHIFT, XKB_KEY_parenright, tag, {.ui = ~0}},
    {MODKEY, XKB_KEY_comma, focusmon, {.i = WLR_DIRECTION_LEFT}},
    {MODKEY, XKB_KEY_period, focusmon, {.i = WLR_DIRECTION_RIGHT}},
    {MODKEY | WLR_MODIFIER_SHIFT,
     XKB_KEY_less,
     tagmon,
     {.i = WLR_DIRECTION_LEFT}},
    {MODKEY | WLR_MODIFIER_SHIFT,
     XKB_KEY_greater,
     tagmon,
     {.i = WLR_DIRECTION_RIGHT}},
    TAGKEYS(XKB_KEY_1, XKB_KEY_exclam, 0),
    TAGKEYS(XKB_KEY_2, XKB_KEY_at, 1),
    TAGKEYS(XKB_KEY_3, XKB_KEY_numbersign, 2),
    TAGKEYS(XKB_KEY_4, XKB_KEY_dollar, 3),
    TAGKEYS(XKB_KEY_5, XKB_KEY_percent, 4),
    TAGKEYS(XKB_KEY_6, XKB_KEY_asciicircum, 5),
    TAGKEYS(XKB_KEY_7, XKB_KEY_ampersand, 6),
    TAGKEYS(XKB_KEY_8, XKB_KEY_asterisk, 7),
    TAGKEYS(XKB_KEY_9, XKB_KEY_parenleft, 8),
    {MODKEY | WLR_MODIFIER_SHIFT, XKB_KEY_q, quit, {0}},

    /* Ctrl-Alt-Backspace and Ctrl-Alt-Fx used to be handled by X server */
    {WLR_MODIFIER_CTRL | WLR_MODIFIER_ALT, XKB_KEY_Terminate_Server, quit, {0}},
    /* Ctrl-Alt-Fx is used to switch to another VT, if you don't know what a VT
     * is do not remove them.
     */
    {MODKEY, XKB_KEY_g, togglegaps, {0}},
    {MODKEY | WLR_MODIFIER_SHIFT, XKB_KEY_g, defaultgaps, {0}},
    {MODKEY, XKB_KEY_Up, incgaps, {.i = +1}},
    {MODKEY, XKB_KEY_Down, incgaps, {.i = -1}},
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
const size_t keys_count = LENGTH(keys);
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

void load_config() {
  // free heap rules and close previously running lua runtime to prevent memory
  // leaks when reloading config
  layouts_free();
  rules_free();
  if (L) {
    wlr_log(WLR_INFO, "restarting lua runtime");
    lua_close(L);
    L = NULL;
  }
  wlr_log(WLR_INFO, "loading lua config");
  L = lua_init();
  lua_load_config("./config.lua");
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
  reload_monitor_layouts();
}

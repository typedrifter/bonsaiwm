/* See LICENSE file for copyright and license details. */

#include <linux/input-event-codes.h>
#include <wlr/types/wlr_output_layout.h>
#include <wlr/util/log.h>

#include "config.h"

#define LENGTH(X) (sizeof X / sizeof X[0])

/* appearance */
const int sloppyfocus = 1;        /* focus follows mouse */
const int bypass_surface_visibility =
    0; /* 1 means idle inhibitors will disable idle tracking even if its
          surface isn't visible  */
const unsigned int borderpx = 1; /* border pixel of windows */
const float rootcolor[] = COLOR(0x222222ff);
const float bordercolor[] = COLOR(0x444444ff);
const float focuscolor[] = COLOR(0x005577ff);
const float urgentcolor[] = COLOR(0xff0000ff);
const float fullscreen_bg[] = {0.0f, 0.0f, 0.0f,
                               1.0f}; /* You can also use glsl colors */

int enablegaps = 1;       /* 1 = gaps enabled by default */
const int smartgaps = 1;  /* 1 = no outer gap when only one window */
unsigned int gappih = 10; /* horiz inner gap between windows */
unsigned int gappiv = 10; /* vert inner gap between windows */
unsigned int gappoh = 20; /* horiz outer gap between windows and screen edge */
unsigned int gappov = 20; /* vert outer gap between windows and screen edge */

/* logging */
int log_level = WLR_ERROR;

const Rule rules[] = {
    /* app_id             title       tags mask     isfloating   monitor */
    {"Gimp_EXAMPLE", NULL, 0, 1,
     -1}, /* Start on currently visible tags floating, not tiled */
    {"firefox_EXAMPLE", NULL, 1 << 8, 0, -1}, /* Start on ONLY tag "9" */
    /* default/example rule: can be changed but cannot be eliminated; at least
       one rule must exist */
};

/* layout(s) */
const Layout layouts[] = {
    /* symbol     arrange function */
    {"[]=", tile},
    {"><>", NULL}, /* no layout function means floating behavior */
    {"[M]", monocle},
};

/* monitors */
/* (x=-1, y=-1) is reserved as an "autoconfigure" monitor position indicator
 * WARNING: negative values other than (-1, -1) cause problems with Xwayland
 * clients due to https://gitlab.freedesktop.org/xorg/xserver/-/issues/899 */
const MonitorRule monrules[] = {
    /* name        mfact  nmaster scale layout       rotate/reflect x    y
     * example of a HiDPI laptop monitor:
     { "eDP-1",    0.5f,  1,      2,    &layouts[0], WL_OUTPUT_TRANSFORM_NORMAL,
     -1,  -1 }, */
    {NULL, 0.55f, 1, 1, &layouts[0], WL_OUTPUT_TRANSFORM_NORMAL, -1, -1},
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

const int repeat_rate = 25;
const int repeat_delay = 600;

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
    {MODKEY, XKB_KEY_j, focusstack, {.i = +1}},
    {MODKEY, XKB_KEY_k, focusstack, {.i = -1}},
    {MODKEY, XKB_KEY_i, incnmaster, {.i = +1}},
    {MODKEY, XKB_KEY_d, incnmaster, {.i = -1}},
    {MODKEY, XKB_KEY_h, setmfact, {.f = -0.05f}},
    {MODKEY, XKB_KEY_l, setmfact, {.f = +0.05f}},
    {MODKEY, XKB_KEY_Return, zoom, {0}},
    {MODKEY, XKB_KEY_Tab, view, {0}},
    {MODKEY | WLR_MODIFIER_SHIFT, XKB_KEY_c, killclient, {0}},
    {MODKEY, XKB_KEY_t, setlayout, {.v = &layouts[0]}},
    {MODKEY, XKB_KEY_f, setlayout, {.v = &layouts[1]}},
    {MODKEY, XKB_KEY_m, setlayout, {.v = &layouts[2]}},
    {MODKEY, XKB_KEY_space, setlayout, {0}},
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

const size_t rules_count = LENGTH(rules);
const size_t layouts_count = LENGTH(layouts);
const size_t monrules_count = LENGTH(monrules);
const size_t keys_count = LENGTH(keys);
const size_t buttons_count = LENGTH(buttons);

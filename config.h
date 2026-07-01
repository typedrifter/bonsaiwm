/* See LICENSE file for copyright and license details. */

#ifndef CONFIG_H
#define CONFIG_H

#include <libinput.h>
#include <stddef.h>
#include <stdint.h>
#include <wlr/types/wlr_keyboard.h>
#include <wlr/types/wlr_output.h>
#include <xkbcommon/xkbcommon.h>

/* forward declaration of the compositor's Monitor type */
typedef struct Monitor Monitor;

/* Taken from https://github.com/djpohly/dwl/issues/466 */
#define COLOR(hex)                                                             \
  {((hex >> 24) & 0xFF) / 255.0f, ((hex >> 16) & 0xFF) / 255.0f,               \
   ((hex >> 8) & 0xFF) / 255.0f, (hex & 0xFF) / 255.0f}

/* cursor symbols */
enum { CurNormal, CurPressed, CurMove, CurResize };

/* argument passed to a callback function */
typedef union {
  int i;
  uint32_t ui;
  float f;
  const void *v;
} Arg;

/* a monitor layout */
typedef struct {
  const char *symbol;
  void (*arrange)(Monitor *);
} Layout;

/* a tag/rule for a client */
typedef struct {
  const char *id;
  const char *title;
  uint32_t tags;
  int isfloating;
  int monitor;
} Rule;

/* output configuration matched when a monitor appears */
typedef struct {
  const char *name;
  float mfact;
  int nmaster;
  float scale;
  const Layout *lt;
  enum wl_output_transform rr;
  int x, y;
} MonitorRule;

/* a key binding */
typedef struct {
  uint32_t mod;
  xkb_keysym_t keysym;
  void (*func)(const Arg *);
  const Arg arg;
} Key;

/* a mouse button binding */
typedef struct {
  uint32_t mod;
  unsigned int button;
  void (*func)(const Arg *);
  const Arg arg;
} Button;

/* tagging - TAGCOUNT must be no greater than 31 */
#define TAGCOUNT (9)

/* modifier key used in bindings */
#define MODKEY WLR_MODIFIER_ALT

/* generate tag key bindings */
#define TAGKEYS(KEY, SKEY, TAG)                                                \
  {MODKEY, KEY, view, {.ui = 1 << TAG}},                                       \
      {MODKEY | WLR_MODIFIER_CTRL, KEY, toggleview, {.ui = 1 << TAG}},         \
      {MODKEY | WLR_MODIFIER_SHIFT, SKEY, tag, {.ui = 1 << TAG}}, {            \
    MODKEY | WLR_MODIFIER_CTRL | WLR_MODIFIER_SHIFT, SKEY, toggletag, {        \
      .ui = 1 << TAG                                                           \
    }                                                                          \
  }

/* helper for spawning shell commands in the pre dwm-5.0 fashion */
#define SHCMD(cmd)                                                             \
  {                                                                            \
    .v = (const char *[]) { "/bin/sh", "-c", cmd, NULL }                       \
  }

/* generate Ctrl-Alt-Fn virtual terminal switch bindings */
#define CHVT(n)                                                                \
  {                                                                            \
    WLR_MODIFIER_CTRL | WLR_MODIFIER_ALT, XKB_KEY_XF86Switch_VT_##n, chvt, {   \
      .ui = (n)                                                                \
    }                                                                          \
  }

/* functions referenced by the configuration arrays */
extern void chvt(const Arg *arg);
extern void defaultgaps(const Arg *arg);
extern void focusmon(const Arg *arg);
extern void focusstack(const Arg *arg);
extern void incgaps(const Arg *arg);
extern void incnmaster(const Arg *arg);
extern void killclient(const Arg *arg);
extern void monocle(Monitor *m);
extern void moveresize(const Arg *arg);
extern void quit(const Arg *arg);
extern void setlayout(const Arg *arg);
extern void setmfact(const Arg *arg);
extern void spawn(const Arg *arg);
extern void tag(const Arg *arg);
extern void tagmon(const Arg *arg);
extern void tile(Monitor *m);
extern void togglefloating(const Arg *arg);
extern void togglefullscreen(const Arg *arg);
extern void togglegaps(const Arg *arg);
extern void toggletag(const Arg *arg);
extern void toggleview(const Arg *arg);
extern void view(const Arg *arg);
extern void zoom(const Arg *arg);

/* runtime-mutable configuration values */
extern int enablegaps;
extern int smartgaps;
extern unsigned int gappih;
extern unsigned int gappiv;
extern unsigned int gappoh;
extern unsigned int gappov;
extern int log_level;

/* appearance */
extern const int sloppyfocus;
extern const int bypass_surface_visibility;
extern const unsigned int borderpx;
extern const float rootcolor[];
extern const float bordercolor[];
extern const float focuscolor[];
extern const float urgentcolor[];
extern const float fullscreen_bg[];

/* window rules */
extern const Rule rules[];
extern const size_t rules_count;

/* layouts */
extern const Layout layouts[];
extern const size_t layouts_count;

/* monitor rules */
extern const MonitorRule monrules[];
extern const size_t monrules_count;

/* keyboard */
extern const struct xkb_rule_names xkb_rules;
extern const int repeat_rate;
extern const int repeat_delay;

/* trackpad */
extern const int tap_to_click;
extern const int tap_and_drag;
extern const int drag_lock;
extern const int natural_scrolling;
extern const int disable_while_typing;
extern const int left_handed;
extern const int middle_button_emulation;
extern const enum libinput_config_scroll_method scroll_method;
extern const enum libinput_config_click_method click_method;
extern const uint32_t send_events_mode;
extern const enum libinput_config_accel_profile accel_profile;
extern const double accel_speed;
extern const enum libinput_config_tap_button_map button_map;

/* commands */
extern const char *termcmd[];
extern const char *menucmd[];

/* bindings */
extern const Key keys[];
extern const size_t keys_count;
extern const Button buttons[];
extern const size_t buttons_count;

void load_config();

#endif /* CONFIG_H */

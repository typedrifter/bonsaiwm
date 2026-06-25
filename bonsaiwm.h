#ifndef BONSAIWM_H
#define BONSAIWM_H

#include <stdint.h>
#include <libinput.h>
#include <xkbcommon/xkbcommon.h>

#define CLEANMASK(mask) (mask & ~WLR_MODIFIER_CAPS)

/* Forward declarations for state shared with bonsaiwm_lua.c */
struct Monitor;
extern struct Monitor *selmon;

/* Mutable config globals read by apply_config(). These are defined in config.h
 * and used by createmon/createpointer/setup/keymap setup. Declared here so
 * bonsaiwm_lua.c can write to them without including the full config.h
 * (which contains keys[]/buttons[] arrays referencing C symbols). */
extern int sloppyfocus;
extern unsigned int borderpx;
extern float rootcolor[];
extern float bordercolor[];
extern float focuscolor[];
extern float urgentcolor[];
extern float fullscreen_bg[];
extern int enablegaps;
extern int smartgaps;
extern unsigned int gappih;
extern unsigned int gappiv;
extern unsigned int gappoh;
extern unsigned int gappov;
extern int repeat_rate;
extern int repeat_delay;
extern struct xkb_rule_names xkb_rules;
extern int tap_to_click;
extern int natural_scrolling;
extern int disable_while_typing;
extern int left_handed;
extern enum libinput_config_accel_profile accel_profile;
extern double accel_speed;

/* Request a config reload. Exposed for the Lua bonsaiwm.reload() binding. */
void bonsaiwm_request_reload(void);

void setgaps(int oh, int ov, int ih, int iv);
void adjustgaps(int delta);
void resetgaps(void);
void setnmaster(int n);
void adjustnmaster(int delta);
int setmfact_val(float f);
int adjustmfact(float delta);
void setsloppyfocus(int v);
void setsmartgaps(int v);
void setborderwidth(unsigned int px);
void setbordercolor(float r, float g, float b, float a);
void setfocuscolor(float r, float g, float b, float a);
void seturgentcolor(float r, float g, float b, float a);
void setrootcolor(float r, float g, float b, float a);

int bonsaiwm_lua_keybind(uint32_t mod, xkb_keysym_t sym);

#endif /* BONSAIWM_H */

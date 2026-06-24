#ifndef BONSAIWM_H
#define BONSAIWM_H

#include <stdint.h>
#include <xkbcommon/xkbcommon.h>

#define CLEANMASK(mask) (mask & ~WLR_MODIFIER_CAPS)

void setgaps(int oh, int ov, int ih, int iv);
void setborderwidth(unsigned int px);
void setbordercolor(float r, float g, float b, float a);
void setfocuscolor(float r, float g, float b, float a);
void seturgentcolor(float r, float g, float b, float a);
void setrootcolor(float r, float g, float b, float a);

int bonsaiwm_lua_keybind(uint32_t mod, xkb_keysym_t sym);

#endif /* BONSAIWM_H */

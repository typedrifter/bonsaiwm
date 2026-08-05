/* Focus module: ownership of the focus stack (fstack) and focus queries.
 *
 * Owns:
 *   - fstack (the focus-ordered list of Clients)
 *   - focustop  (which Client has focus on a Monitor)
 *   - focusmon / focusstack (action handlers for cycling focus)
 *   - dirtomon  (find Monitor adjacent in a direction)
 *
 * Does NOT own:
 *   - focusclient (the heavy side-effect orchestrator; stays in bonsaiwm.c)
 *   - pointerfocus (cursor → focus bridge; stays in bonsaiwm.c)
 */
#ifndef FOCUS_H
#define FOCUS_H

#include <wayland-util.h>
#include <wlr/types/wlr_output_layout.h>

#include "bonsaiwm.h"

/* globals defined in bonsaiwm.c, accessed by focus module */
extern struct wl_list fstack;
extern struct wl_list mons;
extern Monitor *selmon;
extern struct wlr_output_layout *output_layout;

/* focusclient is the heavy side-effect orchestrator (still in bonsaiwm.c) */
void focusclient(Client *c, int lift);

Client *focustop(Monitor *m);
void focusmon(const Arg *arg);
void focusstack(const Arg *arg);
Monitor *dirtomon(enum wlr_direction dir);

#endif /* FOCUS_H */

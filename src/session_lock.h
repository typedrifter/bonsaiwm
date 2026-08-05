/* Session lock module: ownership of the session lock manager and lock lifecycle.
 *
 * Owns:
 *   - session_lock_mgr (the wlr_session_lock_manager_v1)
 *   - cur_lock (the current active lock)
 *   - locked_bg (the scene background for the lock screen)
 *   - All session lock listener functions and wl_listener instances
 *
 * Does NOT own:
 *   - locked / exclusive_focus (shared compositor state, in bonsaiwm.c)
 *   - scene / layers[] (shared scene graph, in bonsaiwm.c)
 */
#ifndef SESSION_LOCK_H
#define SESSION_LOCK_H

#include <wayland-util.h>
#include <wlr/types/wlr_seat.h>
#include <wlr/types/wlr_session_lock_v1.h>
#include <wlr/types/wlr_idle_inhibit_v1.h>
#include <scenefx/types/wlr_scene.h>

#include "bonsaiwm.h"

extern struct wl_display *dpy;
extern struct wlr_scene *scene;
extern struct wlr_scene_tree *layers[];
extern struct wlr_seat *seat;
extern struct wlr_box sgeom;
extern int locked;
extern void *exclusive_focus;
extern Monitor *selmon;
extern struct wlr_idle_notifier_v1 *idle_notifier;
extern struct wlr_idle_inhibit_manager_v1 *idle_inhibit_mgr;

extern struct wlr_scene_rect *locked_bg;

void session_lock_init(void);
void session_lock_cleanup(void);
void checkidleinhibitor(struct wlr_surface *exclude);
void destroylocksurface(struct wl_listener *listener, void *data);

#endif /* SESSION_LOCK_H */

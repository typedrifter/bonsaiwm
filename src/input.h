/* Input module: ownership of input handling — keyboard, pointer, cursor,
 * virtual devices, pointer constraints, and selection.
 *
 * Owns:
 *   - kb_group (the KeyboardGroup aggregating all physical keyboards)
 *   - cursor, cursor_mgr, cursor_shape_mgr (cursor state)
 *   - virtual_keyboard_mgr, virtual_pointer_mgr
 *   - pointer_constraints, active_constraint, relative_pointer_mgr
 *   - cursor_mode, grabc, grabcx, grabcy (grab state)
 *   - All input listener functions (keyboard, pointer, cursor, virtual, etc.)
 *
 * Does NOT own:
 *   - seat, dpy, backend, scene, layers, output_layout, mons, selmon
 *     (shared compositor state, in bonsaiwm.c)
 *   - motionnotify (used by other modules, defined in bonsaiwm.c)
 *   - xytonode (used by moveresize, in bonsaiwm.c)
 */
#ifndef INPUT_H
#define INPUT_H

#include <wayland-util.h>
#include <wlr/backend.h>
#include <wlr/types/wlr_seat.h>
#include <wlr/types/wlr_cursor.h>
#include <wlr/types/wlr_keyboard_group.h>
#include <scenefx/types/wlr_scene.h>

#include "bonsaiwm.h"

extern struct wl_display *dpy;
extern struct wlr_backend *backend;
extern struct wlr_scene *scene;
extern struct wlr_scene_tree *layers[];
extern struct wlr_seat *seat;
extern struct wlr_output_layout *output_layout;
extern struct wlr_compositor *compositor;
extern struct wlr_renderer *drw;
extern struct wl_list fstack;
extern struct wl_list mons;
extern Monitor *selmon;
extern struct wlr_idle_notifier_v1 *idle_notifier;
extern int locked;
extern struct wlr_xcursor_manager *cursor_mgr;

extern KeyboardGroup *kb_group;

KeyboardGroup *createkeyboardgroup(void);
void destroykeyboardgroup(struct wl_listener *listener, void *data);

void pointerfocus(Client *c, struct wlr_surface *surface, double sx, double sy,
                  uint32_t time);
void cursorconstrain(struct wlr_pointer_constraint_v1 *constraint);

void input_init(void);
void input_cleanup(void);

#endif /* INPUT_H */

/* Layer shell module: ownership of the layer shell protocol lifecycle and
 * layer surface arrangement.
 *
 * Owns:
 *   - layer_shell (the wlr_layer_shell_v1)
 *   - All layer surface listener functions (create, destroy, commit, unmap)
 *   - Layer arrangement computation (arrangelayer, arrangelayers)
 *
 * Does NOT own:
 *   - layers[] (shared scene graph trees, in bonsaiwm.c)
 *   - scene / seat / selmon / mons (shared compositor state)
 */
#ifndef LAYER_SHELL_H
#define LAYER_SHELL_H

#include <wayland-util.h>
#include <wlr/types/wlr_layer_shell_v1.h>
#include <wlr/types/wlr_seat.h>
#include <wlr/util/box.h>
#include <scenefx/types/wlr_scene.h>

#include "bonsaiwm.h"

extern struct wl_display *dpy;
extern struct wlr_scene *scene;
extern struct wlr_scene_tree *layers[];
extern struct wlr_seat *seat;
extern Monitor *selmon;
extern struct wl_list mons;
extern int locked;
extern void *exclusive_focus;

/* config.h globals referenced during commit */
extern int blur;

void layer_shell_init(void);
void layer_shell_cleanup(void);
void arrangelayers(Monitor *m);

#endif /* LAYER_SHELL_H */

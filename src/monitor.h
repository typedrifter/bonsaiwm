/* Monitor module: ownership of output management, monitor lifecycle, and
 * power management.
 *
 * Owns:
 *   - output_mgr (the wlr_output_manager_v1)
 *   - power_mgr (the wlr_output_power_manager_v1)
 *   - Monitor lifecycle (createmon, cleanupmon, closemon)
 *   - Output layout change handler (updatemons)
 *   - Frame rendering (rendermon)
 *   - Output management apply/test (outputmgrapply, outputmgrtest)
 *   - Power management (powermgrsetmode, requestmonstate)
 *
 * Does NOT own:
 *   - output_layout (shared compositor state, in bonsaiwm.c)
 *   - scene/layers (shared scene graph, in bonsaiwm.c)
 *   - seat/selmon/mons (shared compositor state)
 */
#ifndef MONITOR_H
#define MONITOR_H

#include <wayland-util.h>
#include <wlr/backend.h>
#include <wlr/render/allocator.h>
#include <wlr/render/wlr_renderer.h>
#include <wlr/types/wlr_compositor.h>
#include <wlr/types/wlr_output.h>
#include <wlr/types/wlr_output_layout.h>
#include <scenefx/types/wlr_scene.h>

#include "bonsaiwm.h"

extern struct wl_display *dpy;
extern struct wlr_backend *backend;
extern struct wlr_scene *scene;
extern struct wlr_scene_tree *layers[];
extern struct wlr_seat *seat;
extern struct wlr_output_layout *output_layout;
extern struct wlr_renderer *drw;
extern struct wlr_allocator *alloc;
extern struct wlr_compositor *compositor;
extern struct wlr_scene_rect *root_bg;
extern struct wlr_box sgeom;
extern Monitor *selmon;
extern struct wl_list mons;

void monitor_init(void);
void monitor_cleanup(void);

#endif /* MONITOR_H */

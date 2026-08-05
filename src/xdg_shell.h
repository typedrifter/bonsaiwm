/* XDG shell module: ownership of the XDG toplevel, popup, and decoration
 * protocol lifecycle.
 *
 * Owns:
 *   - xdg_shell (the wlr_xdg_shell)
 *   - activation (the wlr_xdg_activation_v1)
 *   - xdg_decoration_mgr (the wlr_xdg_decoration_manager_v1)
 *   - All XDG toplevel listener functions (create, map, unmap, destroy,
 *     commit, title, fullscreen, maximize, urgent)
 *   - All XDG popup listener functions (create, commit)
 *   - All XDG decoration listener functions (create, destroy, mode)
 *   - Rule application (applyrules)
 *
 * Does NOT own:
 *   - scene / layers (shared scene graph, in bonsaiwm.c)
 *   - seat / selmon / mons / fstack (shared compositor state)
 *   - setfloating / setfullscreen / setmon / resize (cross-cutting helpers,
 *     in bonsaiwm.c)
 */
#ifndef XDG_SHELL_H
#define XDG_SHELL_H

#include <wayland-util.h>
#include <wlr/types/wlr_seat.h>
#include <wlr/types/wlr_xdg_shell.h>
#include <wlr/types/wlr_xdg_activation_v1.h>
#include <wlr/types/wlr_xdg_decoration_v1.h>
#include <scenefx/types/wlr_scene.h>

#include "bonsaiwm.h"

extern struct wl_display *dpy;
extern struct wlr_scene *scene;
extern struct wlr_scene_tree *layers[];
extern struct wlr_seat *seat;
extern struct wlr_output_layout *output_layout;
extern struct wlr_compositor *compositor;
extern struct wlr_xdg_shell *xdg_shell;
extern struct wlr_xdg_activation_v1 *activation;
extern struct wlr_xdg_decoration_manager_v1 *xdg_decoration_mgr;
extern struct wlr_renderer *drw;
extern struct wlr_allocator *alloc;
extern struct wl_list fstack;
extern struct wl_list mons;
extern Monitor *selmon;
extern struct wlr_box sgeom;
extern int locked;
extern void *exclusive_focus;
extern struct wlr_backend *backend;

void mapnotify(struct wl_listener *listener, void *data);
void unmapnotify(struct wl_listener *listener, void *data);
void destroynotify(struct wl_listener *listener, void *data);
void fullscreennotify(struct wl_listener *listener, void *data);
void updatetitle(struct wl_listener *listener, void *data);

void xdg_shell_init(void);
void xdg_shell_cleanup(void);

#endif /* XDG_SHELL_H */

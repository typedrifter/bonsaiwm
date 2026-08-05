/* Client abstraction: uniform accessors over XDG Shell and XWayland surfaces.
 *
 * Owns:
 *   - surface type dispatch (XDG vs. X11)
 *   - geometry, app-id, title queries
 *   - float-type detection
 *   - focus/activate/close protocol calls
 *   - toplevel_from_wlr_surface (surface → Client/LayerSurface lookup)
 *   - scene-graph border-color helper
 *
 * Does NOT own: the Client struct itself (bonsaiwm.h), scene-graph trees,
 *   or compositor-level focus decisions.
 */
#ifndef CLIENT_H
#define CLIENT_H

#include <wayland-util.h>
#include <wlr/util/box.h>

#include "bonsaiwm.h"

/* globals accessed by client.c */
extern struct wlr_seat *seat;

/* ── Core identity ── */

int client_is_x11(Client *c);
struct wlr_surface *client_surface(Client *c);
int client_is_unmanaged(Client *c);

/* ── Surface lookup ── */

int toplevel_from_wlr_surface(struct wlr_surface *s, Client **pc,
                               LayerSurface **pl);

/* ── Activation / protocol ── */

void client_activate_surface(struct wlr_surface *s, int activated);
void client_send_close(Client *c);
void client_notify_enter(struct wlr_surface *s, struct wlr_keyboard *kb);
int client_wants_focus(Client *c);
int client_wants_fullscreen(Client *c);

/* ── Geometry / sizing ── */

void client_get_geometry(Client *c, struct wlr_box *geom);
void client_get_clip(Client *c, struct wlr_box *clip);
uint32_t client_set_bounds(Client *c, int32_t width, int32_t height);
uint32_t client_set_size(Client *c, uint32_t width, uint32_t height);
void client_set_tiled(Client *c, uint32_t edges);
void client_set_fullscreen(Client *c, int fullscreen);
void client_set_suspended(Client *c, int suspended);
void client_set_scale(struct wlr_surface *s, float scale);

/* ── Properties ── */

const char *client_get_appid(Client *c);
const char *client_get_title(Client *c);
Client *client_get_parent(Client *c);
int client_has_children(Client *c);
int client_is_float_type(Client *c);
int client_is_rendered_on_mon(Client *c, Monitor *m);
int client_is_stopped(Client *c);

/* ── Scene-graph helpers ── */

void client_set_border_color(Client *c, const float color[static 4]);

#endif /* CLIENT_H */

/* Decorations module: scene-fx visual effects (corner radius, shadow,
 * blur, opacity) for Client surfaces.
 *
 * Owns:
 *   - all wlr_scene_buffer / wlr_scene_shadow / wlr_scene_rect mutations
 *     for visual decorations
 *   - the transparent color constant
 *   - per-buffer scene-fx iterators used by wlr_scene_node_for_each_buffer
 *
 * The compositor (bonsaiwm.c) decides *when* decorations are applied;
 * this module owns *how*.
 */
#ifndef DECORATIONS_H
#define DECORATIONS_H

#include <scenefx/types/fx/clipped_region.h>
#include <scenefx/types/fx/corner_location.h>
#include <scenefx/types/wlr_scene.h>

#include "bonsaiwm.h"

extern float transparent[4];

void apply_output_scene_effects(struct wlr_scene_node *node, Client *c);
void apply_client_decorations(Client *c, int focused);

void update_client_focus_decorations(Client *c, int focused, int urgent);
void update_client_corner_radius(Client *c);
void update_client_blur(Client *c);
void update_client_shadow_color(Client *c, int focused);
void update_buffer_corner_radius(Client *c, struct wlr_scene_buffer *buffer);

int effective_corner_radius(Client *c);
enum corner_location set_client_corner_location(Client *c);
void client_set_shadow_blur_sigma(Client *c, int blur_sigma);

void iter_xdg_scene_buffers(struct wlr_scene_buffer *buffer, int sx, int sy,
                            void *user_data);
void iter_xdg_scene_buffers_opacity(struct wlr_scene_buffer *buffer, int sx,
                                    int sy, void *user_data);

#endif /* DECORATIONS_H */

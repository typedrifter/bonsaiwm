/* Decorations module implementation. Scene-fx visual effects applied to
 * Client surfaces: corner radius, shadow, blur, opacity. */
#include <math.h>
#include <string.h>

#include <wlr/types/wlr_xdg_shell.h>
#include <wlr/util/log.h>

#ifdef XWAYLAND
#include <wlr/xwayland.h>
#include <xcb/xcb.h>
#include <xcb/xcb_icccm.h>
#endif

#include "decorations.h"
#include "config.h"
#include "layout.h"
#include "client.h"

float transparent[4] = {0.1f, 0.1f, 0.1f, 0.0f};

/* ── Internal helpers ── */

static struct wlr_surface *client_surface_from_buffer(
    struct wlr_scene_buffer *buffer, Client *c) {
  struct wlr_scene_surface *scene_surface;
  struct wlr_surface *surface;

  if (!c || (c->type != XDGShell && c->type != X11)) {
    return NULL;
  }

  scene_surface = wlr_scene_surface_try_from_buffer(buffer);
  if (!scene_surface) {
    return NULL;
  }

  surface = scene_surface->surface;
  if (surface != client_surface(c)) {
    return NULL;
  }
  return surface;
}

static struct wlr_surface *iter_xdg_get_surface(
    struct wlr_scene_buffer *buffer, void *user_data, Client **c) {
  *c = user_data;
  return client_surface_from_buffer(buffer, user_data);
}

/* ── Scene buffer iterators (passed to wlr_scene_node_for_each_buffer) ── */

void iter_xdg_scene_buffers(struct wlr_scene_buffer *buffer, int sx, int sy,
                            void *user_data) {
  Client *c;
  struct wlr_surface *surface = iter_xdg_get_surface(buffer, user_data, &c);
  if (!surface)
    return;

  if (opacity)
    wlr_scene_buffer_set_opacity(buffer, c->opacity);

  update_buffer_corner_radius(c, buffer);

  if (blur) {
    int blur_optimized = !c->isfloating || blur_xray;
    wlr_scene_buffer_set_backdrop_blur(buffer, 1);
    wlr_scene_buffer_set_backdrop_blur_optimized(buffer, blur_optimized);
    wlr_scene_buffer_set_backdrop_blur_ignore_transparent(
        buffer, blur_ignore_transparent);
  }
}

void iter_xdg_scene_buffers_blur(struct wlr_scene_buffer *buffer, int sx,
                                 int sy, void *user_data) {
  Client *c;
  struct wlr_surface *surface = iter_xdg_get_surface(buffer, user_data, &c);
  if (!surface)
    return;

  if (blur) {
    int blur_optimized = !c->isfloating || blur_xray;
    wlr_scene_buffer_set_backdrop_blur(buffer, 1);
    wlr_scene_buffer_set_backdrop_blur_optimized(buffer, blur_optimized);
    wlr_scene_buffer_set_backdrop_blur_ignore_transparent(
        buffer, blur_ignore_transparent);
  } else {
    wlr_scene_buffer_set_backdrop_blur(buffer, 0);
  }
}

static void iter_xdg_scene_buffers_corner_radius(struct wlr_scene_buffer *buffer,
                                          int sx, int sy,
                                          void *user_data) {
  Client *c;
  struct wlr_surface *surface = iter_xdg_get_surface(buffer, user_data, &c);
  if (!surface)
    return;

  update_buffer_corner_radius(c, buffer);
}

void iter_xdg_scene_buffers_opacity(struct wlr_scene_buffer *buffer, int sx,
                                    int sy, void *user_data) {
  Client *c;
  struct wlr_surface *surface = iter_xdg_get_surface(buffer, user_data, &c);
  if (!surface)
    return;

  if (opacity)
    wlr_scene_buffer_set_opacity(buffer, c->opacity);
}

/* ── Output scene effects (re-applied each frame via arrange_effects) ── */

void apply_output_scene_effects(struct wlr_scene_node *node, Client *c) {
  if (!opacity && !corner_radius)
    return;

  Client *_c;
  struct wlr_surface *surface;
  struct wlr_scene_node *_node;

  if (!node->enabled) {
    return;
  }

  _c = node->data;
  if (_c) {
    c = _c;
  }

  if (node->type == WLR_SCENE_NODE_BUFFER) {
    struct wlr_scene_buffer *buffer = wlr_scene_buffer_from_node(node);

    surface = client_surface_from_buffer(buffer, c);
    if (!surface) {
      return;
    }

    if (opacity) {
      wlr_scene_buffer_set_opacity(buffer, c->opacity);
    }

    update_buffer_corner_radius(c, buffer);
  } else if (node->type == WLR_SCENE_NODE_TREE) {
    struct wlr_scene_tree *tree = wl_container_of(node, tree, node);
    wl_list_for_each(_node, &tree->children, link) {
      apply_output_scene_effects(_node, c);
    }
  }
}

/* ── Shadow ignore list ── */

int in_shadow_ignore_list(const char *str) {
  if (!str)
    return 0;
  for (size_t i = 0; i < shadow_ignore_list_count; i++) {
    if (strstr(str, shadow_ignore_list[i])) {
      return 1;
    }
  }
  return 0;
}

/* ── Corner radius ── */

enum corner_location set_client_corner_location(Client *c) {
  enum corner_location loc = CORNER_LOCATION_ALL;
  if (!c->mon) {
    return loc;
  }
  if (no_radius_when_single && layout_tiling_count(c->mon) == 1)
    return CORNER_LOCATION_NONE;
  if (c->geom.x + corner_radius <= c->mon->m.x) {
    loc &= ~CORNER_LOCATION_LEFT;
  }
  if (c->geom.x + c->geom.width - corner_radius >=
      c->mon->m.x + c->mon->m.width) {
    loc &= ~CORNER_LOCATION_RIGHT;
  }
  if (c->geom.y + corner_radius <= c->mon->m.y) {
    loc &= ~CORNER_LOCATION_TOP;
  }
  if (c->geom.y + c->geom.height - corner_radius >=
      c->mon->m.y + c->mon->m.height) {
    loc &= ~CORNER_LOCATION_BOTTOM;
  }
  return loc;
}

int effective_corner_radius(Client *c) {
  if ((corner_radius_only_floating && !c->isfloating) || c->isfullscreen) {
    return 0;
  }
  return c->corner_radius;
}

/* ── Shadow ── */

void client_set_shadow_blur_sigma(Client *c, int blur_sigma) {
  int radius = effective_corner_radius(c);
  enum corner_location corners = radius ? set_client_corner_location(c)
                                        : CORNER_LOCATION_NONE;
  if (radius && corners == CORNER_LOCATION_NONE)
    radius = 0;
  wlr_scene_shadow_set_blur_sigma(c->shadow, blur_sigma);
  wlr_scene_node_set_position(&c->shadow->node, -blur_sigma, -blur_sigma);
  wlr_scene_shadow_set_size(c->shadow, c->geom.width + blur_sigma * 2,
                            c->geom.height + blur_sigma * 2);
  wlr_scene_shadow_set_clipped_region(
      c->shadow, (struct clipped_region){
          .corner_radius = radius,
          .corners = corners,
          .area = {blur_sigma, blur_sigma, c->geom.width,
                   c->geom.height},
      });
}

void update_client_shadow_color(Client *c, int focused) {
  int has_shadow_enabled = 1;
  const float *color;

  if (!shadow || !c->shadow) {
    return;
  }

  color = focused ? shadow_color_focus : shadow_color;

  if ((shadow_only_floating && !c->isfloating) ||
      in_shadow_ignore_list(client_get_appid(c)) || c->isfullscreen) {
    color = transparent;
    has_shadow_enabled = 0;
  }

  wlr_scene_shadow_set_color(c->shadow, color);
  c->has_shadow_enabled = has_shadow_enabled;

  client_set_shadow_blur_sigma(c, (int)round(focused
                                                  ? shadow_blur_sigma_focus
                                                  : shadow_blur_sigma));
}

/* ── Effects application ── */

void update_client_corner_radius(Client *c) {
  if (corner_radius && c->round_border) {
    int radius = effective_corner_radius(c);
    wlr_scene_rect_set_corner_radius(c->round_border, radius,
                                     radius ? set_client_corner_location(c)
                                            : CORNER_LOCATION_NONE);
  }

  if (corner_radius > 0 && c->scene) {
    wlr_scene_node_for_each_buffer(&c->scene_surface->node,
                                   iter_xdg_scene_buffers_corner_radius, c);
  }
}

void update_client_blur(Client *c) {
  if (c->scene) {
    wlr_scene_node_for_each_buffer(&c->scene_surface->node,
                                   iter_xdg_scene_buffers_blur, c);
  }
}

void update_buffer_corner_radius(Client *c, struct wlr_scene_buffer *buffer) {
  if (!corner_radius) {
    return;
  }

  int radius = effective_corner_radius(c);
  wlr_scene_buffer_set_corner_radius(buffer, radius,
                                     radius ? set_client_corner_location(c)
                                            : CORNER_LOCATION_NONE);
}

void apply_client_decorations(Client *c, int focused) {
  c->corner_radius = corner_radius;
  update_client_corner_radius(c);
  update_client_shadow_color(c, focused);
  update_client_blur(c);
}

void update_client_focus_decorations(Client *c, int focused, int urgent) {
  if (corner_radius > 0 && c->round_border) {
    wlr_scene_rect_set_color(
        c->round_border,
        urgent ? border_color_urgent : (focused ? border_color_focus : border_color));
  }
  if (shadow && c->shadow) {
    client_set_shadow_blur_sigma(
        c, (int)round(focused ? shadow_blur_sigma_focus : shadow_blur_sigma));
    if (c->has_shadow_enabled) {
      wlr_scene_shadow_set_color(c->shadow,
                                 focused ? shadow_color_focus : shadow_color);
    }
  }
  if (opacity) {
    c->opacity = focused ? opacity_active : opacity_inactive;
    wlr_scene_node_for_each_buffer(&c->scene_surface->node,
                                   iter_xdg_scene_buffers_opacity, c);
  }
}

#include <stdlib.h>
#include <string.h>

#include <wayland-server-core.h>
#include <wlr/types/wlr_server_decoration.h>
#include <scenefx/render/fx_renderer/fx_renderer.h>
#include <scenefx/types/wlr_scene.h>

#include "xdg_shell.h"
#include "client.h"
#include "config.h"
#include "decorations.h"
#include "focus.h"
#include "layout.h"
#include "util.h"

static void createnotify(struct wl_listener *listener, void *data);
void mapnotify(struct wl_listener *listener, void *data);
void unmapnotify(struct wl_listener *listener, void *data);
void destroynotify(struct wl_listener *listener, void *data);
static void commitnotify(struct wl_listener *listener, void *data);
static void commitpopup(struct wl_listener *listener, void *data);
static void createpopup(struct wl_listener *listener, void *data);
static void createdecoration(struct wl_listener *listener, void *data);
static void destroydecoration(struct wl_listener *listener, void *data);
static void requestdecorationmode(struct wl_listener *listener, void *data);
void fullscreennotify(struct wl_listener *listener, void *data);
static void maximizenotify(struct wl_listener *listener, void *data);
void updatetitle(struct wl_listener *listener, void *data);
static void urgent(struct wl_listener *listener, void *data);
static void applyrules(Client *c);

static struct wl_listener new_xdg_toplevel = {.notify = createnotify};
static struct wl_listener new_xdg_popup = {.notify = createpopup};
static struct wl_listener new_xdg_decoration = {.notify = createdecoration};
static struct wl_listener request_activate = {.notify = urgent};

extern unsigned int cursor_mode;
extern Client *grabc;

void setfloating(Client *c, int floating);
void setfullscreen(Client *c, int fullscreen);
void setmon(Client *c, Monitor *m, uint32_t newtags);
void resize(Client *c, struct wlr_box geo, int interact);
void motionnotify(uint32_t time, struct wlr_input_device *device, double dx,
                  double dy, double dx_unaccel, double dy_unaccel);
Monitor *xytomon(double x, double y);

/* ----- rule application ----- */

static void applyrules(Client *c) {
  const char *appid, *title;
  uint32_t newtags = 0;
  int i;
  const Rule *r;
  Monitor *mon = selmon, *m;

  appid = client_get_appid(c);
  title = client_get_title(c);

  for (r = rules; r < rules + rules_count; r++) {
    if ((!r->title || strstr(title, r->title)) &&
        (!r->id || strstr(appid, r->id))) {
      c->isfloating = r->isfloating;
      newtags |= r->tags;
      i = 0;
      wl_list_for_each(m, &mons, link) {
        if (r->monitor == i++)
          mon = m;
      }
    }
  }

  c->isfloating |= client_is_float_type(c);
  setmon(c, mon, newtags);
}

/* ----- toplevel lifecycle ----- */

static void createnotify(struct wl_listener *listener, void *data) {
  struct wlr_xdg_toplevel *toplevel = data;
  Client *c = NULL;

  c = toplevel->base->data = ecalloc(1, sizeof(*c));
  c->surface.xdg = toplevel->base;
  c->bw = config.borderpx;

  c->opacity = opacity ? opacity_inactive : 1.0f;
  c->corner_radius = corner_radius;

  LISTEN(&toplevel->base->surface->events.commit, &c->commit, commitnotify);
  LISTEN(&toplevel->base->surface->events.map, &c->map, mapnotify);
  LISTEN(&toplevel->base->surface->events.unmap, &c->unmap, unmapnotify);
  LISTEN(&toplevel->events.destroy, &c->destroy, destroynotify);
  LISTEN(&toplevel->events.request_fullscreen, &c->fullscreen,
         fullscreennotify);
  LISTEN(&toplevel->events.request_maximize, &c->maximize, maximizenotify);
  LISTEN(&toplevel->events.set_title, &c->set_title, updatetitle);
}

static void commitnotify(struct wl_listener *listener, void *data) {
  Client *c = wl_container_of(listener, c, commit);

  if (c->surface.xdg->initial_commit) {
    applyrules(c);
    if (c->mon) {
      client_set_scale(client_surface(c), c->mon->wlr_output->scale);
    }
    setmon(c, NULL, 0);

    wlr_xdg_toplevel_set_wm_capabilities(
        c->surface.xdg->toplevel, WLR_XDG_TOPLEVEL_WM_CAPABILITIES_FULLSCREEN);
    if (c->decoration)
      requestdecorationmode(&c->set_decoration_mode, c->decoration);
    wlr_xdg_toplevel_set_size(c->surface.xdg->toplevel, 0, 0);
    return;
  }

  resize(c, c->geom, (c->isfloating && !c->isfullscreen));

  if (c->resize && c->resize <= c->surface.xdg->current.configure_serial)
    c->resize = 0;
}

void mapnotify(struct wl_listener *listener, void *data) {
  Client *p = NULL;
  Client *w, *c = wl_container_of(listener, c, map);
  Monitor *m;
  int i;

  c->scene = client_surface(c)->data = wlr_scene_tree_create(layers[LyrTile]);
  wlr_scene_node_set_enabled(&c->scene->node, client_is_unmanaged(c));
  c->scene_surface =
      c->type == XDGShell
          ? wlr_scene_xdg_surface_create(c->scene, c->surface.xdg)
          : wlr_scene_subsurface_tree_create(c->scene, client_surface(c));
  c->scene->node.data = c->scene_surface->node.data = c;

  client_get_geometry(c, &c->geom);

  if (client_is_unmanaged(c)) {
    wlr_scene_node_reparent(&c->scene->node, layers[LyrFloat]);
    wlr_scene_node_set_position(&c->scene->node, c->geom.x, c->geom.y);
    client_set_size(c, c->geom.width, c->geom.height);
    if (client_wants_focus(c)) {
      focusclient(c, 1);
      exclusive_focus = c;
    }
    goto unset_fullscreen;
  }

  for (i = 0; i < 4; i++) {
    c->border[i] = wlr_scene_rect_create(
        c->scene, 0, 0, c->isurgent ? border_color_urgent : border_color);
    c->border[i]->node.data = c;
  }

  wlr_scene_node_for_each_buffer(&c->scene_surface->node,
                                 iter_xdg_scene_buffers, c);

  if (corner_radius > 0) {
    c->round_border = wlr_scene_rect_create(
        c->scene, 0, 0, c->isurgent ? border_color_urgent : border_color);
    c->round_border->node.data = c;
    wlr_scene_node_lower_to_bottom(&c->round_border->node);

    for (i = 0; i < 4; i++) {
      wlr_scene_rect_set_color(c->border[i], transparent);
    }
  }

  if (shadow) {
    c->shadow = wlr_scene_shadow_create(c->scene, 0, 0, c->corner_radius,
                                        shadow_blur_sigma, shadow_color);
    wlr_scene_node_lower_to_bottom(&c->shadow->node);
  }

  client_set_tiled(c, WLR_EDGE_TOP | WLR_EDGE_BOTTOM | WLR_EDGE_LEFT |
                          WLR_EDGE_RIGHT);
  c->geom.width += 2 * c->bw;
  c->geom.height += 2 * c->bw;

  layout_insert(c);
  wl_list_insert(&fstack, &c->flink);

  if ((p = client_get_parent(c))) {
    c->isfloating = 1;
    setmon(c, p->mon, p->tags);
  } else {
    applyrules(c);
  }
  printstatus();

  apply_client_decorations(c, 0);

unset_fullscreen:
  m = c->mon ? c->mon : xytomon(c->geom.x, c->geom.y);
  wl_list_for_each(w, layout_tiling_order(), link) {
    if (w != c && w != p && w->isfullscreen && m == w->mon &&
        (w->tags & c->tags))
      setfullscreen(w, 0);
  }
}

void unmapnotify(struct wl_listener *listener, void *data) {
  Client *c = wl_container_of(listener, c, unmap);
  if (c == grabc) {
    cursor_mode = CurNormal;
    grabc = NULL;
  }

  if (client_is_unmanaged(c)) {
    if (c == exclusive_focus) {
      exclusive_focus = NULL;
      focusclient(focustop(selmon), 1);
    }
  } else {
    layout_remove(c);
    setmon(c, NULL, 0);
    wl_list_remove(&c->flink);
  }

  wlr_scene_node_destroy(&c->scene->node);
  printstatus();
  motionnotify(0, NULL, 0, 0, 0, 0);
}

void destroynotify(struct wl_listener *listener, void *data) {
  Client *c = wl_container_of(listener, c, destroy);
  wl_list_remove(&c->destroy.link);
  wl_list_remove(&c->set_title.link);
  wl_list_remove(&c->fullscreen.link);
#ifdef XWAYLAND
  if (c->type != XDGShell) {
    wl_list_remove(&c->activate.link);
    wl_list_remove(&c->associate.link);
    wl_list_remove(&c->configure.link);
    wl_list_remove(&c->dissociate.link);
    wl_list_remove(&c->set_hints.link);
  } else
#endif
  {
    wl_list_remove(&c->commit.link);
    wl_list_remove(&c->map.link);
    wl_list_remove(&c->unmap.link);
    wl_list_remove(&c->maximize.link);
  }
  free(c);
}

/* ----- toplevel events ----- */

void fullscreennotify(struct wl_listener *listener, void *data) {
  Client *c = wl_container_of(listener, c, fullscreen);
  setfullscreen(c, client_wants_fullscreen(c));
}

static void maximizenotify(struct wl_listener *listener, void *data) {
  Client *c = wl_container_of(listener, c, maximize);
  if (c->surface.xdg->initialized &&
      wl_resource_get_version(c->surface.xdg->toplevel->resource) <
          XDG_TOPLEVEL_WM_CAPABILITIES_SINCE_VERSION)
    wlr_xdg_surface_schedule_configure(c->surface.xdg);
}

void updatetitle(struct wl_listener *listener, void *data) {
  Client *c = wl_container_of(listener, c, set_title);
  if (c == focustop(c->mon))
    printstatus();
}

static void urgent(struct wl_listener *listener, void *data) {
  struct wlr_xdg_activation_v1_request_activate_event *event = data;
  Client *c = NULL;
  toplevel_from_wlr_surface(event->surface, &c, NULL);
  if (!c || c == focustop(selmon))
    return;

  c->isurgent = 1;
  printstatus();

  if (client_surface(c)->mapped) {
    client_set_border_color(c, border_color_urgent);

    update_client_focus_decorations(c, 1, 1);
  }
}

/* ----- popup lifecycle ----- */

static void createpopup(struct wl_listener *listener, void *data) {
  struct wlr_xdg_popup *popup = data;
  struct wlr_xdg_surface *parent =
      wlr_xdg_surface_try_from_wlr_surface(popup->parent);
  struct wlr_scene_tree *parent_tree;
  struct wlr_box box;
  LayerSurface *l = NULL;
  Client *c = NULL;
  int type;
  struct wl_listener *popup_listener;

  if (!parent) {
    wlr_xdg_surface_schedule_configure(popup->base);
    return;
  }

  parent_tree = parent->data;

  popup_listener = ecalloc(1, sizeof(*popup_listener));

  popup_listener->notify = commitpopup;
  LISTEN(&popup->base->surface->events.commit, popup_listener, commitpopup);

  type = toplevel_from_wlr_surface(popup->base->surface, &c, &l);
  if (!popup->parent || type < 0)
    return;
  popup->base->surface->data =
      wlr_scene_xdg_surface_create(popup->parent->data, popup->base);
  if ((l && !l->mon) || (c && !c->mon)) {
    wlr_xdg_popup_destroy(popup);
    return;
  }
  box = type == LayerShell ? l->mon->m : c->mon->w;
  box.x -= (type == LayerShell ? l->scene->node.x : c->geom.x);
  box.y -= (type == LayerShell ? l->scene->node.y : c->geom.y);
  wlr_xdg_popup_unconstrain_from_box(popup, &box);
  wl_list_remove(&listener->link);
  free(listener);
}

static void commitpopup(struct wl_listener *listener, void *data) {
  struct wlr_surface *surface = data;
  struct wlr_xdg_popup *popup = wlr_xdg_popup_try_from_wlr_surface(surface);
  LayerSurface *l = NULL;
  Client *c = NULL;
  struct wlr_box box;
  int type = -1;

  if (!popup->base->initial_commit)
    return;

  type = toplevel_from_wlr_surface(popup->base->surface, &c, &l);
  if (!popup->parent || type < 0)
    return;
  popup->base->surface->data =
      wlr_scene_xdg_surface_create(popup->parent->data, popup->base);
  if ((l && !l->mon) || (c && !c->mon)) {
    wlr_xdg_popup_destroy(popup);
    return;
  }
  box = type == LayerShell ? l->mon->m : c->mon->w;
  box.x -= (type == LayerShell ? l->scene->node.x : c->geom.x);
  box.y -= (type == LayerShell ? l->scene->node.y : c->geom.y);
  wlr_xdg_popup_unconstrain_from_box(popup, &box);
  wl_list_remove(&listener->link);
  free(listener);
}

/* ----- decoration lifecycle ----- */

static void createdecoration(struct wl_listener *listener, void *data) {
  struct wlr_xdg_toplevel_decoration_v1 *deco = data;
  Client *c = deco->toplevel->base->data;
  c->decoration = deco;

  LISTEN(&deco->events.request_mode, &c->set_decoration_mode,
         requestdecorationmode);
  LISTEN(&deco->events.destroy, &c->destroy_decoration, destroydecoration);

  requestdecorationmode(&c->set_decoration_mode, deco);
}

static void destroydecoration(struct wl_listener *listener, void *data) {
  Client *c = wl_container_of(listener, c, destroy_decoration);
  c->decoration = NULL;
  wl_list_remove(&c->destroy_decoration.link);
  wl_list_remove(&c->set_decoration_mode.link);
}

static void requestdecorationmode(struct wl_listener *listener, void *data) {
  Client *c = wl_container_of(listener, c, set_decoration_mode);
  if (c->surface.xdg->initialized)
    wlr_xdg_toplevel_decoration_v1_set_mode(
        c->decoration, WLR_XDG_TOPLEVEL_DECORATION_V1_MODE_SERVER_SIDE);
}

/* ----- init / cleanup ----- */

void xdg_shell_init(void) {
  xdg_shell = wlr_xdg_shell_create(dpy, 6);
  wl_signal_add(&xdg_shell->events.new_toplevel, &new_xdg_toplevel);
  wl_signal_add(&xdg_shell->events.new_popup, &new_xdg_popup);

  activation = wlr_xdg_activation_v1_create(dpy);
  wl_signal_add(&activation->events.request_activate, &request_activate);

  wlr_server_decoration_manager_set_default_mode(
      wlr_server_decoration_manager_create(dpy),
      WLR_SERVER_DECORATION_MANAGER_MODE_SERVER);
  xdg_decoration_mgr = wlr_xdg_decoration_manager_v1_create(dpy);
  wl_signal_add(&xdg_decoration_mgr->events.new_toplevel_decoration,
                &new_xdg_decoration);
}

void xdg_shell_cleanup(void) {
  wl_list_remove(&new_xdg_toplevel.link);
  wl_list_remove(&new_xdg_popup.link);
  wl_list_remove(&new_xdg_decoration.link);
  wl_list_remove(&request_activate.link);
}

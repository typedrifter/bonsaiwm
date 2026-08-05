/*
 * See LICENSE file for copyright and license details.
 */
#include <getopt.h>
#include <libinput.h>
#include <linux/input-event-codes.h>
#include <math.h>
#include <signal.h>
#include <stdio.h>
#include <stdlib.h>
#include <sys/wait.h>
#include <time.h>
#include <scenefx/render/fx_renderer/fx_renderer.h>
#include <scenefx/types/fx/blur_data.h>
#include <scenefx/types/fx/clipped_region.h>
#include <scenefx/types/fx/corner_location.h>
#include <scenefx/types/wlr_scene.h>
#include <unistd.h>
#include <wayland-server-core.h>
#include <wayland-util.h>
#include <wlr/backend.h>
#include <wlr/backend/libinput.h>
#include <wlr/render/allocator.h>
#include <wlr/render/wlr_renderer.h>
#include <wlr/types/wlr_alpha_modifier_v1.h>
#include <wlr/types/wlr_compositor.h>
#include <wlr/types/wlr_cursor.h>
#include <wlr/types/wlr_cursor_shape_v1.h>
#include <wlr/types/wlr_data_control_v1.h>
#include <wlr/types/wlr_data_device.h>
#include <wlr/types/wlr_drm.h>
#include <wlr/types/wlr_export_dmabuf_v1.h>
#include <wlr/types/wlr_ext_data_control_v1.h>
#include <wlr/types/wlr_fractional_scale_v1.h>
#include <wlr/types/wlr_gamma_control_v1.h>
#include <wlr/types/wlr_idle_inhibit_v1.h>
#include <wlr/types/wlr_idle_notify_v1.h>
#include <wlr/types/wlr_input_device.h>
#include <wlr/types/wlr_keyboard.h>
#include <wlr/types/wlr_keyboard_group.h>
#include <wlr/types/wlr_layer_shell_v1.h>
#include <wlr/types/wlr_linux_dmabuf_v1.h>
#include <wlr/types/wlr_linux_drm_syncobj_v1.h>
#include <wlr/types/wlr_output.h>
#include <wlr/types/wlr_output_layout.h>
#include <wlr/types/wlr_output_management_v1.h>
#include <wlr/types/wlr_output_power_management_v1.h>
#include <wlr/types/wlr_pointer.h>
#include <wlr/types/wlr_pointer_constraints_v1.h>
#include <wlr/types/wlr_presentation_time.h>
#include <wlr/types/wlr_primary_selection.h>
#include <wlr/types/wlr_primary_selection_v1.h>
#include <wlr/types/wlr_relative_pointer_v1.h>
#include <wlr/types/wlr_screencopy_v1.h>
#include <wlr/types/wlr_seat.h>
#include <wlr/types/wlr_server_decoration.h>
#include <wlr/types/wlr_session_lock_v1.h>
#include <wlr/types/wlr_single_pixel_buffer_v1.h>
#include <wlr/types/wlr_subcompositor.h>
#include <wlr/types/wlr_viewporter.h>
#include <wlr/types/wlr_virtual_keyboard_v1.h>
#include <wlr/types/wlr_virtual_pointer_v1.h>
#include <wlr/types/wlr_xcursor_manager.h>
#include <wlr/types/wlr_xdg_activation_v1.h>
#include <wlr/types/wlr_xdg_decoration_v1.h>
#include <wlr/types/wlr_xdg_output_v1.h>
#include <wlr/types/wlr_xdg_shell.h>
#include <wlr/util/log.h>
#include <wlr/util/region.h>
#include <xkbcommon/xkbcommon.h>
#include "ext-protocol/wlr_ext_workspace_v1.h"
#ifdef XWAYLAND
#include <wlr/xwayland.h>
#include <xcb/xcb.h>
#include <xcb/xcb_icccm.h>
#endif

#include "config.h"
#include "bonsaiwm.h"
#include "layout.h"
#include "util.h"
#include "decorations.h"
#include "focus.h"
#include "input.h"
#include "layer_shell.h"
#include "monitor.h"
#include "session_lock.h"
#include "xdg_shell.h"

/* enums (client types + scene layers) live in bonsaiwm.h */

/* function declarations */
int arrange(Monitor *m);
void arrange_effects(void);
void chvt(const Arg *arg);
static void cleanup(void);
static void cleanuplisteners(void);
static void createidleinhibitor(struct wl_listener *listener, void *data);
void defaultgaps(const Arg *arg);
static void destroyidleinhibitor(struct wl_listener *listener, void *data);
void focusclient(Client *c, int lift);
void gpureset(struct wl_listener *listener, void *data);
static void handlesig(int signo);
void incgaps(const Arg *arg);
void incnmaster(const Arg *arg);
void killclient(const Arg *arg);
void monocle(Monitor *m);
void motionnotify(uint32_t time, struct wlr_input_device *device,
                  double sx, double sy, double sx_unaccel,
                  double sy_unaccel);
void moveresize(const Arg *arg);
void pointerfocus(Client *c, struct wlr_surface *surface, double sx, double sy,
                  uint32_t time);
void cursorconstrain(struct wlr_pointer_constraint_v1 *constraint);
void printstatus(void);
void pointerfocus(Client *c, struct wlr_surface *surface, double sx, double sy,
                  uint32_t time);
void cursorconstrain(struct wlr_pointer_constraint_v1 *constraint);
void quit(const Arg *arg);
void resize(Client *c, struct wlr_box geo, int interact);
static void run(char *startup_cmd);
void setfloating(Client *c, int floating);
void setfullscreen(Client *c, int fullscreen);
void setlayout(const Arg *arg);
void setmfact(const Arg *arg);
static void setgaps(int oh, int ov, int ih, int iv);
void setmon(Client *c, Monitor *m, uint32_t newtags);
static void init_foundation(void);
static void init_render(void);
static void init_protocols(void);
static void init_output(void);
static void init_shells(void);
static void init_aux(void);
static void init_input(void);
static void init_xwayland(void);
static void setup(void);
void spawn(const Arg *arg);
void tag(const Arg *arg);
void tagmon(const Arg *arg);
void tile(Monitor *m);
void togglefloating(const Arg *arg);
void togglefullscreen(const Arg *arg);
void togglegaps(const Arg *arg);
void toggletag(const Arg *arg);
void toggleview(const Arg *arg);
void view(const Arg *arg);
Monitor *xytomon(double x, double y);
void xytonode(double x, double y, struct wlr_surface **psurface,
                     Client **pc, LayerSurface **pl, double *nx, double *ny);
void zoom(const Arg *arg);

/* variables */
static pid_t child_pid = -1;
static int log_level = WLR_ERROR;
int locked;
void *exclusive_focus;
struct wl_display *dpy;
struct wl_event_loop *event_loop;
struct wlr_backend *backend;
struct wlr_scene *scene;
struct wlr_scene_tree *layers[NUM_LAYERS];
struct wlr_scene_tree *drag_icon;
struct wlr_renderer *drw;
struct wlr_allocator *alloc;
struct wlr_compositor *compositor;
struct wlr_session *session;

struct wlr_xdg_shell *xdg_shell;
struct wlr_xdg_activation_v1 *activation;
struct wlr_xdg_decoration_manager_v1 *xdg_decoration_mgr;
struct wl_list fstack; /* focus order */
struct wlr_idle_notifier_v1 *idle_notifier;
struct wlr_idle_inhibit_manager_v1 *idle_inhibit_mgr;
struct wlr_cursor *cursor;
struct wlr_xcursor_manager *cursor_mgr;

struct wlr_scene_rect *root_bg;

struct wlr_seat *seat;
KeyboardGroup *kb_group;
unsigned int cursor_mode;
Client *grabc;
int grabcx, grabcy;
struct wlr_pointer_constraints_v1 *pointer_constraints;
struct wlr_pointer_constraint_v1 *active_constraint;
struct wlr_relative_pointer_manager_v1 *relative_pointer_mgr;

struct wlr_output_layout *output_layout;
struct wlr_box sgeom;
struct wl_list mons;
Monitor *selmon;

/* global event handlers */
static struct wl_listener gpu_reset = {.notify = gpureset};
static struct wl_listener new_idle_inhibitor = {.notify = createidleinhibitor};

#ifdef XWAYLAND
static void activatex11(struct wl_listener *listener, void *data);
static void associatex11(struct wl_listener *listener, void *data);
static void configurex11(struct wl_listener *listener, void *data);
static void createnotifyx11(struct wl_listener *listener, void *data);
static void dissociatex11(struct wl_listener *listener, void *data);
static void sethints(struct wl_listener *listener, void *data);
static void xwaylandready(struct wl_listener *listener, void *data);
static struct wl_listener new_xwayland_surface = {.notify = createnotifyx11};
static struct wl_listener xwayland_ready = {.notify = xwaylandready};
static struct wlr_xwayland *xwayland;
#endif

/* attempt to encapsulate suck into one file */
#include "client.h"
#include "ext-protocol/ext-workspace.h"

/* Per-tag layout state, TagState, moved to bonsaiwm.h; the tagstate_*
 * accessors and layout_firsttag live in layout.h. */

/* Seed every per-tag slot with the same layout values.
 * When a monitor is created we have one default layout; all tags start from it
 * so that switching to a tag for the first time feels consistent instead of
 * falling back to zeroed state. */
/* TagState init and clamp have moved to the layout module (layout.c). */

/* function implementations */

void arrange_effects(void) {
  motionnotify(0, NULL, 0, 0, 0, 0);
  checkidleinhibitor(NULL);
}

int arrange(Monitor *m) {
  Client *c;

  if (!m->wlr_output->enabled)
    return 0;

  wl_list_for_each(c, layout_tiling_order(), link) {
    if (c->mon == m) {
      wlr_scene_node_set_enabled(&c->scene->node, VISIBLEON(c, m));
      client_set_suspended(c, !VISIBLEON(c, m));
    }
  }

  wlr_scene_node_set_enabled(&m->fullscreen_bg->node,
                             (c = focustop(m)) && c->isfullscreen);

  wlr_scene_node_set_enabled(&m->blur_layer->node, blur);

  strncpy(m->ltsymbol, layouts[tagstate_layout(m)].symbol, LENGTH(m->ltsymbol));

  void (*arr)(Monitor *) = arrangefn[layouts[tagstate_layout(m)].arrange];

  /* We move all clients (except fullscreen and unmanaged) to LyrTile while
   * in floating layout to avoid "real" floating clients be always on top */
  wl_list_for_each(c, layout_tiling_order(), link) {
    if (c->mon != m || c->scene->node.parent == layers[LyrFS])
      continue;

    wlr_scene_node_reparent(&c->scene->node,
                            (!arr && c->isfloating)  ? layers[LyrTile]
                            : (arr && c->isfloating) ? layers[LyrFloat]
                                                     : c->scene->node.parent);
  }

  if (arr)
    arr(m);
  return arr != NULL;
}

void chvt(const Arg *arg) { wlr_session_change_vt(session, arg->ui); }

void cleanup(void) {
  cleanuplisteners();
#ifdef XWAYLAND
  wlr_xwayland_destroy(xwayland);
  xwayland = NULL;
#endif
  wl_display_destroy_clients(dpy);
  if (child_pid > 0) {
    kill(-child_pid, SIGTERM);
    waitpid(child_pid, NULL, 0);
  }
  wlr_xcursor_manager_destroy(cursor_mgr);

  destroykeyboardgroup(&kb_group->destroy, NULL);

  /* If it's not destroyed manually, it will cause a use-after-free of wlr_seat.
   * Destroy it until it's fixed on the wlroots side */
  wlr_backend_destroy(backend);

  wl_display_destroy(dpy);
  /* Destroy after the wayland display (when the monitors are already destroyed)
     to avoid destroying them with an invalid scene output. */
  wlr_scene_node_destroy(&scene->tree.node);
}

void cleanuplisteners(void) {
  wl_list_remove(&ext_manager_commit_listener.link);
  wl_list_remove(&gpu_reset.link);
  wl_list_remove(&new_idle_inhibitor.link);
  layer_shell_cleanup();
  monitor_cleanup();
  xdg_shell_cleanup();
  input_cleanup();
  session_lock_cleanup();
#ifdef XWAYLAND
  wl_list_remove(&new_xwayland_surface.link);
  wl_list_remove(&xwayland_ready.link);
#endif
}

void createidleinhibitor(struct wl_listener *listener, void *data) {
  struct wlr_idle_inhibitor_v1 *idle_inhibitor = data;
  LISTEN_STATIC(&idle_inhibitor->events.destroy, destroyidleinhibitor);

  checkidleinhibitor(NULL);
}

void destroyidleinhibitor(struct wl_listener *listener, void *data) {
  /* `data` is the wlr_surface of the idle inhibitor being destroyed,
   * at this point the idle inhibitor is still in the list of the manager */
  checkidleinhibitor(wlr_surface_get_root_surface(data));
  wl_list_remove(&listener->link);
  free(listener);
}

void focusclient(Client *c, int lift) {
  struct wlr_surface *old = seat->keyboard_state.focused_surface;
  int unused_lx, unused_ly, old_client_type;
  Client *old_c = NULL;
  LayerSurface *old_l = NULL;

  if (locked)
    return;

  /* Raise client in stacking order if requested */
  if (c && lift)
    wlr_scene_node_raise_to_top(&c->scene->node);

  if (c && client_surface(c) == old) {
    /* Nothing to change focus-wise, but c->mon/c->tags may have moved
     * (e.g. setmon after a drag), so keep the status bar fresh. */
    printstatus();
    return;
  }

  if ((old_client_type = toplevel_from_wlr_surface(old, &old_c, &old_l)) ==
      XDGShell) {
    struct wlr_xdg_popup *popup, *tmp;
    wl_list_for_each_safe(popup, tmp, &old_c->surface.xdg->popups, link)
        wlr_xdg_popup_destroy(popup);
  }

  /* Put the new client atop the focus stack and select its monitor */
  if (c && !client_is_unmanaged(c)) {
    wl_list_remove(&c->flink);
    wl_list_insert(&fstack, &c->flink);
    selmon = c->mon;
    c->isurgent = 0;

    /* Don't change border color if there is an exclusive focus or we are
     * handling a drag operation */
    if (!exclusive_focus && !seat->drag) {
      client_set_border_color(c, border_color_focus);

      update_client_focus_decorations(c, 1, 0);
    }
  }

  /* Deactivate old client if focus is changing */
  if (old && (!c || client_surface(c) != old)) {
    /* If an overlay is focused, don't focus or activate the client,
     * but only update its position in fstack to render its border with
     * border_color_focus and focus it after the overlay is closed. */
    if (old_client_type == LayerShell &&
        wlr_scene_node_coords(&old_l->scene->node, &unused_lx, &unused_ly) &&
        old_l->layer_surface->current.layer >= ZWLR_LAYER_SHELL_V1_LAYER_TOP) {
      return;
    } else if (old_c && old_c == exclusive_focus && client_wants_focus(old_c)) {
      return;
      /* Don't deactivate old client if the new one wants focus, as this causes
       * issues with winecfg and probably other clients */
    } else if (old_c && !client_is_unmanaged(old_c) &&
               (!c || !client_wants_focus(c))) {
      client_set_border_color(old_c, border_color);

      update_client_focus_decorations(old_c, 0, 0);

      client_activate_surface(old, 0);
    }
  }
  printstatus();

  if (!c) {
    /* With no client, all we have left is to clear focus */
    wlr_seat_keyboard_notify_clear_focus(seat);
    return;
  }

  /* Change cursor surface */
  motionnotify(0, NULL, 0, 0, 0, 0);

  /* Have a client, so focus its top-level wlr_surface */
  client_notify_enter(client_surface(c), wlr_seat_get_keyboard(seat));

  /* Activate the new client */
  client_activate_surface(client_surface(c), 1);
}

void gpureset(struct wl_listener *listener, void *data) {
  struct wlr_renderer *old_drw = drw;
  struct wlr_allocator *old_alloc = alloc;
  struct Monitor *m;
  if (!(drw = fx_renderer_create(backend)))
    die("couldn't recreate renderer");

  if (!(alloc = wlr_allocator_autocreate(backend, drw)))
    die("couldn't recreate allocator");

  wl_list_remove(&gpu_reset.link);
  wl_signal_add(&drw->events.lost, &gpu_reset);

  wlr_compositor_set_renderer(compositor, drw);

  wl_list_for_each(m, &mons, link) {
    wlr_output_init_render(m->wlr_output, alloc, drw);
  }

  wlr_allocator_destroy(old_alloc);
  wlr_renderer_destroy(old_drw);
}

void handlesig(int signo) {
  if (signo == SIGCHLD)
    while (waitpid(-1, NULL, WNOHANG) > 0)
      ;
  else if (signo == SIGINT || signo == SIGTERM)
    quit(NULL);
}

void incnmaster(const Arg *arg) {
  if (!arg || !selmon)
    return;
  layout_incnmaster(selmon, arg->i);
  arrange(selmon);
  arrange_effects();
}

/* toggle gaps on/off (Super+0) */
void togglegaps(const Arg *arg) {
  config.enablegaps = !config.enablegaps;
  arrange(selmon);
  arrange_effects();
}

void setgaps(int oh, int ov, int ih, int iv) {
  layout_gaps_set(selmon, oh, ov, ih, iv);
  arrange(selmon);
  arrange_effects();
}

void incgaps(const Arg *arg) {
  setgaps(selmon->gappoh + arg->i, selmon->gappov + arg->i,
          selmon->gappih + arg->i, selmon->gappiv + arg->i);
}

/* Reset to config defaults (Super+Shift+) */
void defaultgaps(const Arg *arg) {
  setgaps(config.gappoh, config.gappov, config.gappih, config.gappiv);
}

void killclient(const Arg *arg) {
  Client *sel = focustop(selmon);
  if (sel)
    client_send_close(sel);
}

void monocle(Monitor *m) {
  /* Compute the placements purely in the layout module, then apply each to
   * the scene via resize(). The status symbol and z-order raise are scene
   * side effects that stay here. */
  size_t n = layout_tiling_count(m);
  struct Placement *p = n ? ecalloc(n, sizeof(*p)) : NULL;
  size_t got = p ? layout_place(m, LAYOUT_MONOCLE, NULL, p, n) : 0;

  for (size_t i = 0; i < got; i++)
    resize(p[i].client, p[i].box, 0);
  if (got)
    snprintf(m->ltsymbol, LENGTH(m->ltsymbol), "[%u]", (unsigned)got);
  Client *c = focustop(m);
  if (c)
    wlr_scene_node_raise_to_top(&c->scene->node);
  free(p);
}

void motionnotify(uint32_t time, struct wlr_input_device *device, double dx,
                  double dy, double dx_unaccel, double dy_unaccel) {
  double sx = 0, sy = 0, sx_confined, sy_confined;
  Client *c = NULL, *w = NULL;
  LayerSurface *l = NULL;
  struct wlr_surface *surface = NULL;
  struct wlr_pointer_constraint_v1 *constraint;

  /* Find the client under the pointer and send the event along. */
  xytonode(cursor->x, cursor->y, &surface, &c, NULL, &sx, &sy);

  if (cursor_mode == CurPressed && !seat->drag &&
      surface != seat->pointer_state.focused_surface &&
      toplevel_from_wlr_surface(seat->pointer_state.focused_surface, &w, &l) >=
          0) {
    c = w;
    surface = seat->pointer_state.focused_surface;
    sx = cursor->x - (l ? l->scene->node.x : w->geom.x);
    sy = cursor->y - (l ? l->scene->node.y : w->geom.y);
  }

  /* time is 0 in internal calls meant to restore pointer focus. */
  if (time) {
    wlr_relative_pointer_manager_v1_send_relative_motion(
        relative_pointer_mgr, seat, (uint64_t)time * 1000, dx, dy, dx_unaccel,
        dy_unaccel);

    wl_list_for_each(constraint, &pointer_constraints->constraints, link)
        cursorconstrain(constraint);

    if (active_constraint && cursor_mode != CurResize &&
        cursor_mode != CurMove) {
      toplevel_from_wlr_surface(active_constraint->surface, &c, NULL);
      if (c &&
          active_constraint->surface == seat->pointer_state.focused_surface) {
        sx = cursor->x - c->geom.x - c->bw;
        sy = cursor->y - c->geom.y - c->bw;
        if (wlr_region_confine(&active_constraint->region, sx, sy, sx + dx,
                               sy + dy, &sx_confined, &sy_confined)) {
          dx = sx_confined - sx;
          dy = sy_confined - sy;
        }

        if (active_constraint->type == WLR_POINTER_CONSTRAINT_V1_LOCKED)
          return;
      }
    }

    wlr_cursor_move(cursor, device, dx, dy);
    wlr_idle_notifier_v1_notify_activity(idle_notifier, seat);

    /* Update selmon (even while dragging a window) */
    if (config.sloppyfocus)
      selmon = xytomon(cursor->x, cursor->y);
  }

  /* Update drag icon's position */
  wlr_scene_node_set_position(&drag_icon->node, (int)round(cursor->x),
                              (int)round(cursor->y));

  /* If we are currently grabbing the mouse, handle and return */
  if (cursor_mode == CurMove) {
    /* Move the grabbed client to the new position. */
    resize(grabc,
           (struct wlr_box){.x = (int)round(cursor->x) - grabcx,
                            .y = (int)round(cursor->y) - grabcy,
                            .width = grabc->geom.width,
                            .height = grabc->geom.height},
           1);
    return;
  } else if (cursor_mode == CurResize) {
    resize(grabc,
           (struct wlr_box){.x = grabc->geom.x,
                            .y = grabc->geom.y,
                            .width = (int)round(cursor->x) - grabc->geom.x,
                            .height = (int)round(cursor->y) - grabc->geom.y},
           1);
    return;
  }

  /* If there's no client surface under the cursor, set the cursor image to a
   * default. This is what makes the cursor image appear when you move it
   * off of a client or over its border. */
  if (!surface && !seat->drag)
    wlr_cursor_set_xcursor(cursor, cursor_mgr, "default");

  pointerfocus(c, surface, sx, sy, time);
}

void moveresize(const Arg *arg) {
  if (cursor_mode != CurNormal && cursor_mode != CurPressed)
    return;
  xytonode(cursor->x, cursor->y, NULL, &grabc, NULL, NULL, NULL);
  if (!grabc || client_is_unmanaged(grabc) || grabc->isfullscreen)
    return;

  /* Float the window and tell motionnotify to grab it */
  setfloating(grabc, 1);
  switch (cursor_mode = arg->ui) {
  case CurMove:
    grabcx = (int)round(cursor->x) - grabc->geom.x;
    grabcy = (int)round(cursor->y) - grabc->geom.y;
    wlr_cursor_set_xcursor(cursor, cursor_mgr, "all-scroll");
    break;
  case CurResize:
    /* Doesn't work for X11 output - the next absolute motion event
     * returns the cursor to where it started */
    wlr_cursor_warp_closest(cursor, NULL, grabc->geom.x + grabc->geom.width,
                            grabc->geom.y + grabc->geom.height);
    wlr_cursor_set_xcursor(cursor, cursor_mgr, "se-resize");
    break;
  }
}

void printstatus(void) {
void pointerfocus(Client *c, struct wlr_surface *surface, double sx, double sy,
                  uint32_t time);
void cursorconstrain(struct wlr_pointer_constraint_v1 *constraint);
  Monitor *m = NULL;
  Client *c;
  uint32_t occ, urg, sel;

  wl_list_for_each(m, &mons, link) {
    occ = urg = 0;
    wl_list_for_each(c, layout_tiling_order(), link) {
      if (c->mon != m)
        continue;
      occ |= c->tags;
      if (c->isurgent)
        urg |= c->tags;
    }
    if ((c = focustop(m))) {
      printf("%s title %s\n", m->wlr_output->name, client_get_title(c));
      printf("%s appid %s\n", m->wlr_output->name, client_get_appid(c));
      printf("%s fullscreen %d\n", m->wlr_output->name, c->isfullscreen);
      printf("%s floating %d\n", m->wlr_output->name, c->isfloating);
      sel = c->tags;
    } else {
      printf("%s title \n", m->wlr_output->name);
      printf("%s appid \n", m->wlr_output->name);
      printf("%s fullscreen \n", m->wlr_output->name);
      printf("%s floating \n", m->wlr_output->name);
      sel = 0;
    }

    printf("%s gaps %u %u %u %u\n", m->wlr_output->name, m->gappoh, m->gappov,
           m->gappih, m->gappiv);
    printf("%s gap.smart %u\n", m->wlr_output->name, config.smartgaps);
    printf("%s selmon %u\n", m->wlr_output->name, m == selmon);
    printf("%s tags %" PRIu32 " %" PRIu32 " %" PRIu32 " %" PRIu32 "\n",
           m->wlr_output->name, occ, m->tagset[m->seltags], sel, urg);
    printf("%s layout %s\n", m->wlr_output->name, m->ltsymbol);
    ext_workspace_printstatus(m, occ, urg);
  }
  fflush(stdout);
}

void quit(const Arg *arg) { wl_display_terminate(dpy); }

void resize(Client *c, struct wlr_box geo, int interact) {
  struct wlr_box *bbox;
  struct wlr_box clip;

  if (!c->mon || !client_surface(c)->mapped)
    return;

  bbox = interact ? &sgeom : &c->mon->w;

  client_set_bounds(c, geo.width, geo.height);
  c->geom = geo;
  layout_bounds(&c->geom, bbox, c->bw);

  /* Update scene-graph, including borders */
  unsigned int border_smart_eff =
      (border_smart && layout_tiling_count(c->mon) == 1) ? 0 : c->bw;
  wlr_scene_node_set_position(&c->scene->node, c->geom.x, c->geom.y);
  wlr_scene_node_set_position(&c->scene_surface->node, border_smart_eff,
                              border_smart_eff);
  wlr_scene_rect_set_size(c->border[0], c->geom.width, border_smart_eff);
  wlr_scene_rect_set_size(c->border[1], c->geom.width, border_smart_eff);
  wlr_scene_rect_set_size(c->border[2], border_smart_eff,
                          c->geom.height - 2 * border_smart_eff);
  wlr_scene_rect_set_size(c->border[3], border_smart_eff,
                          c->geom.height - 2 * border_smart_eff);
  wlr_scene_node_set_position(&c->border[1]->node, 0,
                              c->geom.height - border_smart_eff);
  wlr_scene_node_set_position(&c->border[2]->node, 0, border_smart_eff);
  wlr_scene_node_set_position(&c->border[3]->node,
                              c->geom.width - border_smart_eff,
                              border_smart_eff);

  /* this is a no-op if size hasn't changed */
  c->resize = client_set_size(c, c->geom.width - 2 * border_smart_eff,
                              c->geom.height - 2 * border_smart_eff);
  unsigned int saved_bw = c->bw;
  c->bw = border_smart_eff;
  client_get_clip(c, &clip);
  c->bw = saved_bw;
  wlr_scene_subsurface_tree_set_clip(&c->scene_surface->node, &clip);

  if (corner_radius > 0 && c->round_border) {
    int radius = effective_corner_radius(c);
    enum corner_location corners = radius ? set_client_corner_location(c)
                                          : CORNER_LOCATION_NONE;
    if (radius && corners == CORNER_LOCATION_NONE)
      radius = 0;
    wlr_scene_node_set_position(&c->round_border->node, 0, 0);
    wlr_scene_rect_set_size(c->round_border, c->geom.width, c->geom.height);
    wlr_scene_rect_set_clipped_region(
        c->round_border, (struct clipped_region){
                             .corner_radius = radius,
                             .corners = corners,
                             .area = {c->bw, c->bw,
                                      c->geom.width - c->bw * 2,
                                      c->geom.height - c->bw * 2},
                         });
  }

  if (shadow && c->shadow) {
    client_set_shadow_blur_sigma(c, (int)round(c->shadow->blur_sigma));
  }

  update_client_corner_radius(c);
}

void run(char *startup_cmd) {
  /* Add a Unix socket to the Wayland display. */
  const char *socket = wl_display_add_socket_auto(dpy);
  if (!socket)
    die("startup: display_add_socket_auto");
  setenv("WAYLAND_DISPLAY", socket, 1);

  load_config();

  /* Start the backend. This will enumerate outputs and inputs, become the DRM
   * master, etc */
  if (!wlr_backend_start(backend))
    die("startup: backend_start");

  /* Now that the socket exists and the backend is started, run the startup
   * command */
  if (startup_cmd) {
    int piperw[2];
    if (pipe(piperw) < 0)
      die("startup: pipe:");
    if ((child_pid = fork()) < 0)
      die("startup: fork:");
    if (child_pid == 0) {
      setsid();
      dup2(piperw[0], STDIN_FILENO);
      close(piperw[0]);
      close(piperw[1]);
      execl("/bin/sh", "/bin/sh", "-c", startup_cmd, NULL);
      die("startup: execl:");
    }
    dup2(piperw[1], STDOUT_FILENO);
    close(piperw[1]);
    close(piperw[0]);
  }

  /* Mark stdout as non-blocking to avoid the startup script
   * causing bonsaiwm to freeze when a user neither closes stdin
   * nor consumes standard input in his startup script */

  if (fd_set_nonblock(STDOUT_FILENO) < 0)
    close(STDOUT_FILENO);

  printstatus();

  /* At this point the outputs are initialized, choose initial selmon based on
   * cursor position, and set default cursor image */
  selmon = xytomon(cursor->x, cursor->y);

  /* TODO hack to get cursor to display in its initial location (100, 100)
   * instead of (0, 0) and then jumping. Still may not be fully
   * initialized, as the image/coordinates are not transformed for the
   * monitor when displayed here */
  wlr_cursor_warp_closest(cursor, NULL, cursor->x, cursor->y);
  wlr_cursor_set_xcursor(cursor, cursor_mgr, "default");

  /* Run the Wayland event loop. This does not return until you exit the
   * compositor. Starting the backend rigged up all of the necessary event
   * loop configuration to listen to libinput events, DRM events, generate
   * frame events at the refresh rate, and so on. */
  wl_display_run(dpy);
}

void setfloating(Client *c, int floating) {
  Client *p = client_get_parent(c);
  c->isfloating = floating;

  apply_client_decorations(c, focustop(c->mon) == c);

  /* If in floating layout do not change the client's layer */
  if (!c->mon || !client_surface(c)->mapped ||
      !arrangefn[layouts[tagstate_layout(c->mon)].arrange])
    return;
  wlr_scene_node_reparent(
      &c->scene->node, layers[c->isfullscreen || (p && p->isfullscreen) ? LyrFS
                              : c->isfloating ? LyrFloat
                                              : LyrTile]);
  arrange(c->mon);
  printstatus();
}

void setfullscreen(Client *c, int fullscreen) {
  c->isfullscreen = fullscreen;
  if (!c->mon || !client_surface(c)->mapped)
    return;
  c->bw = fullscreen ? 0 : config.borderpx;
  client_set_fullscreen(c, fullscreen);
  wlr_scene_node_reparent(&c->scene->node, layers[c->isfullscreen ? LyrFS
                                                  : c->isfloating ? LyrFloat
                                                                  : LyrTile]);

  if (fullscreen) {
    c->prev = c->geom;
    resize(c, c->mon->m, 0);
  } else {
    /* restore previous size instead of arrange for floating windows since
     * client positions are set by the user and cannot be recalculated */
    resize(c, c->prev, 0);
  }

  apply_client_decorations(c, focustop(c->mon) == c);

  arrange(c->mon);
  printstatus();
}

void setlayout(const Arg *arg) {
  int slot;

  if (!selmon)
    return;
  if (!arg || arg->i < 0 || arg->i != tagstate_layout(selmon))
    slot = layout_setlayout_toggle(selmon);
  else
    slot = (int)tagstate_sellt(selmon);
  if (arg && arg->i >= 0 && (size_t)arg->i < layouts_count)
    layout_setlayout_idx(selmon, arg->i, slot);
  strncpy(selmon->ltsymbol, layouts[tagstate_lt(selmon, slot)].symbol,
          LENGTH(selmon->ltsymbol));
  arrange(selmon);
  arrange_effects();
  printstatus();
}

void reload_monitor_layouts(void) {
  Monitor *m;

  wlr_log(WLR_DEBUG, "reload_monitor_layouts: layouts_count=%zu",
          layouts_count);
  if (layouts_count == 0) {
    wlr_log(WLR_DEBUG, "reload_monitor_layouts: no layouts, skipping monitors");
    return;
  }
  wl_list_for_each(m, &mons, link) {
    int old0 = tagstate_lt(m, 0), old1 = tagstate_lt(m, 1);
    unsigned int oldsellt = tagstate_sellt(m);
    wlr_log(WLR_DEBUG, "reload_monitor_layouts: monitor %s lt=[%d,%d] sellt=%u",
            m->wlr_output->name, old0, old1, oldsellt);
    layout_tagstate_clamp(m->tagstate, layouts_count);
    if (old0 == tagstate_lt(m, 0) && old1 == tagstate_lt(m, 1) &&
        oldsellt == tagstate_sellt(m))
      wlr_log(WLR_DEBUG,
              "reload_monitor_layouts: %s layouts already valid, no change",
              m->wlr_output->name);
    else
      wlr_log(WLR_DEBUG,
              "reload_monitor_layouts: %s layouts fixed -> lt=[%d,%d] sellt=%u",
              m->wlr_output->name, tagstate_lt(m, 0), tagstate_lt(m, 1),
              tagstate_sellt(m));
    layout_gaps_set(m, config.gappoh, config.gappov, config.gappih, config.gappiv);
    strncpy(m->ltsymbol, layouts[tagstate_layout(m)].symbol,
            LENGTH(m->ltsymbol));
    arrange(m);
    arrange_effects();
  }
}

/* rebuild the xkb keymap from the (possibly reloaded) xkb_rules and re-push
 * it onto the keyboard group, along with the repeat settings. Called from
 * load_config() on every Mod-Shift-R reload. On first startup it's a no-op
 * because kb_group doesn't exist yet — createkeyboardgroup() picks up the
 * already-populated xkb_rules when it runs. wlr_keyboard_set_keymap is safe
 * to call on an already-keymapped keyboard; it replaces the keymap and every
 * attached keyboard inherits the new group keymap. A keymap compile failure
 * (e.g. a typo in options = "ctrl:nocapsss") is logged but non-fatal: the
 * previous keymap is kept so the user can fix their config and retry. */
void reload_keyboard(void) {
  if (!kb_group)
    return;

  struct xkb_context *ctx = xkb_context_new(XKB_CONTEXT_NO_FLAGS);
  if (!ctx) {
    wlr_log(WLR_ERROR, "reload_keyboard: xkb_context_new failed");
    return;
  }

  struct xkb_keymap *keymap = xkb_keymap_new_from_names(
      ctx, &xkb_rules, XKB_KEYMAP_COMPILE_NO_FLAGS);
  if (!keymap) {
    wlr_log(WLR_ERROR,
            "reload_keyboard: failed to compile keymap, keeping previous");
    xkb_context_unref(ctx);
    return;
  }

  wlr_keyboard_set_keymap(&kb_group->wlr_group->keyboard, keymap);
  xkb_keymap_unref(keymap);
  xkb_context_unref(ctx);

  /* re-push repeat settings from the (possibly reloaded) config struct.
   * without this, changing repeat_rate/repeat_delay in lua and reloading
   * would have no effect on the live keyboard. */
  wlr_keyboard_set_repeat_info(&kb_group->wlr_group->keyboard,
                               config.repeat_rate, config.repeat_delay);
}

/* re-push blur_data to the live scene after a config reload. blur_data is
 * otherwise only applied once at scene creation (see main()), so without this
 * a lua change to bonsaiwm.blur params would need a full restart to
 * take effect. Safe to call before the scene exists (first startup runs
 * load_config before main builds the scene) — the NULL guard handles that. */
void reload_blur(void) {
  if (!scene || !blur)
    return;
  wlr_scene_set_blur_data(scene, blur_data.num_passes, blur_data.radius,
                          blur_data.noise, blur_data.brightness,
                          blur_data.contrast, blur_data.saturation);
}

/* re-apply decorations to every mapped client after a config reload,
 * so value tweaks (corner radii, shadow colors/blur sigma, blur, opacity) and
 * structural master-flag toggles (shadow/corner_radius/blur on-off) take
 * effect on already-mapped windows without re-creating them. */
void reload_decorations(void) {
  Client *c;
  wl_list_for_each(c, layout_tiling_order(), link) {
    c->corner_radius = corner_radius;

    if (client_is_unmanaged(c))
      continue;

    /* The per-client scenefx nodes are normally born once at map time, so a
     * master-flag toggle only reached new windows. Rebuild them here to honor
     * toggles for existing clients. */
    if (shadow && !c->shadow) {
      c->shadow = wlr_scene_shadow_create(c->scene, 0, 0, c->corner_radius,
                                          shadow_blur_sigma, shadow_color);
      wlr_scene_node_lower_to_bottom(&c->shadow->node);
    } else if (!shadow && c->shadow) {
      wlr_scene_node_destroy(&c->shadow->node);
      c->shadow = NULL;
    }

    if (corner_radius > 0 && !c->round_border) {
      int i;
      int radius = effective_corner_radius(c);
      enum corner_location corners = radius ? set_client_corner_location(c)
                                            : CORNER_LOCATION_NONE;
      if (radius && corners == CORNER_LOCATION_NONE)
        radius = 0;
      c->round_border = wlr_scene_rect_create(
          c->scene, 0, 0,
          c->isurgent ? border_color_urgent
                      : focustop(c->mon) == c ? border_color_focus : border_color);
      c->round_border->node.data = c;
      wlr_scene_node_lower_to_bottom(&c->round_border->node);
      wlr_scene_node_set_position(&c->round_border->node, 0, 0);
      wlr_scene_rect_set_size(c->round_border, c->geom.width, c->geom.height);
      wlr_scene_rect_set_clipped_region(
          c->round_border, (struct clipped_region){
                              .corner_radius = radius,
                              .corners = corners,
                              .area = {c->bw, c->bw,
                                       c->geom.width - c->bw * 2,
                                       c->geom.height - c->bw * 2},
                          });
      for (i = 0; i < 4; i++)
        wlr_scene_rect_set_color(c->border[i], transparent);
    } else if (corner_radius == 0 && c->round_border) {
      wlr_scene_node_destroy(&c->round_border->node);
      c->round_border = NULL;
      client_set_border_color(c, c->isurgent ? border_color_urgent
                          : focustop(c->mon) == c ? border_color_focus : border_color);
    }

    apply_client_decorations(c, focustop(c->mon) == c);
    if (opacity) {
      c->opacity = (focustop(c->mon) == c) ? opacity_active : opacity_inactive;
      wlr_scene_node_for_each_buffer(&c->scene_surface->node,
                                     iter_xdg_scene_buffers_opacity, c);
    }
  }
}

/* arg > 1.0 will set mfact absolutely */
void setmfact(const Arg *arg) {
  float f;

  if (!arg || !selmon || !arrangefn[layouts[tagstate_layout(selmon)].arrange])
    return;
  f = arg->f < 1.0f ? arg->f + tagstate_mfact(selmon) : arg->f - 1.0f;
  if (f < 0.1 || f > 0.9)
    return;
  layout_setmfact(selmon, f);
  arrange(selmon);
  arrange_effects();
}

void setmon(Client *c, Monitor *m, uint32_t newtags) {
  Monitor *oldmon = c->mon;

  if (oldmon == m)
    return;
  c->mon = m;
  c->prev = c->geom;

  /* Scene graph sends surface leave/enter events on move and resize */
  if (oldmon)
    arrange(oldmon);
  arrange_effects();
  if (m) {
    /* Make sure window actually overlaps with the monitor */
    resize(c, c->geom, 0);
    c->tags = newtags
                  ? newtags
                  : m->tagset[m->seltags]; /* assign tags of target monitor */
    setfullscreen(c, c->isfullscreen);     /* This will call arrange(c->mon) */
    setfloating(c, c->isfloating);
  }
  focusclient(focustop(selmon), 1);
}

static void init_foundation(void) {
  int i, sig[] = {SIGCHLD, SIGINT, SIGTERM, SIGPIPE};
  struct sigaction sa = {.sa_flags = SA_RESTART, .sa_handler = handlesig};
  sigemptyset(&sa.sa_mask);

  for (i = 0; i < (int)LENGTH(sig); i++)
    sigaction(sig[i], &sa, NULL);

  wlr_log_init(log_level, NULL);

  /* The Wayland display is managed by libwayland. It handles accepting
   * clients from the Unix socket, managing Wayland globals, and so on. */
  dpy = wl_display_create();
  event_loop = wl_display_get_event_loop(dpy);

  /* The backend is a wlroots feature which abstracts the underlying input and
   * output hardware. The autocreate option will choose the most suitable
   * backend based on the current environment, such as opening an X11 window
   * if an X11 server is running. */
  if (!(backend = wlr_backend_autocreate(event_loop, &session)))
    die("couldn't create backend");
}

static void init_render(void) {
  int drm_fd, i;

  /* Initialize the scene graph used to lay out windows */
  scene = wlr_scene_create();
  root_bg = wlr_scene_rect_create(&scene->tree, 0, 0, background);
  for (i = 0; i < NUM_LAYERS; i++)
    layers[i] = wlr_scene_tree_create(&scene->tree);
  drag_icon = wlr_scene_tree_create(&scene->tree);
  wlr_scene_node_place_below(&drag_icon->node, &layers[LyrBlock]->node);

  if (blur) {
    wlr_scene_set_blur_data(scene, blur_data.num_passes, blur_data.radius,
                            blur_data.noise, blur_data.brightness,
                            blur_data.contrast, blur_data.saturation);
  }

  /* Autocreates a renderer, either Pixman, GLES2 or Vulkan for us. The user
   * can also specify a renderer using the WLR_RENDERER env var.
   * The renderer is responsible for defining the various pixel formats it
   * supports for shared memory, this configures that for clients. */
  if (!(drw = fx_renderer_create(backend)))
    die("couldn't create renderer");
  wl_signal_add(&drw->events.lost, &gpu_reset);

  /* Create shm, drm and linux_dmabuf interfaces by ourselves.
   * The simplest way is to call:
   *      wlr_renderer_init_wl_display(drw);
   * but we need to create the linux_dmabuf interface manually to integrate it
   * with wlr_scene. */
  wlr_renderer_init_wl_shm(drw, dpy);

  if (wlr_renderer_get_texture_formats(drw, WLR_BUFFER_CAP_DMABUF)) {
    wlr_drm_create(dpy, drw);
    wlr_scene_set_linux_dmabuf_v1(
        scene, wlr_linux_dmabuf_v1_create_with_renderer(dpy, 5, drw));
  }

  if ((drm_fd = wlr_renderer_get_drm_fd(drw)) >= 0 && drw->features.timeline &&
      backend->features.timeline)
    wlr_linux_drm_syncobj_manager_v1_create(dpy, 1, drm_fd);

  /* Autocreates an allocator for us.
   * The allocator is the bridge between the renderer and the backend. It
   * handles the buffer creation, allowing wlroots to render onto the
   * screen */
  if (!(alloc = wlr_allocator_autocreate(backend, drw)))
    die("couldn't create allocator");
}

static void init_protocols(void) {
  compositor = wlr_compositor_create(dpy, 6, drw);
  wlr_subcompositor_create(dpy);
  wlr_data_device_manager_create(dpy);
  wlr_export_dmabuf_manager_v1_create(dpy);
  wlr_screencopy_manager_v1_create(dpy);
  wlr_data_control_manager_v1_create(dpy);
  wlr_ext_data_control_manager_v1_create(dpy, 1);
  wlr_primary_selection_v1_device_manager_create(dpy);
  wlr_viewporter_create(dpy);
  wlr_single_pixel_buffer_manager_v1_create(dpy);
  wlr_fractional_scale_manager_v1_create(dpy, 1);
  wlr_presentation_create(dpy, backend, 2);
  wlr_alpha_modifier_v1_create(dpy);
}

static void init_output(void) {
  monitor_init();
}

static void init_shells(void) {
  /* Set up our client lists, the xdg-shell and the layer-shell. The xdg-shell
   * is a Wayland protocol which is used for application windows. For more
   * detail on shells, refer to the article:
   *
   * https://drewdevault.com/2018/07/29/Wayland-shells.html
   */
  layout_init();
  wl_list_init(&fstack);

  xdg_shell_init();

  layer_shell_init();
}

static void init_aux(void) {
  idle_notifier = wlr_idle_notifier_v1_create(dpy);

  idle_inhibit_mgr = wlr_idle_inhibit_v1_create(dpy);
  wl_signal_add(&idle_inhibit_mgr->events.new_inhibitor, &new_idle_inhibitor);

  session_lock_init();
}

static void init_input(void) {
  input_init();
}

static void init_xwayland(void) {
  /* Make sure XWayland clients don't connect to the parent X server,
   * e.g when running in the x11 backend or the wayland backend and the
   * compositor has Xwayland support */
  unsetenv("DISPLAY");
#ifdef XWAYLAND
  /*
   * Initialise the XWayland X server.
   * It will be started when the first X client is started.
   */
  if ((xwayland = wlr_xwayland_create(dpy, compositor, 1))) {
    wl_signal_add(&xwayland->events.ready, &xwayland_ready);
    wl_signal_add(&xwayland->events.new_surface, &new_xwayland_surface);

    setenv("DISPLAY", xwayland->display_name, 1);
  } else {
    fprintf(stderr,
            "failed to setup XWayland X server, continuing without it\n");
  }
#endif
}

void setup(void) {
  init_foundation();
  init_render();
  init_protocols();
  init_output();
  init_shells();
  init_aux();
  init_input();
  init_xwayland();
}

void spawn(const Arg *arg) {
  if (fork() == 0) {
    dup2(STDERR_FILENO, STDOUT_FILENO);
    setsid();
    execvp(((char **)arg->v)[0], (char **)arg->v);
    die("bonsaiwm: execvp %s failed:", ((char **)arg->v)[0]);
  }
}

void tag(const Arg *arg) {
  Client *sel = focustop(selmon);
  if (!sel || (arg->ui & TAGMASK) == 0)
    return;

  sel->tags = arg->ui & TAGMASK;
  focusclient(focustop(selmon), 1);
  arrange(selmon);
  arrange_effects();
  printstatus();
}

void tagmon(const Arg *arg) {
  Client *sel = focustop(selmon);
  if (sel)
    setmon(sel, dirtomon(arg->i), 0);
}

void tile(Monitor *m) {
  /* Compute the placements purely in the layout module, then apply each to
   * the scene via resize(). */
  struct layout_opts opts = {.enablegaps = config.enablegaps,
                             .smartgaps = config.smartgaps};
  size_t n = layout_tiling_count(m);
  struct Placement *p = n ? ecalloc(n, sizeof(*p)) : NULL;
  size_t got = p ? layout_place(m, LAYOUT_TILE, &opts, p, n) : 0;

  for (size_t i = 0; i < got; i++)
    resize(p[i].client, p[i].box, 0);
  free(p);
}

void togglefloating(const Arg *arg) {
  Client *sel = focustop(selmon);
  /* return if fullscreen */
  if (sel && !sel->isfullscreen)
    setfloating(sel, !sel->isfloating);
}

void togglefullscreen(const Arg *arg) {
  Client *sel = focustop(selmon);
  if (sel)
    setfullscreen(sel, !sel->isfullscreen);
}

void toggletag(const Arg *arg) {
  uint32_t newtags;
  Client *sel = focustop(selmon);
  if (!sel || !(newtags = sel->tags ^ (arg->ui & TAGMASK)))
    return;

  sel->tags = newtags;
  focusclient(focustop(selmon), 1);
  arrange(selmon);
  arrange_effects();
  printstatus();
}

void toggleview(const Arg *arg) {
  uint32_t newtagset;
  if (!selmon || !(newtagset = selmon->tagset[selmon->seltags] ^ (arg->ui & TAGMASK)))
    return;
  layout_view_toggle(selmon, arg->ui & TAGMASK);
  focusclient(focustop(selmon), 1);
  arrange(selmon);
  arrange_effects();
  printstatus();
}

void view(const Arg *arg) {
  uint32_t tags = arg->ui & TAGMASK;
  if (!selmon || tags == selmon->tagset[selmon->seltags])
    return;
  layout_view_set(selmon, tags);
  focusclient(focustop(selmon), 1);
  arrange(selmon);
  arrange_effects();
  printstatus();
}

Monitor *xytomon(double x, double y) {
  struct wlr_output *o = wlr_output_layout_output_at(output_layout, x, y);
  return o ? o->data : NULL;
}

void xytonode(double x, double y, struct wlr_surface **psurface, Client **pc,
              LayerSurface **pl, double *nx, double *ny) {
  struct wlr_scene_node *node, *pnode;
  struct wlr_surface *surface = NULL;
  Client *c = NULL;
  LayerSurface *l = NULL;
  int layer;

  for (layer = NUM_LAYERS - 1; !surface && layer >= 0; layer--) {
    if (!(node = wlr_scene_node_at(&layers[layer]->node, x, y, nx, ny)))
      continue;

    if (node->type == WLR_SCENE_NODE_BUFFER)
      surface =
          wlr_scene_surface_try_from_buffer(wlr_scene_buffer_from_node(node))
              ->surface;
    /* Walk the tree to find a node that knows the client */
    for (pnode = node; pnode && !c; pnode = &pnode->parent->node)
      c = pnode->data;
    if (c && c->type == LayerShell) {
      c = NULL;
      l = pnode->data;
    }
  }

  if (psurface)
    *psurface = surface;
  if (pc)
    *pc = c;
  if (pl)
    *pl = l;
}

void zoom(const Arg *arg) {
  Client *c, *sel = focustop(selmon);

  if (!sel || !selmon ||
      !arrangefn[layouts[tagstate_layout(selmon)].arrange] || sel->isfloating)
    return;

  /* Search for the first tiled window that is not sel, marking sel as
   * NULL if we pass it along the way */
  wl_list_for_each(c, layout_tiling_order(), link) {
    if (VISIBLEON(c, selmon) && !c->isfloating) {
      if (c != sel)
        break;
      sel = NULL;
    }
  }

  /* Return if no other tiled window was found */
  if (&c->link == layout_tiling_order())
    return;

  /* If we passed sel, move c to the front; otherwise, move sel to the
   * front */
  if (!sel)
    sel = c;
  layout_promote(sel);

  focusclient(sel, 1);
  arrange(selmon);
  arrange_effects();
}

#ifdef XWAYLAND
void activatex11(struct wl_listener *listener, void *data) {
  Client *c = wl_container_of(listener, c, activate);

  /* Only "managed" windows can be activated */
  if (!client_is_unmanaged(c))
    wlr_xwayland_surface_activate(c->surface.xwayland, 1);
}

void associatex11(struct wl_listener *listener, void *data) {
  Client *c = wl_container_of(listener, c, associate);

  LISTEN(&client_surface(c)->events.map, &c->map, mapnotify);
  LISTEN(&client_surface(c)->events.unmap, &c->unmap, unmapnotify);
}

void configurex11(struct wl_listener *listener, void *data) {
  Client *c = wl_container_of(listener, c, configure);
  struct wlr_xwayland_surface_configure_event *event = data;
  if (!client_surface(c) || !client_surface(c)->mapped) {
    wlr_xwayland_surface_configure(c->surface.xwayland, event->x, event->y,
                                   event->width, event->height);
    return;
  }
  if (client_is_unmanaged(c)) {
    wlr_scene_node_set_position(&c->scene->node, event->x, event->y);
    wlr_xwayland_surface_configure(c->surface.xwayland, event->x, event->y,
                                   event->width, event->height);
    return;
  }
  if ((c->isfloating && c != grabc) ||
      !arrangefn[layouts[tagstate_layout(c->mon)].arrange]) {
    resize(c,
           (struct wlr_box){.x = event->x - c->bw,
                            .y = event->y - c->bw,
                            .width = event->width + c->bw * 2,
                            .height = event->height + c->bw * 2},
           0);
  } else {
    arrange(c->mon);
    arrange_effects();
  }
}

void createnotifyx11(struct wl_listener *listener, void *data) {
  struct wlr_xwayland_surface *xsurface = data;
  Client *c;

  /* Allocate a Client for this surface */
  c = xsurface->data = ecalloc(1, sizeof(*c));
  c->surface.xwayland = xsurface;
  c->type = X11;
  c->bw = client_is_unmanaged(c) ? 0 : config.borderpx;

  c->opacity = opacity ? opacity_inactive : 1.0f;
  c->corner_radius = corner_radius;

  /* Listen to the various events it can emit */
  LISTEN(&xsurface->events.associate, &c->associate, associatex11);
  LISTEN(&xsurface->events.destroy, &c->destroy, destroynotify);
  LISTEN(&xsurface->events.dissociate, &c->dissociate, dissociatex11);
  LISTEN(&xsurface->events.request_activate, &c->activate, activatex11);
  LISTEN(&xsurface->events.request_configure, &c->configure, configurex11);
  LISTEN(&xsurface->events.request_fullscreen, &c->fullscreen,
         fullscreennotify);
  LISTEN(&xsurface->events.set_hints, &c->set_hints, sethints);
  LISTEN(&xsurface->events.set_title, &c->set_title, updatetitle);
}

void dissociatex11(struct wl_listener *listener, void *data) {
  Client *c = wl_container_of(listener, c, dissociate);
  wl_list_remove(&c->map.link);
  wl_list_remove(&c->unmap.link);
}

void sethints(struct wl_listener *listener, void *data) {
  Client *c = wl_container_of(listener, c, set_hints);
  struct wlr_surface *surface = client_surface(c);
  if (c == focustop(selmon) || !c->surface.xwayland->hints)
    return;

  c->isurgent = xcb_icccm_wm_hints_get_urgency(c->surface.xwayland->hints);
  printstatus();

  if (c->isurgent && surface && surface->mapped) {
    client_set_border_color(c, border_color_urgent);

    update_client_focus_decorations(c, 1, 1);
  }
}

void xwaylandready(struct wl_listener *listener, void *data) {
  struct wlr_xcursor *xcursor;

  /* assign the one and only seat */
  wlr_xwayland_set_seat(xwayland, seat);

  /* Set the default XWayland cursor to match the rest of bonsaiwm. */
  if ((xcursor = wlr_xcursor_manager_get_xcursor(cursor_mgr, "default", 1)))
    wlr_xwayland_set_cursor(
        xwayland, xcursor->images[0]->buffer, xcursor->images[0]->width * 4,
        xcursor->images[0]->width, xcursor->images[0]->height,
        xcursor->images[0]->hotspot_x, xcursor->images[0]->hotspot_y);
}
#endif

int main(int argc, char *argv[]) {
  char *startup_cmd = NULL;
  int c;

  while ((c = getopt(argc, argv, "s:hdv")) != -1) {
    if (c == 's')
      startup_cmd = optarg;
    else if (c == 'd')
      log_level = WLR_DEBUG;
    else if (c == 'v')
      die("bonsaiwm " VERSION);
    else
      goto usage;
  }
  if (optind < argc)
    goto usage;

  /* Wayland requires XDG_RUNTIME_DIR for creating its communications socket */
  if (!getenv("XDG_RUNTIME_DIR"))
    die("XDG_RUNTIME_DIR must be set");
  setup();
  run(startup_cmd);
  cleanup();
  return EXIT_SUCCESS;

usage:
  die("Usage: %s [-v] [-d] [-s startup command]", argv[0]);
}

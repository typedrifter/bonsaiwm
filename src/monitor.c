#include <stdlib.h>

#include <wayland-server-core.h>
#include <scenefx/render/fx_renderer/fx_renderer.h>
#include <scenefx/types/wlr_scene.h>
#include <wlr/types/wlr_cursor.h>
#include <wlr/types/wlr_gamma_control_v1.h>
#include <wlr/types/wlr_output_management_v1.h>
#include <wlr/types/wlr_output_power_management_v1.h>
#include <wlr/types/wlr_xdg_output_v1.h>
#include <wlr/util/log.h>

#include "monitor.h"
#include "client.h"
#include "config.h"
#include "decorations.h"
#include "focus.h"
#include "layer_shell.h"
#include "layout.h"
#include "session_lock.h"
#include "util.h"
#include "ext-protocol/ext-workspace.h"

static struct wlr_output_manager_v1 *output_mgr;
static struct wlr_output_power_manager_v1 *power_mgr;

static void createmon(struct wl_listener *listener, void *data);
static void cleanupmon(struct wl_listener *listener, void *data);
static void closemon(Monitor *m);
void rendermon(struct wl_listener *listener, void *data);
static void outputmgrapply(struct wl_listener *listener, void *data);
static void outputmgrtest(struct wl_listener *listener, void *data);
static void outputmgrapplyortest(struct wlr_output_configuration_v1 *config,
                                 int test);
static void powermgrsetmode(struct wl_listener *listener, void *data);
static void requestmonstate(struct wl_listener *listener, void *data);
static void updatemons(struct wl_listener *listener, void *data);

static struct wl_listener new_output = {.notify = createmon};
static struct wl_listener layout_change = {.notify = updatemons};
static struct wl_listener output_mgr_apply = {.notify = outputmgrapply};
static struct wl_listener output_mgr_test = {.notify = outputmgrtest};
static struct wl_listener output_power_mgr_set_mode = {
    .notify = powermgrsetmode};

extern struct wlr_scene_rect *locked_bg;
extern struct wlr_xdg_shell *xdg_shell;
extern struct wlr_cursor *cursor;
extern struct wl_list fstack;

struct wlr_output_state;
void resize(Client *c, struct wlr_box geo, int interact);

/* ----- helper functions and externs from bonsaiwm.c ----- */

void motionnotify(uint32_t time, struct wlr_input_device *device, double dx,
                  double dy, double dx_unaccel, double dy_unaccel);
void setmon(Client *c, Monitor *m, uint32_t newtags);

/* ----- monitor lifecycle ----- */

static void createmon(struct wl_listener *listener, void *data) {
  struct wlr_output *wlr_output = data;
  const MonitorRule *r;
  size_t i;
  struct wlr_output_state state;
  Monitor *m;

  if (!wlr_output_init_render(wlr_output, alloc, drw))
    return;

  m = wlr_output->data = ecalloc(1, sizeof(*m));
  m->wlr_output = wlr_output;

  for (i = 0; i < LENGTH(m->layers); i++)
    wl_list_init(&m->layers[i]);

  m->gappih = config.gappih;
  m->gappiv = config.gappiv;
  m->gappoh = config.gappoh;
  m->gappov = config.gappov;

  wlr_output_state_init(&state);
  m->tagstate = ecalloc(1, sizeof(TagState));
  m->tagstate->curtag = m->tagstate->prevtag = 1;
  m->tagset[0] = m->tagset[1] = 1;
  for (r = monrules; r < monrules + monrules_count; r++) {
    if (!r->name || strstr(wlr_output->name, r->name)) {
      int lt0, lt1;
      m->m.x = r->x;
      m->m.y = r->y;
      lt0 = r->lt;
      lt1 = (layouts_count > 1 && r->lt != LtFloat) ? LtFloat : LtTile;
      layout_tagstate_init(m->tagstate, r->nmaster, r->mfact, lt0, lt1, 0);
      strncpy(m->ltsymbol, layouts[tagstate_layout(m)].symbol,
              LENGTH(m->ltsymbol));
      wlr_output_state_set_scale(&state, r->scale);
      wlr_output_state_set_transform(&state, r->rr);
      break;
    }
  }

  wlr_output_state_set_mode(&state, wlr_output_preferred_mode(wlr_output));

  LISTEN(&wlr_output->events.frame, &m->frame, rendermon);
  LISTEN(&wlr_output->events.destroy, &m->destroy, cleanupmon);
  LISTEN(&wlr_output->events.request_state, &m->request_state, requestmonstate);

  wlr_output_state_set_enabled(&state, 1);
  wlr_output_commit_state(wlr_output, &state);
  wlr_output_state_finish(&state);

  wl_list_insert(&mons, &m->link);

  workspaces_create(m);

  printstatus();

  m->fullscreen_bg =
      wlr_scene_rect_create(layers[LyrFS], 0, 0, fullscreen_background);
  wlr_scene_node_set_enabled(&m->fullscreen_bg->node, 0);

  m->blur_layer = wlr_scene_optimized_blur_create(&scene->tree, 0, 0);
  wlr_scene_node_reparent(&m->blur_layer->node, layers[LyrBlur]);
  wlr_scene_node_set_enabled(&m->blur_layer->node, 0);

  m->scene_output = wlr_scene_output_create(scene, wlr_output);
  if (m->m.x == -1 && m->m.y == -1)
    wlr_output_layout_add_auto(output_layout, wlr_output);
  else
    wlr_output_layout_add(output_layout, wlr_output, m->m.x, m->m.y);
}

static void cleanupmon(struct wl_listener *listener, void *data) {
  Monitor *m = wl_container_of(listener, m, destroy);
  LayerSurface *l, *tmp;
  size_t i;

  for (i = 0; i < LENGTH(m->layers); i++) {
    wl_list_for_each_safe(l, tmp, &m->layers[i], link)
        wlr_layer_surface_v1_destroy(l->layer_surface);
  }

  wl_list_remove(&m->destroy.link);
  wl_list_remove(&m->frame.link);
  wl_list_remove(&m->link);
  wl_list_remove(&m->request_state.link);
  if (m->lock_surface)
    destroylocksurface(&m->destroy_lock_surface, NULL);

  workspaces_destroy(m);

  m->wlr_output->data = NULL;
  wlr_output_layout_remove(output_layout, m->wlr_output);
  wlr_scene_output_destroy(m->scene_output);

  closemon(m);
  free(m);
}

static void closemon(Monitor *m) {
  Client *c;
  int i = 0, nmons = wl_list_length(&mons);
  if (!nmons) {
    selmon = NULL;
  } else if (m == selmon) {
    do
      selmon = wl_container_of(mons.next, selmon, link);
    while (!selmon->wlr_output->enabled && i++ < nmons);

    if (!selmon->wlr_output->enabled)
      selmon = NULL;
  }

  wl_list_for_each(c, layout_tiling_order(), link) {
    if (c->isfloating && c->geom.x > m->m.width)
      resize(c,
             (struct wlr_box){.x = c->geom.x - m->w.width,
                               .y = c->geom.y,
                               .width = c->geom.width,
                               .height = c->geom.height},
             0);
    if (c->mon == m)
      setmon(c, selmon, c->tags);
  }
  focusclient(focustop(selmon), 1);
  printstatus();
}

/* ----- rendering ----- */

void rendermon(struct wl_listener *listener, void *data) {
  Monitor *m = wl_container_of(listener, m, frame);
  Client *c;
  struct wlr_output_state pending = {0};
  struct timespec now;

  wl_list_for_each(c, layout_tiling_order(), link) {
    if (c->resize && !c->isfloating && client_is_rendered_on_mon(c, m) &&
        !client_is_stopped(c))
      goto skip;
  }

  wlr_scene_output_build_state(m->scene_output, &pending, NULL);

  apply_output_scene_effects(&m->scene_output->scene->tree.node, NULL);

  wlr_scene_output_commit(m->scene_output, NULL);

skip:
  clock_gettime(CLOCK_MONOTONIC, &now);
  wlr_scene_output_send_frame_done(m->scene_output, &now);
  wlr_output_state_finish(&pending);
}

/* ----- output management ----- */

static void outputmgrapply(struct wl_listener *listener, void *data) {
  struct wlr_output_configuration_v1 *config = data;
  outputmgrapplyortest(config, 0);
}

static void outputmgrtest(struct wl_listener *listener, void *data) {
  struct wlr_output_configuration_v1 *config = data;
  outputmgrapplyortest(config, 1);
}

static void outputmgrapplyortest(struct wlr_output_configuration_v1 *config,
                                 int test) {
  struct wlr_output_configuration_head_v1 *config_head;
  int ok = 1;

  wl_list_for_each(config_head, &config->heads, link) {
    struct wlr_output *wlr_output = config_head->state.output;
    Monitor *m = wlr_output->data;
    struct wlr_output_state state;

    m->asleep = 0;

    wlr_output_state_init(&state);
    wlr_output_state_set_enabled(&state, config_head->state.enabled);
    if (!config_head->state.enabled)
      goto apply_or_test;

    if (config_head->state.mode)
      wlr_output_state_set_mode(&state, config_head->state.mode);
    else
      wlr_output_state_set_custom_mode(&state,
                                       config_head->state.custom_mode.width,
                                       config_head->state.custom_mode.height,
                                       config_head->state.custom_mode.refresh);

    wlr_output_state_set_transform(&state, config_head->state.transform);
    wlr_output_state_set_scale(&state, config_head->state.scale);
    wlr_output_state_set_adaptive_sync_enabled(
        &state, config_head->state.adaptive_sync_enabled);

  apply_or_test:
    ok &= test ? wlr_output_test_state(wlr_output, &state)
               : wlr_output_commit_state(wlr_output, &state);

    if (!test && wlr_output->enabled &&
        (m->m.x != config_head->state.x || m->m.y != config_head->state.y))
      wlr_output_layout_add(output_layout, wlr_output, config_head->state.x,
                            config_head->state.y);

    wlr_output_state_finish(&state);
  }

  if (ok)
    wlr_output_configuration_v1_send_succeeded(config);
  else
    wlr_output_configuration_v1_send_failed(config);
  wlr_output_configuration_v1_destroy(config);

  updatemons(NULL, NULL);
}

/* ----- power management ----- */

static void powermgrsetmode(struct wl_listener *listener, void *data) {
  struct wlr_output_power_v1_set_mode_event *event = data;
  struct wlr_output_state state = {0};
  Monitor *m = event->output->data;

  if (!m)
    return;

  m->gamma_lut_changed = 1;
  wlr_output_state_set_enabled(&state, event->mode);
  wlr_output_commit_state(m->wlr_output, &state);

  m->asleep = !event->mode;
  updatemons(NULL, NULL);
}

static void requestmonstate(struct wl_listener *listener, void *data) {
  struct wlr_output_event_request_state *event = data;
  wlr_output_commit_state(event->output, event->state);
  updatemons(NULL, NULL);
}

/* ----- layout change ----- */

static void updatemons(struct wl_listener *listener, void *data) {
  struct wlr_output_configuration_v1 *config =
      wlr_output_configuration_v1_create();
  Client *c;
  struct wlr_output_configuration_head_v1 *config_head;
  Monitor *m;

  wl_list_for_each(m, &mons, link) {
    if (m->wlr_output->enabled || m->asleep)
      continue;
    config_head =
        wlr_output_configuration_head_v1_create(config, m->wlr_output);
    config_head->state.enabled = 0;
    wlr_output_layout_remove(output_layout, m->wlr_output);
    closemon(m);
    m->m = m->w = (struct wlr_box){0};
  }
  wl_list_for_each(m, &mons, link) {
    if (m->wlr_output->enabled &&
        !wlr_output_layout_get(output_layout, m->wlr_output))
      wlr_output_layout_add_auto(output_layout, m->wlr_output);
  }

  wlr_output_layout_get_box(output_layout, NULL, &sgeom);

  wlr_scene_node_set_position(&root_bg->node, sgeom.x, sgeom.y);
  wlr_scene_rect_set_size(root_bg, sgeom.width, sgeom.height);

  wlr_scene_node_set_position(&locked_bg->node, sgeom.x, sgeom.y);
  wlr_scene_rect_set_size(locked_bg, sgeom.width, sgeom.height);

  wl_list_for_each(m, &mons, link) {
    if (!m->wlr_output->enabled)
      continue;
    config_head =
        wlr_output_configuration_head_v1_create(config, m->wlr_output);

    wlr_output_layout_get_box(output_layout, m->wlr_output, &m->m);
    m->w = m->m;
    wlr_scene_output_set_position(m->scene_output, m->m.x, m->m.y);

    wlr_scene_node_set_position(&m->fullscreen_bg->node, m->m.x, m->m.y);
    wlr_scene_rect_set_size(m->fullscreen_bg, m->m.width, m->m.height);

    wlr_scene_node_set_position(&m->blur_layer->node, m->m.x, m->m.y);
    wlr_scene_optimized_blur_set_size(m->blur_layer, m->m.width, m->m.height);

    if (m->lock_surface) {
      struct wlr_scene_tree *scene_tree = m->lock_surface->surface->data;
      wlr_scene_node_set_position(&scene_tree->node, m->m.x, m->m.y);
      wlr_session_lock_surface_v1_configure(m->lock_surface, m->m.width,
                                            m->m.height);
    }

    arrangelayers(m);
    arrange(m);
    arrange_effects();
    if ((c = focustop(m)) && c->isfullscreen)
      resize(c, m->m, 0);

    m->gamma_lut_changed = 1;

    config_head->state.x = m->m.x;
    config_head->state.y = m->m.y;

    if (!selmon) {
      selmon = m;
    }
  }

  if (selmon && selmon->wlr_output->enabled) {
    wl_list_for_each(c, layout_tiling_order(), link) {
      if (!c->mon && client_surface(c)->mapped)
        setmon(c, selmon, c->tags);
    }
    focusclient(focustop(selmon), 1);
    if (selmon->lock_surface) {
      client_notify_enter(selmon->lock_surface->surface,
                          wlr_seat_get_keyboard(seat));
      client_activate_surface(selmon->lock_surface->surface, 1);
    }
  }

  wlr_cursor_move(cursor, NULL, 0, 0);

  wlr_output_manager_v1_set_configuration(output_mgr, config);
}

/* ----- init / cleanup ----- */

void monitor_init(void) {
  output_layout = wlr_output_layout_create(dpy);
  wl_signal_add(&output_layout->events.change, &layout_change);
  wlr_xdg_output_manager_v1_create(dpy, output_layout);

  wl_list_init(&mons);
  wl_signal_add(&backend->events.new_output, &new_output);

  output_mgr = wlr_output_manager_v1_create(dpy);
  wl_signal_add(&output_mgr->events.apply, &output_mgr_apply);
  wl_signal_add(&output_mgr->events.test, &output_mgr_test);

  workspaces_init();

  power_mgr = wlr_output_power_manager_v1_create(dpy);
  wl_signal_add(&power_mgr->events.set_mode, &output_power_mgr_set_mode);

  wlr_scene_set_gamma_control_manager_v1(
      scene, wlr_gamma_control_manager_v1_create(dpy));
}

void monitor_cleanup(void) {
  wl_list_remove(&layout_change.link);
  wl_list_remove(&new_output.link);
  wl_list_remove(&output_mgr_apply.link);
  wl_list_remove(&output_mgr_test.link);
  wl_list_remove(&output_power_mgr_set_mode.link);
}

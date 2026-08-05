/* Session lock module implementation. */
#include <stdlib.h>

#include <wayland-server-core.h>
#include <wlr/types/wlr_idle_notify_v1.h>
#include <wlr/types/wlr_scene.h>
#include <wlr/util/log.h>
#include <scenefx/types/wlr_scene.h>

#include "session_lock.h"
#include "client.h"
#include "config.h"
#include "focus.h"
#include "util.h"

void motionnotify(uint32_t time, struct wlr_input_device *device, double dx,
                  double dy, double dx_unaccel, double dy_unaccel);

static struct wlr_scene_rect *locked_bg;
static struct wlr_session_lock_manager_v1 *session_lock_mgr;
static struct wlr_session_lock_v1 *cur_lock;

static void locksession(struct wl_listener *listener, void *data);
static void unlocksession(struct wl_listener *listener, void *data);
static void createlocksurface(struct wl_listener *listener, void *data);
static void destroylock(SessionLock *lock, int unlock);
static void destroysessionlock(struct wl_listener *listener, void *data);

static struct wl_listener new_session_lock = {.notify = locksession};

void checkidleinhibitor(struct wlr_surface *exclude) {
  int inhibited = 0, unused_lx, unused_ly;
  struct wlr_idle_inhibitor_v1 *inhibitor;
  wl_list_for_each(inhibitor, &idle_inhibit_mgr->inhibitors, link) {
    struct wlr_surface *surface =
        wlr_surface_get_root_surface(inhibitor->surface);
    struct wlr_scene_tree *tree = surface->data;
    if (exclude != surface &&
        (bypass_surface_visibility ||
         (!tree ||
          wlr_scene_node_coords(&tree->node, &unused_lx, &unused_ly)))) {
      inhibited = 1;
      break;
    }
  }

  wlr_idle_notifier_v1_set_inhibited(idle_notifier, inhibited);
}

static void locksession(struct wl_listener *listener, void *data) {
  struct wlr_session_lock_v1 *session_lock = data;
  SessionLock *lock;
  wlr_scene_node_set_enabled(&locked_bg->node, 1);
  if (cur_lock) {
    wlr_session_lock_v1_destroy(session_lock);
    return;
  }
  lock = session_lock->data = ecalloc(1, sizeof(*lock));
  focusclient(NULL, 0);

  lock->scene = wlr_scene_tree_create(layers[LyrBlock]);
  cur_lock = lock->lock = session_lock;
  locked = 1;

  LISTEN(&session_lock->events.new_surface, &lock->new_surface,
         createlocksurface);
  LISTEN(&session_lock->events.destroy, &lock->destroy, destroysessionlock);
  LISTEN(&session_lock->events.unlock, &lock->unlock, unlocksession);

  wlr_session_lock_v1_send_locked(session_lock);
}

static void createlocksurface(struct wl_listener *listener, void *data) {
  SessionLock *lock = wl_container_of(listener, lock, new_surface);
  struct wlr_session_lock_surface_v1 *lock_surface = data;
  Monitor *m = lock_surface->output->data;
  struct wlr_scene_tree *scene_tree = lock_surface->surface->data =
      wlr_scene_subsurface_tree_create(lock->scene, lock_surface->surface);
  m->lock_surface = lock_surface;

  wlr_scene_node_set_position(&scene_tree->node, m->m.x, m->m.y);
  wlr_session_lock_surface_v1_configure(lock_surface, m->m.width, m->m.height);

  LISTEN(&lock_surface->events.destroy, &m->destroy_lock_surface,
         destroylocksurface);

  if (m == selmon)
    client_notify_enter(lock_surface->surface, wlr_seat_get_keyboard(seat));
}

static void destroylock(SessionLock *lock, int unlock) {
  wlr_seat_keyboard_notify_clear_focus(seat);
  if ((locked = !unlock))
    goto destroy;

  wlr_scene_node_set_enabled(&locked_bg->node, 0);

  focusclient(focustop(selmon), 0);
  motionnotify(0, NULL, 0, 0, 0, 0);

destroy:
  wl_list_remove(&lock->new_surface.link);
  wl_list_remove(&lock->unlock.link);
  wl_list_remove(&lock->destroy.link);

  wlr_scene_node_destroy(&lock->scene->node);
  cur_lock = NULL;
  free(lock);
}

void destroylocksurface(struct wl_listener *listener, void *data) {
  Monitor *m = wl_container_of(listener, m, destroy_lock_surface);
  struct wlr_session_lock_surface_v1 *surface, *lock_surface = m->lock_surface;

  m->lock_surface = NULL;
  wl_list_remove(&m->destroy_lock_surface.link);

  if (lock_surface->surface != seat->keyboard_state.focused_surface)
    return;

  if (locked && cur_lock && !wl_list_empty(&cur_lock->surfaces)) {
    surface = wl_container_of(cur_lock->surfaces.next, surface, link);
    client_notify_enter(surface->surface, wlr_seat_get_keyboard(seat));
  } else if (!locked) {
    focusclient(focustop(selmon), 1);
  } else {
    wlr_seat_keyboard_clear_focus(seat);
  }
}

static void destroysessionlock(struct wl_listener *listener, void *data) {
  SessionLock *lock = wl_container_of(listener, lock, destroy);
  destroylock(lock, 0);
}

static void unlocksession(struct wl_listener *listener, void *data) {
  SessionLock *lock = wl_container_of(listener, lock, unlock);
  destroylock(lock, 1);
}

void session_lock_init(void) {
  session_lock_mgr = wlr_session_lock_manager_v1_create(dpy);
  wl_signal_add(&session_lock_mgr->events.new_lock, &new_session_lock);
  locked_bg = wlr_scene_rect_create(layers[LyrBlock], sgeom.width, sgeom.height,
                                    (float[4]){0.1f, 0.1f, 0.1f, 1.0f});
  wlr_scene_node_set_enabled(&locked_bg->node, 0);
}


void session_lock_update_geometry(void) {
  wlr_scene_node_set_position(&locked_bg->node, sgeom.x, sgeom.y);
  wlr_scene_rect_set_size(locked_bg, sgeom.width, sgeom.height);
}

void session_lock_cleanup(void) {
  wl_list_remove(&new_session_lock.link);
}

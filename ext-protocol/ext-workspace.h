/*
 * ext-workspace-v1 glue.
 *
 * Exposes each tag of every monitor as an ext-workspace handle so clients
 * (e.g. Waybar's ext/workspaces module) can enumerate, activate and deactivate
 * bonsaiwm tags. The protocol implementation itself (wlr_ext_workspace_v1.c)
 * is vendored from wlroots because wlroots 0.19 does not ship it yet.
 *
 * It follows the project's fixed-tag idiom: each monitor owns one group
 * handle plus a fixed array of workspace handles (one per tag).
 */
#include "wlr_ext_workspace_v1.h"

#define EXT_WORKSPACE_ENABLE_CAPS                                              \
  EXT_WORKSPACE_HANDLE_V1_WORKSPACE_CAPABILITIES_ACTIVATE |                    \
      EXT_WORKSPACE_HANDLE_V1_WORKSPACE_CAPABILITIES_DEACTIVATE

static struct wlr_ext_workspace_manager_v1 *ext_manager;

static void handle_ext_commit(struct wl_listener *listener, void *data);
static struct wl_listener ext_manager_commit_listener = {
    .notify = handle_ext_commit};

/* Map a workspace handle back to its (monitor, 1-based tag). */
static int find_workspace(struct wlr_ext_workspace_handle_v1 *handle,
                          Monitor **mon, uint32_t *tag) {
  Monitor *m;
  wl_list_for_each(m, &mons, link) {
    uint32_t i;
    for (i = 1; i <= TAGCOUNT; i++)
      if (m->ext_workspaces[i - 1] == handle) {
        *mon = m;
        *tag = i;
        return 1;
      }
  }
  return 0;
}

static void handle_ext_commit(struct wl_listener *listener, void *data) {
  struct wlr_ext_workspace_v1_commit_event *event = data;
  struct wlr_ext_workspace_v1_request *request;

  wl_list_for_each(request, event->requests, link) {
    struct wlr_ext_workspace_handle_v1 *handle;
    Monitor *m;
    uint32_t tag;
    Arg arg;

    switch (request->type) {
    case WLR_EXT_WORKSPACE_V1_REQUEST_ACTIVATE:
      handle = request->activate.workspace;
      if (handle && find_workspace(handle, &m, &tag)) {
        arg.ui = 1u << (tag - 1);
        view_on(m, &arg);
      }
      break;
    case WLR_EXT_WORKSPACE_V1_REQUEST_DEACTIVATE:
      handle = request->deactivate.workspace;
      if (handle && find_workspace(handle, &m, &tag)) {
        arg.ui = 1u << (tag - 1);
        view_off(m, &arg);
      }
      break;
    default:
      break;
    }
  }
}

/* Create the group for a new monitor and one workspace handle per tag. */
void workspaces_create(Monitor *m) {
  uint32_t i;

  m->ext_group = wlr_ext_workspace_group_handle_v1_create(
      ext_manager, 0 /* no group capabilities: workspaces are fixed */);
  wlr_ext_workspace_group_handle_v1_output_enter(m->ext_group, m->wlr_output);
  for (i = 1; i <= TAGCOUNT; i++) {
    char name[2] = {0};
    name[0] = '0' + i;
    m->ext_workspaces[i - 1] = wlr_ext_workspace_handle_v1_create(
        ext_manager, name, EXT_WORKSPACE_ENABLE_CAPS);
    wlr_ext_workspace_handle_v1_set_group(m->ext_workspaces[i - 1],
                                          m->ext_group);
    wlr_ext_workspace_handle_v1_set_name(m->ext_workspaces[i - 1], name);
  }
}

void workspaces_destroy(Monitor *m) {
  uint32_t i;

  wlr_ext_workspace_group_handle_v1_output_leave(m->ext_group, m->wlr_output);
  for (i = 1; i <= TAGCOUNT; i++)
    wlr_ext_workspace_handle_v1_destroy(m->ext_workspaces[i - 1]);
  wlr_ext_workspace_group_handle_v1_destroy(m->ext_group);
}

/* Push a monitor's current tag state to its workspace handles.
 * occ/urg are computed once per monitor by printstatus (one client walk). */
void ext_workspace_printstatus(Monitor *m, uint32_t occ, uint32_t urg) {
  uint32_t i;
  for (i = 1; i <= TAGCOUNT; i++) {
    struct wlr_ext_workspace_handle_v1 *ws = m->ext_workspaces[i - 1];
    int active, occupied, urgent;

    active = !!(m->tagset[m->seltags] & (1u << (i - 1)) & TAGMASK);
    occupied = !!(occ & (1u << (i - 1)));
    urgent = !!(urg & (1u << (i - 1)));

    wlr_ext_workspace_handle_v1_set_hidden(ws,
                                           !(active || occupied || urgent));
    wlr_ext_workspace_handle_v1_set_urgent(ws, urgent);
    wlr_ext_workspace_handle_v1_set_active(ws, active);
  }
}

void workspaces_init(void) {
  ext_manager = wlr_ext_workspace_manager_v1_create(dpy, 1);
  wl_signal_add(&ext_manager->events.commit, &ext_manager_commit_listener);
}

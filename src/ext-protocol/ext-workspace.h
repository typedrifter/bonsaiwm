/*
 * ext-workspace-v1 glue.
 *
 * Exposes each tag of every monitor as an ext-workspace handle so clients
 * (e.g. Waybar's ext/workspaces module) can enumerate, activate and deactivate
 * bonsaiwm tags. The protocol implementation itself (wlr_ext_workspace_v1.c)
 * is vendored from wlroots because wlroots 0.19 does not ship it yet.
 */
#ifndef EXT_WORKSPACE_H
#define EXT_WORKSPACE_H

#include "wlr_ext_workspace_v1.h"
#include "bonsaiwm.h"

/* extern globals (defined in ext-workspace.c, wired by bonsaiwm.c) */
extern struct wl_listener ext_manager_commit_listener;

void workspaces_create(Monitor *m);
void workspaces_destroy(Monitor *m);
void ext_workspace_printstatus(Monitor *m, uint32_t occ, uint32_t urg);
void workspaces_init(void);

#endif /* EXT_WORKSPACE_H */

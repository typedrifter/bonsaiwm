/* Shared compositor types: the top-level structures that cut across
 * modules. bonsaiwm.c owns the concrete listeners/lifecycle; layout.c
 * computes Placements from these. Nothing here is module implementation —
 * only data types and their dependencies. */
#ifndef BONSAIWM_H
#define BONSAIWM_H

#include <stdint.h>
#include <wayland-util.h>
#include <wlr/types/wlr_output.h>
#include <wlr/types/wlr_session_lock_v1.h>
#include <wlr/types/wlr_layer_shell_v1.h>
#include <wlr/types/wlr_xdg_decoration_v1.h>
#include <wlr/types/wlr_xdg_shell.h>
#include <wlr/util/box.h>
#include <scenefx/types/wlr_scene.h>

#include "config.h"
#include "ext-protocol/wlr_ext_workspace_v1.h"

/* client types */
enum { XDGShell, LayerShell, X11 };

/* scene layers */
enum {
  LyrBg,
  LyrBlur,
  LyrBottom,
  LyrTile,
  LyrFloat,
  LyrTop,
  LyrFS,
  LyrOverlay,
  LyrBlock,
  NUM_LAYERS
};

typedef struct TagState TagState;
typedef struct Monitor Monitor;

/* Per-tag layout state, ported from the dwl pertag patch:
 * https://codeberg.org/dwl/dwl-patches/src/branch/main/patches/pertag
 * Layouts are stored as indices into layouts[] (upstream uses pointers).
 * Owned per-Monitor; the layout module is the only writer. */
struct TagState {
  unsigned int curtag, prevtag;           /* current and previous tag */
  int nmasters[TAGCOUNT + 1];             /* number of windows in master area */
  float mfacts[TAGCOUNT + 1];             /* mfacts per tag */
  unsigned int sellts[TAGCOUNT + 1];      /* selected layouts */
  int ltidxs[TAGCOUNT + 1][2];            /* matrix of tags and layouts indexes */
};

typedef struct Monitor Monitor;

typedef struct {
  /* Must keep this field first */
  unsigned int type; /* XDGShell or X11* */

  Monitor *mon;
  struct wlr_scene_tree *scene;
  struct wlr_scene_rect *border[4]; /* top, bottom, left, right */
  struct wlr_scene_tree *scene_surface;
  struct wl_list link;
  struct wl_list flink;
  struct wlr_box geom;   /* layout-relative, includes border */
  struct wlr_box prev;   /* layout-relative, includes border */
  struct wlr_box bounds; /* only width and height are used */
  union {
    struct wlr_xdg_surface *xdg;
#ifdef XWAYLAND
    struct wlr_xwayland_surface *xwayland;
#endif
  } surface;
  struct wlr_xdg_toplevel_decoration_v1 *decoration;
  struct wl_listener commit;
  struct wl_listener map;
  struct wl_listener maximize;
  struct wl_listener unmap;
  struct wl_listener destroy;
  struct wl_listener set_title;
  struct wl_listener fullscreen;
  struct wl_listener set_decoration_mode;
  struct wl_listener destroy_decoration;
#ifdef XWAYLAND
  struct wl_listener activate;
  struct wl_listener associate;
  struct wl_listener dissociate;
  struct wl_listener configure;
  struct wl_listener set_hints;
#endif
  unsigned int bw;
  uint32_t tags;
  int isfloating, isurgent, isfullscreen;
  uint32_t resize; /* configure serial of a pending resize */

  float opacity;
  int corner_radius;
  struct wlr_scene_shadow *shadow;
  int has_shadow_enabled;
  struct wlr_scene_rect *round_border;
} Client;

struct Monitor {
  struct wl_list link;
  struct wlr_output *wlr_output;
  struct wlr_scene_output *scene_output;
  struct wlr_scene_rect *fullscreen_bg; /* See createmon() for info */
  struct wlr_ext_workspace_group_handle_v1 *ext_group;
  struct wlr_ext_workspace_handle_v1 *ext_workspaces[TAGCOUNT];
  struct wl_listener frame;
  struct wl_listener destroy;
  struct wl_listener request_state;
  struct wl_listener destroy_lock_surface;
  struct wlr_session_lock_surface_v1 *lock_surface;
  struct wlr_box m;         /* monitor area, layout-relative */
  struct wlr_box w;         /* window area, layout-relative */
  struct wl_list layers[4]; /* LayerSurface.link */
  TagState *tagstate;       /* sole source of layout state (nmaster, mfact, ...) */
  int gappih;               /* horizontal gap between windows */
  int gappiv;               /* vertical gap between windows */
  int gappoh;               /* horizontal outer gaps */
  int gappov;               /* vertical outer gaps */
  unsigned int seltags;
  uint32_t tagset[2];
  int gamma_lut_changed;
  char ltsymbol[16];
  int asleep;
  struct wlr_scene_optimized_blur *blur_layer;
};

/* Layer surface wrapper: per-layer_surface tracking for the compositor.
 * Moved here from bonsaiwm.c so client.h (and other modules) can reference
 * it through toplevel_from_wlr_surface. */
typedef struct {
  unsigned int type; /* LayerShell */
  Monitor *mon;
  struct wlr_scene_tree *scene;
  struct wlr_scene_tree *popups;
  struct wlr_scene_layer_surface_v1 *scene_layer;
  struct wl_list link;
  int mapped;
  struct wlr_layer_surface_v1 *layer_surface;
  struct wl_listener destroy;
  struct wl_listener unmap;
  struct wl_listener surface_commit;
} LayerSurface;

#endif /* BONSAIWM_H */

/* Layout module: workspace state + tiling order + pure Placement computation.
 *
 * Owns:
 *   - the Tiling order (the list layout_insert/layout_promote/layout_remove)
 *   - the per-tag layout parameters (TagState) via the tagstate_* accessors
 *   - the placement computation (layout_place), pure and side-effect free
 *
 * Does NOT own: Focus (fstack/focusclient), the scene graph, or rendering.
 * layout_place computes final, bounds-clamped boxes; applying them to the
 * scene and sending configures is the compositor's job (resize()).
 */
#ifndef LAYOUT_H
#define LAYOUT_H

#include <stddef.h>
#include <stdint.h>
#include <wayland-util.h>

#include "bonsaiwm.h"

/* ── TagState read accessors (single source of truth for per-tag state) ── */

/* Return the 1-based index of the lowest set bit in mask.
 * This turns a tag bitmask (e.g. 1<<2) into a tag number (e.g. 3) so the
 * per-tag state arrays know which slot to read/write. The callers already
 * guarantee mask is non-zero, because an empty tagset is never a valid view. */
static inline size_t layout_firsttag(const uint32_t mask) {
  size_t i = 0;
  while (!(mask & (1u << i)))
    i++;
  return i + 1;
}

static inline int tagstate_nmaster(const Monitor *m) {
  return m->tagstate->nmasters[m->tagstate->curtag];
}
static inline float tagstate_mfact(const Monitor *m) {
  return m->tagstate->mfacts[m->tagstate->curtag];
}
static inline unsigned int tagstate_sellt(const Monitor *m) {
  return m->tagstate->sellts[m->tagstate->curtag];
}
static inline int tagstate_lt(const Monitor *m, unsigned int slot) {
  return m->tagstate->ltidxs[m->tagstate->curtag][slot];
}
static inline int tagstate_layout(const Monitor *m) {
  return tagstate_lt(m, tagstate_sellt(m));
}

/* ── Placement ── */

/* Which placement algorithm a pass should run. Mirrors the compositor's
 * arrangefn[] dispatch (LtTile / LtMonocle); floating is no placement. */
enum layout_kind {
  LAYOUT_TILE,
  LAYOUT_MONOCLE,
};

/* Gaps behaviour for a placement pass. Mirrors config.enablegaps / smartgaps
 * so the computation stays free of the config module's globals. */
struct layout_opts {
  int enablegaps; /* 0 disables all gaps */
  int smartgaps;  /* disable gaps when this many tiled Clients are visible;
                     0 = never */
};

struct Placement {
  Client *client;
  struct wlr_box box; /* final, bounds-clamped geometry */
};

/* Is c part of m's current View? (VISIBLEON, now a function so layout.c and
 * bonsaiwm.c share one implementation.) */
int layout_visible(const Client *c, const Monitor *m);

/* Number of visible, non-floating, non-fullscreen Clients on m. */
int layout_tiling_count(const Monitor *m);

/* Clamp a box to fit inside bbox, enforcing a minimum size for borders.
 * This is the pure form of the old applybounds(); it mutates *box only. */
void layout_bounds(struct wlr_box *box, const struct wlr_box *bbox,
                   uint32_t bw);

/* Compute the Placements for m's current Layout. Pure: reads m and the
 * Tiling order, writes up to `cap` entries of out, returns the count placed
 * (<= cap). The box.height the accumulator uses is the clamped height, so
 * the result is exactly what the live compositor applies. */
size_t layout_place(const Monitor *m, enum layout_kind kind,
                    const struct layout_opts *opts, struct Placement *out,
                    size_t cap);

/* ── Tiling order ownership ── */

void layout_init(void);
void layout_insert(Client *c);  /* append to the Tiling order */
void layout_promote(Client *c); /* move to the front (zoom) */
void layout_remove(Client *c);
struct wl_list *layout_tiling_order(void);

#endif /* LAYOUT_H */

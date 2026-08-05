/* Layout module implementation. Pure placement math plus Tiling order
 * ownership. No wlroots/scene/seat calls: the compositor applies the
 * returned Placements (see resize() in bonsaiwm.c). */
#include <math.h>

#include "layout.h"

#define MAX(A, B) ((A) > (B) ? (A) : (B))
#define MIN(A, B) ((A) < (B) ? (A) : (B))

static struct wl_list tiling_order;

void layout_init(void) {
  wl_list_init(&tiling_order);
}

void layout_insert(Client *c) {
  wl_list_insert(tiling_order.prev, &c->link);
}

void layout_promote(Client *c) {
  wl_list_remove(&c->link);
  wl_list_insert(&tiling_order, &c->link);
}

void layout_remove(Client *c) {
  wl_list_remove(&c->link);
}

struct wl_list *layout_tiling_order(void) {
  return &tiling_order;
}

int layout_visible(const Client *c, const Monitor *m) {
  return m && c->mon == m && (c->tags & m->tagset[m->seltags]);
}

int layout_tiling_count(const Monitor *m) {
  Client *c;
  int n = 0;
  wl_list_for_each(c, &tiling_order, link) {
    if (layout_visible(c, m) && !c->isfloating && !c->isfullscreen)
      n++;
  }
  return n;
}

void layout_bounds(struct wlr_box *box, const struct wlr_box *bbox,
                   uint32_t bw) {
  /* set minimum possible */
  box->width = MAX(1 + 2 * (int)bw, box->width);
  box->height = MAX(1 + 2 * (int)bw, box->height);

  if (box->x >= bbox->x + bbox->width)
    box->x = bbox->x + bbox->width - box->width;
  if (box->y >= bbox->y + bbox->height)
    box->y = bbox->y + bbox->height - box->height;
  if (box->x + box->width <= bbox->x)
    box->x = bbox->x;
  if (box->y + box->height <= bbox->y)
    box->y = bbox->y;
}

size_t layout_place(const Monitor *m, enum layout_kind kind,
                    const struct layout_opts *opts, struct Placement *out,
                    size_t cap) {
  Client *c;
  size_t i = 0, placed = 0;
  int n = 0;
  static const struct layout_opts no_gaps = {0, 0};

  if (!opts)
    opts = &no_gaps;

  wl_list_for_each(c, &tiling_order, link) {
    if (layout_visible(c, m) && !c->isfloating && !c->isfullscreen)
      n++;
  }
  if (n == 0)
    return 0;

  if (kind == LAYOUT_MONOCLE) {
    wl_list_for_each(c, &tiling_order, link) {
      struct wlr_box box;
      if (!layout_visible(c, m) || c->isfloating || c->isfullscreen)
        continue;
      box = m->w;
      layout_bounds(&box, &m->w, c->bw);
      if (placed < cap)
        out[placed] = (struct Placement){.client = c, .box = box};
      placed++;
    }
    return placed < cap ? placed : cap;
  }

  /* tile */
  unsigned int mw, my, ty, h, r, oe = opts->enablegaps, ie = opts->enablegaps;

  if (opts->smartgaps == n) {
    oe = 0;
    ie = 0;
  }

  /* master width: include inner gap in calculation */
  if (n > tagstate_nmaster(m))
    mw = tagstate_nmaster(m)
             ? (unsigned int)roundf((m->w.width + m->gappiv * ie) *
                                    tagstate_mfact(m))
             : 0;
  else
    mw = m->w.width - 2 * m->gappov * oe + m->gappiv * ie;

  /* start below outer gap */
  my = ty = m->gappoh * oe;

  wl_list_for_each(c, &tiling_order, link) {
    struct wlr_box box;
    if (!layout_visible(c, m) || c->isfloating || c->isfullscreen)
      continue;
    if (i < (size_t)tagstate_nmaster(m)) {
      /* master windows */
      r = MIN((unsigned int)n, (unsigned int)tagstate_nmaster(m)) -
          (unsigned int)i;
      h = (m->w.height - my - m->gappoh * oe - m->gappih * ie * (r - 1)) / r;
      box = (struct wlr_box){.x = m->w.x + m->gappov * oe,
                             .y = m->w.y + my,
                             .width = mw - m->gappiv * ie,
                             .height = h};
      layout_bounds(&box, &m->w, c->bw);
      if (placed < cap)
        out[placed] = (struct Placement){.client = c, .box = box};
      placed++;
      i++;
      my += box.height + m->gappih * ie;
    } else {
      /* stack windows */
      r = (unsigned int)n - (unsigned int)i;
      h = (m->w.height - ty - m->gappoh * oe - m->gappih * ie * (r - 1)) / r;
      box = (struct wlr_box){.x = m->w.x + mw + m->gappov * oe,
                             .y = m->w.y + ty,
                             .width = m->w.width - mw - 2 * m->gappov * oe,
                             .height = h};
      layout_bounds(&box, &m->w, c->bw);
      if (placed < cap)
        out[placed] = (struct Placement){.client = c, .box = box};
      placed++;
      i++;
      ty += box.height + m->gappih * ie;
    }
  }
  return placed < cap ? placed : cap;
}

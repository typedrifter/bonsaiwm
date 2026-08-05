/* Focus module implementation. */
#include "focus.h"
#include "config.h"
#include "layout.h"
#include "client.h"

/* ── Focus stack queries ── */

Client *focustop(Monitor *m) {
  Client *c;
  wl_list_for_each(c, &fstack, flink) {
    if (layout_visible(c, m))
      return c;
  }
  return NULL;
}

/* ── Action handlers ── */

void focusmon(const Arg *arg) {
  int i = 0, nmons = wl_list_length(&mons);
  if (nmons) {
    do /* don't switch to disabled mons */
      selmon = dirtomon(arg->i);
    while (!selmon->wlr_output->enabled && i++ < nmons);
  }
  focusclient(focustop(selmon), 1);
}

void focusstack(const Arg *arg) {
  Client *c, *sel = focustop(selmon);
  if (!sel || (sel->isfullscreen && !client_has_children(sel)))
    return;
  if (arg->i > 0) {
    wl_list_for_each(c, &sel->link, link) {
      if (&c->link == layout_tiling_order())
        continue;
      if (layout_visible(c, selmon))
        break;
    }
  } else {
    wl_list_for_each_reverse(c, &sel->link, link) {
      if (&c->link == layout_tiling_order())
        continue;
      if (layout_visible(c, selmon))
        break;
    }
  }
  focusclient(c, 1);
}

/* ── Monitor lookup ── */

Monitor *dirtomon(enum wlr_direction dir) {
  struct wlr_output *next;
  if (!wlr_output_layout_get(output_layout, selmon->wlr_output))
    return selmon;
  if ((next = wlr_output_layout_adjacent_output(
           output_layout, dir, selmon->wlr_output, selmon->m.x, selmon->m.y)))
    return next->data;
  if ((next = wlr_output_layout_farthest_output(
           output_layout, dir ^ (WLR_DIRECTION_LEFT | WLR_DIRECTION_RIGHT),
           selmon->wlr_output, selmon->m.x, selmon->m.y)))
    return next->data;
  return selmon;
}

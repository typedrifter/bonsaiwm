/* Unit tests for the layout module's pure core: placement computation,
 * bounds clamping, visibility, Tiling order ownership.
 *
 * layout.c is the only module under test: it links against wlroots/scenefx
 * headers for the struct definitions but never calls into them, so tests
 * exercise the same seam the compositor uses (the interface is the test
 * surface). */
#include <setjmp.h>
#include <stdarg.h>
#include <stddef.h>
#include <stdlib.h>
#include <string.h>

#include <cmocka.h>

#include "layout.h"

/* ── helpers ─────────────────────────────────────────────────────────────── */

static void init_monitor(Monitor *m, int width, int height) {
  memset(m, 0, sizeof(*m));
  m->m = (struct wlr_box){0, 0, width, height};
  m->w = (struct wlr_box){0, 0, width, height};
  m->seltags = 0;
  m->tagset[0] = 1u << 0;
  m->tagset[1] = 1u << 0;
  m->tagstate = calloc(1, sizeof(*m->tagstate));
  m->tagstate->curtag = 1;
  m->tagstate->prevtag = 1;
  for (size_t i = 0; i <= TAGCOUNT; i++) {
    m->tagstate->nmasters[i] = 1;
    m->tagstate->mfacts[i] = 0.5f;
    m->tagstate->sellts[i] = 0;
  }
}

static void init_client(Client *c, Monitor *m, uint32_t tags, int floating,
                        int fullscreen) {
  memset(c, 0, sizeof(*c));
  c->mon = m;
  c->tags = tags;
  c->isfloating = floating;
  c->isfullscreen = fullscreen;
}

#define assert_box(b, ex, ey, ew, eh)                                          \
  do {                                                                         \
    assert_int_equal((b).x, (ex));                                             \
    assert_int_equal((b).y, (ey));                                             \
    assert_int_equal((b).width, (ew));                                         \
    assert_int_equal((b).height, (eh));                                        \
  } while (0)

/* ── tiling ───────────────────────────────────────────────────────────────── */

static void test_tile_master_stack(void **state) {
  (void)state;
  layout_init();
  Monitor m;
  init_monitor(&m, 800, 600);
  Client c1, c2, c3;
  init_client(&c1, &m, 1u << 0, 0, 0);
  init_client(&c2, &m, 1u << 0, 0, 0);
  init_client(&c3, &m, 1u << 0, 0, 0);
  layout_insert(&c1);
  layout_insert(&c2);
  layout_insert(&c3);

  struct layout_opts opts = {.enablegaps = 0, .smartgaps = 0};
  struct Placement p[4];
  size_t n = layout_place(&m, LAYOUT_TILE, &opts, p, 4);

  assert_int_equal(n, 3);
  assert_ptr_equal(p[0].client, &c1);
  assert_box(p[0].box, 0, 0, 400, 600);
  assert_ptr_equal(p[1].client, &c2);
  assert_box(p[1].box, 400, 0, 400, 300);
  assert_ptr_equal(p[2].client, &c3);
  assert_box(p[2].box, 400, 300, 400, 300);
}

static void test_tile_nmaster_two(void **state) {
  (void)state;
  layout_init();
  Monitor m;
  init_monitor(&m, 800, 600);
  m.tagstate->nmasters[1] = 2;
  Client c1, c2, c3;
  init_client(&c1, &m, 1u << 0, 0, 0);
  init_client(&c2, &m, 1u << 0, 0, 0);
  init_client(&c3, &m, 1u << 0, 0, 0);
  layout_insert(&c1);
  layout_insert(&c2);
  layout_insert(&c3);

  struct layout_opts opts = {.enablegaps = 0, .smartgaps = 0};
  struct Placement p[4];
  size_t n = layout_place(&m, LAYOUT_TILE, &opts, p, 4);

  assert_int_equal(n, 3);
  assert_box(p[0].box, 0, 0, 400, 300);
  assert_box(p[1].box, 0, 300, 400, 300);
  assert_box(p[2].box, 400, 0, 400, 600);
}

static void test_tile_gaps(void **state) {
  (void)state;
  layout_init();
  Monitor m;
  init_monitor(&m, 800, 600);
  m.gappoh = 10;
  m.gappov = 10;
  m.gappih = 10;
  m.gappiv = 10;
  Client c1;
  init_client(&c1, &m, 1u << 0, 0, 0);
  layout_insert(&c1);

  struct layout_opts opts = {.enablegaps = 1, .smartgaps = 0};
  struct Placement p[2];
  size_t n = layout_place(&m, LAYOUT_TILE, &opts, p, 2);

  assert_int_equal(n, 1);
  assert_box(p[0].box, 10, 10, 780, 580);
}

static void test_tile_smartgaps_single_window(void **state) {
  (void)state;
  layout_init();
  Monitor m;
  init_monitor(&m, 800, 600);
  m.gappoh = 10;
  m.gappov = 10;
  m.gappih = 10;
  m.gappiv = 10;
  Client c1;
  init_client(&c1, &m, 1u << 0, 0, 0);
  layout_insert(&c1);

  struct layout_opts opts = {.enablegaps = 1, .smartgaps = 1};
  struct Placement p[2];
  size_t n = layout_place(&m, LAYOUT_TILE, &opts, p, 2);

  assert_int_equal(n, 1);
  assert_box(p[0].box, 0, 0, 800, 600);
}

static void test_tile_excludes_floating_and_fullscreen(void **state) {
  (void)state;
  layout_init();
  Monitor m;
  init_monitor(&m, 800, 600);
  Client tiled, floating, fullscreen;
  init_client(&tiled, &m, 1u << 0, 0, 0);
  init_client(&floating, &m, 1u << 0, 1, 0);
  init_client(&fullscreen, &m, 1u << 0, 0, 1);
  layout_insert(&tiled);
  layout_insert(&floating);
  layout_insert(&fullscreen);

  assert_int_equal(layout_tiling_count(&m), 1);

  struct layout_opts opts = {.enablegaps = 0, .smartgaps = 0};
  struct Placement p[4];
  size_t n = layout_place(&m, LAYOUT_TILE, &opts, p, 4);

  assert_int_equal(n, 1);
  assert_ptr_equal(p[0].client, &tiled);
}

/* The accumulator advances by the *clamped* height of each row. With many
 * windows per row the height would undercut the border minimum, so it is
 * bumped to 1+2*bw and the next row must start at that bumped value — the
 * exact coupling that used to live inside resize() and could not be tested. */
static void test_tile_accumulator_uses_clamped_height(void **state) {
  (void)state;
  layout_init();
  Monitor m;
  init_monitor(&m, 800, 600);
  enum { NCLIENTS = 60 };
  Client c[NCLIENTS];
  for (int i = 0; i < NCLIENTS; i++) {
    init_client(&c[i], &m, 1u << 0, 0, 0);
    c[i].bw = 5; /* min height 1 + 2*5 = 11 */
    layout_insert(&c[i]);
  }

  struct layout_opts opts = {.enablegaps = 0, .smartgaps = 0};
  struct Placement p[NCLIENTS];
  size_t n = layout_place(&m, LAYOUT_TILE, &opts, p, NCLIENTS);

  assert_int_equal(n, NCLIENTS);
  assert_box(p[0].box, 0, 0, 400, 600);
  /* stack rows: rows 1..55 keep the clamped height and advance by exactly 11.
   * The first stack row starts at y=0, aligned with the master's top (both
   * start at gappoh); each subsequent row picks up the *clamped* height of
   * the previous row. */
  for (size_t i = 1; i < 55; i++) {
    assert_int_equal(p[i].box.height, 11);
    assert_int_equal(p[i].box.y, 11 * (int)(i - 1));
  }
}

/* ── monocle ──────────────────────────────────────────────────────────────── */

static void test_monocle_fills_monitor(void **state) {
  (void)state;
  layout_init();
  Monitor m;
  init_monitor(&m, 800, 600);
  Client c1, c2, c3;
  init_client(&c1, &m, 1u << 0, 0, 0);
  init_client(&c2, &m, 1u << 0, 0, 0);
  init_client(&c3, &m, 1u << 0, 0, 0);
  layout_insert(&c1);
  layout_insert(&c2);
  layout_insert(&c3);

  struct Placement p[4];
  size_t n = layout_place(&m, LAYOUT_MONOCLE, NULL, p, 4);

  assert_int_equal(n, 3);
  for (size_t i = 0; i < n; i++)
    assert_box(p[i].box, 0, 0, 800, 600);
}

/* ── visibility & tags ────────────────────────────────────────────────────── */

static void test_visible_follows_view_tagset(void **state) {
  (void)state;
  layout_init();
  Monitor m;
  init_monitor(&m, 800, 600);
  Client on_view, off_view;
  init_client(&on_view, &m, 1u << 0, 0, 0);
  init_client(&off_view, &m, 1u << 1, 0, 0);
  layout_insert(&on_view);
  layout_insert(&off_view);

  assert_true(layout_visible(&on_view, &m));
  assert_false(layout_visible(&off_view, &m));

  struct layout_opts opts = {.enablegaps = 0, .smartgaps = 0};
  struct Placement p[4];
  size_t n = layout_place(&m, LAYOUT_TILE, &opts, p, 4);

  assert_int_equal(n, 1);
  assert_ptr_equal(p[0].client, &on_view);
}

/* TagState is per-tag: switching the current tag restores that tag's Master
 * count, independently of what the previous tag held. */
static void test_tagstate_is_per_tag(void **state) {
  (void)state;
  layout_init();
  Monitor m;
  init_monitor(&m, 800, 600);
  Client c1, c2, c3;
  init_client(&c1, &m, 1u << 0, 0, 0);
  init_client(&c2, &m, 1u << 0, 0, 0);
  init_client(&c3, &m, 1u << 0, 0, 0);
  layout_insert(&c1);
  layout_insert(&c2);
  layout_insert(&c3);

  struct layout_opts opts = {.enablegaps = 0, .smartgaps = 0};
  struct Placement p[4];

  /* tag 1: one master */
  m.tagstate->curtag = 1;
  m.tagstate->nmasters[1] = 1;
  assert_int_equal(layout_place(&m, LAYOUT_TILE, &opts, p, 4), 3);
  assert_box(p[0].box, 0, 0, 400, 600);

  /* tag 2: two masters, independent of tag 1 */
  m.tagstate->curtag = 2;
  m.tagstate->nmasters[2] = 2;
  assert_int_equal(layout_place(&m, LAYOUT_TILE, &opts, p, 4), 3);
  assert_box(p[0].box, 0, 0, 400, 300);
  assert_box(p[1].box, 0, 300, 400, 300);

  /* switching back restores tag 1's setting */
  m.tagstate->curtag = 1;
  assert_int_equal(layout_place(&m, LAYOUT_TILE, &opts, p, 4), 3);
  assert_box(p[0].box, 0, 0, 400, 600);
}

/* ── Tiling order ownership ──────────────────────────────────────────────── */

static void test_promote_moves_to_master(void **state) {
  (void)state;
  layout_init();
  Monitor m;
  init_monitor(&m, 800, 600);
  Client c1, c2, c3;
  init_client(&c1, &m, 1u << 0, 0, 0);
  init_client(&c2, &m, 1u << 0, 0, 0);
  init_client(&c3, &m, 1u << 0, 0, 0);
  layout_insert(&c1);
  layout_insert(&c2);
  layout_insert(&c3);

  layout_promote(&c3);

  struct layout_opts opts = {.enablegaps = 0, .smartgaps = 0};
  struct Placement p[4];
  size_t n = layout_place(&m, LAYOUT_TILE, &opts, p, 4);

  assert_int_equal(n, 3);
  assert_ptr_equal(p[0].client, &c3);
  assert_box(p[0].box, 0, 0, 400, 600);
}

static void test_remove_drops_from_order(void **state) {
  (void)state;
  layout_init();
  Monitor m;
  init_monitor(&m, 800, 600);
  Client c1, c2, c3;
  init_client(&c1, &m, 1u << 0, 0, 0);
  init_client(&c2, &m, 1u << 0, 0, 0);
  init_client(&c3, &m, 1u << 0, 0, 0);
  layout_insert(&c1);
  layout_insert(&c2);
  layout_insert(&c3);

  layout_remove(&c2);

  struct layout_opts opts = {.enablegaps = 0, .smartgaps = 0};
  struct Placement p[4];
  size_t n = layout_place(&m, LAYOUT_TILE, &opts, p, 4);

  assert_int_equal(n, 2);
  assert_ptr_equal(p[0].client, &c1);
  assert_ptr_equal(p[1].client, &c3);
}

/* ── bounds clamping ──────────────────────────────────────────────────────── */

static void test_bounds_enforces_minimum_size(void **state) {
  (void)state;
  struct wlr_box box = {0, 0, 10, 10};
  struct wlr_box bbox = {0, 0, 100, 100};
  layout_bounds(&box, &bbox, 5);
  assert_int_equal(box.width, 11);
  assert_int_equal(box.height, 11);
}

static void test_bounds_repositions_out_of_bounds(void **state) {
  (void)state;
  struct wlr_box box = {200, 200, 50, 50};
  struct wlr_box bbox = {0, 0, 100, 100};
  layout_bounds(&box, &bbox, 0);
  assert_int_equal(box.x, 50);
  assert_int_equal(box.y, 50);
}

/* ── main ─────────────────────────────────────────────────────────────────── */

int main(void) {
  const struct CMUnitTest tests[] = {
      cmocka_unit_test(test_tile_master_stack),
      cmocka_unit_test(test_tile_nmaster_two),
      cmocka_unit_test(test_tile_gaps),
      cmocka_unit_test(test_tile_smartgaps_single_window),
      cmocka_unit_test(test_tile_excludes_floating_and_fullscreen),
      cmocka_unit_test(test_tile_accumulator_uses_clamped_height),
      cmocka_unit_test(test_monocle_fills_monitor),
      cmocka_unit_test(test_visible_follows_view_tagset),
      cmocka_unit_test(test_tagstate_is_per_tag),
      cmocka_unit_test(test_promote_moves_to_master),
      cmocka_unit_test(test_remove_drops_from_order),
      cmocka_unit_test(test_bounds_enforces_minimum_size),
      cmocka_unit_test(test_bounds_repositions_out_of_bounds),
  };
  return cmocka_run_group_tests(tests, NULL, NULL);
}

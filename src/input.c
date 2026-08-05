/* Input module: ownership of input handler functions — keyboard, pointer,
 * cursor events, virtual devices, pointer constraints, drag and selection.
 *
 * Owns:
 *   - All input handler functions (listener callbacks)
 *   - Module-scoped state: kb_group, cursor_shape_mgr, virtual_keyboard_mgr,
 *     virtual_pointer_mgr, pointer_constraints, active_constraint,
 *     relative_pointer_mgr, drag_icon, grabcx, grabcy, gpu_reset listener
 *
 * Does NOT own:
 *   - cursor, cursor_mgr, cursor_mode, grabc, event_loop (shared compositor
 *     state used by bonsaiwm.c for motionnotify and moveresize)
 *   - motionnotify, xytonode, gpureset (stay in bonsaiwm.c, cross-module)
 */
#include <math.h>
#include <stdlib.h>

#include <libinput.h>
#include <wlr/backend/libinput.h>
#include <wayland-server-core.h>
#include <wlr/types/wlr_cursor.h>
#include <wlr/types/wlr_cursor_shape_v1.h>
#include <wlr/types/wlr_data_device.h>
#include <wlr/types/wlr_idle_notify_v1.h>
#include <wlr/types/wlr_keyboard.h>
#include <wlr/types/wlr_keyboard_group.h>
#include <wlr/types/wlr_pointer_constraints_v1.h>
#include <wlr/types/wlr_primary_selection.h>
#include <wlr/types/wlr_relative_pointer_v1.h>
#include <wlr/types/wlr_seat.h>
#include <wlr/types/wlr_virtual_keyboard_v1.h>
#include <wlr/types/wlr_virtual_pointer_v1.h>
#include <wlr/types/wlr_xcursor_manager.h>
#include <wlr/util/log.h>
#include <xkbcommon/xkbcommon.h>
#include <scenefx/types/wlr_scene.h>

#include "input.h"
#include "client.h"
#include "config.h"
#include "focus.h"
#include "util.h"

extern KeyboardGroup *kb_group;

static struct wlr_cursor_shape_manager_v1 *cursor_shape_mgr;
static struct wlr_virtual_keyboard_manager_v1 *virtual_keyboard_mgr;
static struct wlr_virtual_pointer_manager_v1 *virtual_pointer_mgr;
static struct wlr_pointer_constraints_v1 *pointer_constraints;
static struct wlr_pointer_constraint_v1 *active_constraint;
static struct wlr_relative_pointer_manager_v1 *relative_pointer_mgr;
static struct wlr_scene_tree *drag_icon;

extern struct wlr_cursor *cursor;
extern unsigned int cursor_mode;
extern Client *grabc;
extern int grabcx, grabcy;
extern struct wl_event_loop *event_loop;

Monitor *xytomon(double x, double y);
void setmon(Client *c, Monitor *m, uint32_t newtags);
void xytonode(double x, double y, struct wlr_surface **psurface, Client **pc,
              LayerSurface **pl, double *nx, double *ny);
static void axisnotify(struct wl_listener *listener, void *data);
static void buttonpress(struct wl_listener *listener, void *data);
static void createkeyboard(struct wlr_keyboard *keyboard);
KeyboardGroup *createkeyboardgroup(void);
static void createpointer(struct wlr_pointer *pointer);
static void createpointerconstraint(struct wl_listener *listener, void *data);
void cursorconstrain(struct wlr_pointer_constraint_v1 *constraint);
static void cursorframe(struct wl_listener *listener, void *data);
static void cursorwarptohint(void);
void destroydragicon(struct wl_listener *listener, void *data);
static void destroypointerconstraint(struct wl_listener *listener, void *data);
void destroykeyboardgroup(struct wl_listener *listener, void *data);
static void inputdevice(struct wl_listener *listener, void *data);
int keybinding(uint32_t mods, xkb_keysym_t sym);
static void keypress(struct wl_listener *listener, void *data);
static void keypressmod(struct wl_listener *listener, void *data);
static int keyrepeat(void *data);
static void motionabsolute(struct wl_listener *listener, void *data);
static void motionrelative(struct wl_listener *listener, void *data);
void pointerfocus(Client *c, struct wlr_surface *surface, double sx, double sy,
                  uint32_t time);
static void setcursor(struct wl_listener *listener, void *data);
static void setcursorshape(struct wl_listener *listener, void *data);
static void setpsel(struct wl_listener *listener, void *data);
static void setsel(struct wl_listener *listener, void *data);
static void virtualkeyboard(struct wl_listener *listener, void *data);
static void virtualpointer(struct wl_listener *listener, void *data);

static struct wl_listener cursor_axis = {.notify = axisnotify};
static struct wl_listener cursor_button = {.notify = buttonpress};
static struct wl_listener cursor_frame = {.notify = cursorframe};
static struct wl_listener cursor_motion = {.notify = motionrelative};
static struct wl_listener cursor_motion_absolute = {.notify = motionabsolute};
static struct wl_listener new_input_device = {.notify = inputdevice};
static struct wl_listener new_virtual_keyboard = {.notify = virtualkeyboard};
static struct wl_listener new_virtual_pointer = {.notify = virtualpointer};
static struct wl_listener new_pointer_constraint = {
    .notify = createpointerconstraint};
static struct wl_listener request_cursor = {.notify = setcursor};
static struct wl_listener request_set_psel = {.notify = setpsel};
static struct wl_listener request_set_sel = {.notify = setsel};
static struct wl_listener request_set_cursor_shape = {.notify = setcursorshape};

void requeststartdrag(struct wl_listener *listener, void *data);
void startdrag(struct wl_listener *listener, void *data);

void requeststartdrag(struct wl_listener *listener, void *data) {
  struct wlr_seat_request_start_drag_event *event = data;

  if (wlr_seat_validate_pointer_grab_serial(seat, event->origin, event->serial))
    wlr_seat_start_pointer_drag(seat, event->drag, event->serial);
  else
    wlr_data_source_destroy(event->drag->source);
}

void startdrag(struct wl_listener *listener, void *data) {
  struct wlr_drag *drag = data;
  if (!drag->icon)
    return;

  drag->icon->data = &wlr_scene_drag_icon_create(drag_icon, drag->icon)->node;
  LISTEN_STATIC(&drag->icon->events.destroy, destroydragicon);
}

void motionnotify(uint32_t time, struct wlr_input_device *device, double dx,
                  double dy, double dx_unaccel, double dy_unaccel);

static void axisnotify(struct wl_listener *listener, void *data) {
  struct wlr_pointer_axis_event *event = data;
  wlr_idle_notifier_v1_notify_activity(idle_notifier, seat);
  wlr_seat_pointer_notify_axis(seat, event->time_msec, event->orientation,
                               event->delta, event->delta_discrete,
                               event->source, event->relative_direction);
}

static void buttonpress(struct wl_listener *listener, void *data) {
  struct wlr_pointer_button_event *event = data;
  struct wlr_keyboard *keyboard;
  uint32_t mods;
  Client *c;
  const Button *b;

  wlr_idle_notifier_v1_notify_activity(idle_notifier, seat);

  switch (event->state) {
  case WL_POINTER_BUTTON_STATE_PRESSED:
    cursor_mode = CurPressed;
    selmon = xytomon(cursor->x, cursor->y);
    if (locked)
      break;

    xytonode(cursor->x, cursor->y, NULL, &c, NULL, NULL, NULL);
    if (c && (!client_is_unmanaged(c) || client_wants_focus(c)))
      focusclient(c, 1);

    keyboard = wlr_seat_get_keyboard(seat);
    mods = keyboard ? wlr_keyboard_get_modifiers(keyboard) : 0;
    for (b = buttons; b < buttons + buttons_count; b++) {
      if (CLEANMASK(mods) == CLEANMASK(b->mod) && event->button == b->button &&
          b->func) {
        b->func(&b->arg);
        return;
      }
    }
    break;
  case WL_POINTER_BUTTON_STATE_RELEASED:
    if (!locked && cursor_mode != CurNormal && cursor_mode != CurPressed) {
      wlr_cursor_set_xcursor(cursor, cursor_mgr, "default");
      cursor_mode = CurNormal;
      selmon = xytomon(cursor->x, cursor->y);
      setmon(grabc, selmon, 0);
      grabc = NULL;
      return;
    }
    cursor_mode = CurNormal;
    break;
  }
  wlr_seat_pointer_notify_button(seat, event->time_msec, event->button,
                                 event->state);
}

static void createkeyboard(struct wlr_keyboard *keyboard) {
  wlr_keyboard_set_keymap(keyboard, kb_group->wlr_group->keyboard.keymap);
  wlr_keyboard_group_add_keyboard(kb_group->wlr_group, keyboard);
}

KeyboardGroup *createkeyboardgroup(void) {
  KeyboardGroup *group = ecalloc(1, sizeof(*group));
  struct xkb_context *context;
  struct xkb_keymap *keymap;

  group->wlr_group = wlr_keyboard_group_create();
  group->wlr_group->data = group;

  context = xkb_context_new(XKB_CONTEXT_NO_FLAGS);
  if (!(keymap = xkb_keymap_new_from_names(context, &xkb_rules,
                                           XKB_KEYMAP_COMPILE_NO_FLAGS)))
    die("failed to compile keymap");

  wlr_keyboard_set_keymap(&group->wlr_group->keyboard, keymap);
  xkb_keymap_unref(keymap);
  xkb_context_unref(context);

  wlr_keyboard_set_repeat_info(&group->wlr_group->keyboard, config.repeat_rate,
                               config.repeat_delay);

  LISTEN(&group->wlr_group->keyboard.events.key, &group->key, keypress);
  LISTEN(&group->wlr_group->keyboard.events.modifiers, &group->modifiers,
         keypressmod);

  group->key_repeat_source =
      wl_event_loop_add_timer(event_loop, keyrepeat, group);

  wlr_seat_set_keyboard(seat, &group->wlr_group->keyboard);
  return group;
}

static void createpointer(struct wlr_pointer *pointer) {
  struct libinput_device *device;
  if (wlr_input_device_is_libinput(&pointer->base) &&
      (device = wlr_libinput_get_device_handle(&pointer->base))) {

    if (libinput_device_config_tap_get_finger_count(device)) {
      libinput_device_config_tap_set_enabled(device, tap_to_click);
      libinput_device_config_tap_set_drag_enabled(device, tap_and_drag);
      libinput_device_config_tap_set_drag_lock_enabled(device, drag_lock);
      libinput_device_config_tap_set_button_map(device, button_map);
    }

    if (libinput_device_config_scroll_has_natural_scroll(device))
      libinput_device_config_scroll_set_natural_scroll_enabled(
          device, natural_scrolling);

    if (libinput_device_config_dwt_is_available(device))
      libinput_device_config_dwt_set_enabled(device, disable_while_typing);

    if (libinput_device_config_left_handed_is_available(device))
      libinput_device_config_left_handed_set(device, left_handed);

    if (libinput_device_config_middle_emulation_is_available(device))
      libinput_device_config_middle_emulation_set_enabled(
          device, middle_button_emulation);

    if (libinput_device_config_scroll_get_methods(device) !=
        LIBINPUT_CONFIG_SCROLL_NO_SCROLL)
      libinput_device_config_scroll_set_method(device, scroll_method);

    if (libinput_device_config_click_get_methods(device) !=
        LIBINPUT_CONFIG_CLICK_METHOD_NONE)
      libinput_device_config_click_set_method(device, click_method);

    if (libinput_device_config_send_events_get_modes(device))
      libinput_device_config_send_events_set_mode(device, send_events_mode);

    if (libinput_device_config_accel_is_available(device)) {
      libinput_device_config_accel_set_profile(device, accel_profile);
      libinput_device_config_accel_set_speed(device, accel_speed);
    }
  }

  wlr_cursor_attach_input_device(cursor, &pointer->base);
}

static void createpointerconstraint(struct wl_listener *listener, void *data) {
  PointerConstraint *pointer_constraint =
      ecalloc(1, sizeof(*pointer_constraint));
  pointer_constraint->constraint = data;
  LISTEN(&pointer_constraint->constraint->events.destroy,
         &pointer_constraint->destroy, destroypointerconstraint);
}

void cursorconstrain(struct wlr_pointer_constraint_v1 *constraint) {
  if (active_constraint == constraint)
    return;

  if (active_constraint)
    wlr_pointer_constraint_v1_send_deactivated(active_constraint);

  active_constraint = constraint;
  wlr_pointer_constraint_v1_send_activated(constraint);
}

static void cursorframe(struct wl_listener *listener, void *data) {
  wlr_seat_pointer_notify_frame(seat);
}

static void cursorwarptohint(void) {
  Client *c = NULL;
  double sx = active_constraint->current.cursor_hint.x;
  double sy = active_constraint->current.cursor_hint.y;

  toplevel_from_wlr_surface(active_constraint->surface, &c, NULL);
  if (c && active_constraint->current.cursor_hint.enabled) {
    wlr_cursor_warp(cursor, NULL, sx + c->geom.x + c->bw,
                    sy + c->geom.y + c->bw);
    wlr_seat_pointer_warp(active_constraint->seat, sx, sy);
  }
}

void destroydragicon(struct wl_listener *listener, void *data) {
  focusclient(focustop(selmon), 1);
  motionnotify(0, NULL, 0, 0, 0, 0);
  wl_list_remove(&listener->link);
  free(listener);
}

static void destroypointerconstraint(struct wl_listener *listener, void *data) {
  PointerConstraint *pointer_constraint =
      wl_container_of(listener, pointer_constraint, destroy);

  if (active_constraint == pointer_constraint->constraint) {
    cursorwarptohint();
    active_constraint = NULL;
  }

  wl_list_remove(&pointer_constraint->destroy.link);
  free(pointer_constraint);
}

void destroykeyboardgroup(struct wl_listener *listener, void *data) {
  KeyboardGroup *group = wl_container_of(listener, group, destroy);
  wl_event_source_remove(group->key_repeat_source);
  wl_list_remove(&group->key.link);
  wl_list_remove(&group->modifiers.link);
  wl_list_remove(&group->destroy.link);
  wlr_keyboard_group_destroy(group->wlr_group);
  free(group);
}

static void inputdevice(struct wl_listener *listener, void *data) {
  struct wlr_input_device *device = data;
  uint32_t caps;

  switch (device->type) {
  case WLR_INPUT_DEVICE_KEYBOARD:
    createkeyboard(wlr_keyboard_from_input_device(device));
    break;
  case WLR_INPUT_DEVICE_POINTER:
    createpointer(wlr_pointer_from_input_device(device));
    break;
  default:
    break;
  }

  caps = WL_SEAT_CAPABILITY_POINTER;
  if (!wl_list_empty(&kb_group->wlr_group->devices))
    caps |= WL_SEAT_CAPABILITY_KEYBOARD;
  wlr_seat_set_capabilities(seat, caps);
}

int keybinding(uint32_t mods, xkb_keysym_t sym) {
  const Key *k;
  for (k = keys; k < keys + keys_count; k++) {
    if (CLEANMASK(mods) == CLEANMASK(k->mod) &&
        xkb_keysym_to_lower(sym) == xkb_keysym_to_lower(k->keysym) && k->func) {
      k->func(&k->arg);
      return 1;
    }
  }
  return 0;
}

static void keypress(struct wl_listener *listener, void *data) {
  int i;
  KeyboardGroup *group = wl_container_of(listener, group, key);
  struct wlr_keyboard_key_event *event = data;

  uint32_t keycode = event->keycode + 8;
  const xkb_keysym_t *syms;
  int nsyms = xkb_state_key_get_syms(group->wlr_group->keyboard.xkb_state,
                                     keycode, &syms);

  int handled = 0;
  uint32_t mods = wlr_keyboard_get_modifiers(&group->wlr_group->keyboard);

  wlr_idle_notifier_v1_notify_activity(idle_notifier, seat);

  if (!locked && event->state == WL_KEYBOARD_KEY_STATE_PRESSED) {
    for (i = 0; i < nsyms; i++)
      handled = keybinding(mods, syms[i]) || handled;
  }

  if (handled && group->wlr_group->keyboard.repeat_info.delay > 0) {
    group->mods = mods;
    group->keysyms = syms;
    group->nsyms = nsyms;
    wl_event_source_timer_update(group->key_repeat_source,
                                 group->wlr_group->keyboard.repeat_info.delay);
  } else {
    group->nsyms = 0;
    wl_event_source_timer_update(group->key_repeat_source, 0);
  }

  if (handled)
    return;

  wlr_seat_set_keyboard(seat, &group->wlr_group->keyboard);
  wlr_seat_keyboard_notify_key(seat, event->time_msec, event->keycode,
                               event->state);
}

static void keypressmod(struct wl_listener *listener, void *data) {
  KeyboardGroup *group = wl_container_of(listener, group, modifiers);

  wlr_seat_set_keyboard(seat, &group->wlr_group->keyboard);
  wlr_seat_keyboard_notify_modifiers(seat,
                                     &group->wlr_group->keyboard.modifiers);
}

static int keyrepeat(void *data) {
  KeyboardGroup *group = data;
  int i;
  if (!group->nsyms || group->wlr_group->keyboard.repeat_info.rate <= 0)
    return 0;

  wl_event_source_timer_update(group->key_repeat_source,
                               1000 /
                                   group->wlr_group->keyboard.repeat_info.rate);

  for (i = 0; i < group->nsyms; i++)
    keybinding(group->mods, group->keysyms[i]);

  return 0;
}

static void motionabsolute(struct wl_listener *listener, void *data) {
  struct wlr_pointer_motion_absolute_event *event = data;
  double lx, ly, dx, dy;

  if (!event->time_msec)
    wlr_cursor_warp_absolute(cursor, &event->pointer->base, event->x, event->y);

  wlr_cursor_absolute_to_layout_coords(cursor, &event->pointer->base, event->x,
                                       event->y, &lx, &ly);
  dx = lx - cursor->x;
  dy = ly - cursor->y;
  motionnotify(event->time_msec, &event->pointer->base, dx, dy, dx, dy);
}

static void motionrelative(struct wl_listener *listener, void *data) {
  struct wlr_pointer_motion_event *event = data;
  motionnotify(event->time_msec, &event->pointer->base, event->delta_x,
               event->delta_y, event->unaccel_dx, event->unaccel_dy);
}

void pointerfocus(Client *c, struct wlr_surface *surface, double sx, double sy,
                  uint32_t time) {
  struct timespec now;

  if (surface != seat->pointer_state.focused_surface && config.sloppyfocus &&
      time && c && !client_is_unmanaged(c))
    focusclient(c, 0);

  if (!surface) {
    wlr_seat_pointer_notify_clear_focus(seat);
    return;
  }

  if (!time) {
    clock_gettime(CLOCK_MONOTONIC, &now);
    time = now.tv_sec * 1000 + now.tv_nsec / 1000000;
  }

  wlr_seat_pointer_notify_enter(seat, surface, sx, sy);
  wlr_seat_pointer_notify_motion(seat, time, sx, sy);
}

static void setcursor(struct wl_listener *listener, void *data) {
  struct wlr_seat_pointer_request_set_cursor_event *event = data;
  if (cursor_mode != CurNormal && cursor_mode != CurPressed)
    return;
  if (event->seat_client == seat->pointer_state.focused_client)
    wlr_cursor_set_surface(cursor, event->surface, event->hotspot_x,
                           event->hotspot_y);
}

static void setcursorshape(struct wl_listener *listener, void *data) {
  struct wlr_cursor_shape_manager_v1_request_set_shape_event *event = data;
  if (cursor_mode != CurNormal && cursor_mode != CurPressed)
    return;
  if (event->seat_client == seat->pointer_state.focused_client)
    wlr_cursor_set_xcursor(cursor, cursor_mgr,
                           wlr_cursor_shape_v1_name(event->shape));
}

static void setpsel(struct wl_listener *listener, void *data) {
  struct wlr_seat_request_set_primary_selection_event *event = data;
  wlr_seat_set_primary_selection(seat, event->source, event->serial);
}

static void setsel(struct wl_listener *listener, void *data) {
  struct wlr_seat_request_set_selection_event *event = data;
  wlr_seat_set_selection(seat, event->source, event->serial);
}

static void virtualkeyboard(struct wl_listener *listener, void *data) {
  struct wlr_virtual_keyboard_v1 *kb = data;
  KeyboardGroup *group = createkeyboardgroup();
  wlr_keyboard_set_keymap(&kb->keyboard, group->wlr_group->keyboard.keymap);
  LISTEN(&kb->keyboard.base.events.destroy, &group->destroy,
         destroykeyboardgroup);

  wlr_keyboard_group_add_keyboard(group->wlr_group, &kb->keyboard);
}

static void virtualpointer(struct wl_listener *listener, void *data) {
  struct wlr_virtual_pointer_v1_new_pointer_event *event = data;
  struct wlr_input_device *device = &event->new_pointer->pointer.base;

  wlr_cursor_attach_input_device(cursor, device);
  if (event->suggested_output)
    wlr_cursor_map_input_to_output(cursor, device, event->suggested_output);
}

static struct wl_listener request_start_drag = {.notify = requeststartdrag};
static struct wl_listener start_drag = {.notify = startdrag};
void input_init(void) {
  cursor = wlr_cursor_create();
  wlr_cursor_attach_output_layout(cursor, output_layout);

  cursor_mgr = wlr_xcursor_manager_create(NULL, 24);
  setenv("XCURSOR_SIZE", "24", 1);

  wl_signal_add(&cursor->events.motion, &cursor_motion);
  wl_signal_add(&cursor->events.motion_absolute, &cursor_motion_absolute);
  wl_signal_add(&cursor->events.button, &cursor_button);
  wl_signal_add(&cursor->events.axis, &cursor_axis);
  wl_signal_add(&cursor->events.frame, &cursor_frame);

  cursor_shape_mgr = wlr_cursor_shape_manager_v1_create(dpy, 1);
  wl_signal_add(&cursor_shape_mgr->events.request_set_shape,
                &request_set_cursor_shape);

  wl_signal_add(&backend->events.new_input, &new_input_device);
  virtual_keyboard_mgr = wlr_virtual_keyboard_manager_v1_create(dpy);
  wl_signal_add(&virtual_keyboard_mgr->events.new_virtual_keyboard,
                &new_virtual_keyboard);
  virtual_pointer_mgr = wlr_virtual_pointer_manager_v1_create(dpy);
  wl_signal_add(&virtual_pointer_mgr->events.new_virtual_pointer,
                &new_virtual_pointer);

  seat = wlr_seat_create(dpy, "seat0");
  wl_signal_add(&seat->events.request_set_cursor, &request_cursor);
  wl_signal_add(&seat->events.request_set_selection, &request_set_sel);
  wl_signal_add(&seat->events.request_set_primary_selection, &request_set_psel);
  wl_signal_add(&seat->events.request_start_drag, &request_start_drag);
  wl_signal_add(&seat->events.start_drag, &start_drag);

  pointer_constraints = wlr_pointer_constraints_v1_create(dpy);
  wl_signal_add(&pointer_constraints->events.new_constraint,
                &new_pointer_constraint);
  relative_pointer_mgr = wlr_relative_pointer_manager_v1_create(dpy);

  kb_group = createkeyboardgroup();
  wl_list_init(&kb_group->destroy.link);
}

void input_cleanup(void) {
  wl_list_remove(&cursor_axis.link);
  wl_list_remove(&cursor_button.link);
  wl_list_remove(&cursor_frame.link);
  wl_list_remove(&cursor_motion.link);
  wl_list_remove(&cursor_motion_absolute.link);
  wl_list_remove(&new_input_device.link);
  wl_list_remove(&new_virtual_keyboard.link);
  wl_list_remove(&new_virtual_pointer.link);
  wl_list_remove(&new_pointer_constraint.link);
  wl_list_remove(&request_cursor.link);
  wl_list_remove(&request_set_psel.link);
  wl_list_remove(&request_set_sel.link);
  wl_list_remove(&request_set_cursor_shape.link);
  wl_list_remove(&request_start_drag.link);
  wl_list_remove(&start_drag.link);
  wl_list_remove(&new_pointer_constraint.link);
}

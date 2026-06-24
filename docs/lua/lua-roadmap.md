---
title: Integration roadmap
---

# Lua API Roadmap

Move appearance, behavior, rules, monitor config, and keybinds to `config.lua`,
toward zero `config.h` edits.

`config.h` stays in C as a boot-safe fallback. The user never edits it. If
`config.lua` is missing or fails to load, C defaults keep the WM usable.

| Status | Step | Description | Commit |
|--------|------|-------------|--------|
| ✓ | Window lifecycle & focus hooks | `client_mapped` (replaces `rules[]`; returns `{ tags, floating, monitor }` table), `client_unmapped` (merged `client_destroyed`), `focus_change` |  |
| ✓ | Monitor hooks | `monitor_created` (replaces user-editable `monrules[]`; returns `{ scale, mfact, nmaster, layout, transform, x, y }` table), `monitor_destroyed` |  |
| ✓ | Configuration functions | Setters for appearance (`set_border_width`, `set_border_color`, `set_focus_color`, `set_urgent_color`, `set_root_color`; done), gaps (`set_gaps`; done, `adjust_gaps`, `default_gaps`; done), master/stack (`set_mfact`, `adjust_mfact`, `set_nmaster`, `adjust_nmaster`; done), behavior (`set_sloppy_focus`, `set_smart_gaps`; done). `set_fullscreen_bg` dropped — C default stays, most users run a wallpaper daemon | `19d55a7` `e284823` `1efad01` |
|  | Layout setters | `set_layout`. Depends on the Layout table step — exposing it prematurely would create throwaway API since the C `setlayout` takes a `Layout*`. Lands alongside the layout-table rework |  |
|  | Additional hooks | `client_title`, `client_urgent`, `client_monitor_changed` (all observational). Enhanced `tag_switch` passes tag number (1-9). `arrange` hook replaced by layout table |  |
|  | Layout table | `bonsaiwm.layouts = { T = bonsaiwm.tile, F = bonsaiwm.floating, M = bonsaiwm.monocle, Z = function() ... end }`. C layout functions exposed as zero-arg `bonsaiwm.tile`, `bonsaiwm.monocle`, `bonsaiwm.floating` (use current state). Layout table is global (per-monitor, matches existing dwl model). Per-tag behavior via `tag_switch` hook. C `layouts[]` stays as fallback |  |
|  | Action functions | Every C keybind action exposed as flat `bonsaiwm.<name>()` with clearer snake_case names: `focus_next`, `kill_client`, `toggle_floating`, `toggle_fullscreen`, `view`, `toggle_view`, `tag`, `toggle_tag`, `focus_monitor`, `move_to_monitor`, `quit`, `switch_vt`, `zoom`, `move_resize`. Tag functions use 1-indexed numbers (`view(4)`, `view(0)` = all tags). Mouse: no `bind_mouse` API, `buttons[]` stays in C, `move_resize("move"/"resize")` exposed as function |  |
|  | Keybind priority | Lua keybinds take priority over C `keys[]` (reversed from current). C `keys[]` stays as boot-safe fallback. Shift+number auto-resolved (bind `"1"` with shift modifier). `bonsaiwm.reload()` takes no argument, reloads from config path |  |
|  | config.h extermination | Deleted from `config.h`: `rules[]`, `MODKEY`, `TAGKEYS`, `SHCMD`, `termcmd`, `menucmd`. Moved to Lua: `borderpx`, colors, gaps, `sloppyfocus`, `smartgaps`, `enablegaps`, `fullscreen_bg`, `repeat_rate`, `repeat_delay`, `xkb_rules`, all input settings (tap_to_click, natural_scrolling, etc.). Stays in C as fallback: `keys[]`, `buttons[]`, `monrules[]` (one default rule), `layouts[]` (overridable), `TAGCOUNT`, `log_level`, `bypass_surface_visibility`, `COLOR` macro, input settings (as defaults) |  |
| ✓ | Keybindings in Lua | `bonsaiwm.bind_key()` + `bonsaiwm.mod` table |  |
|  | Config loading | Single `config.lua` loaded before `wlr_backend_start` (input settings and hooks registered before devices appear). Config path: `$XDG_CONFIG_HOME/bonsaiwm/config.lua`, overridable with `-c` flag, falls back to `~/.config/bonsaiwm/config.lua`. On reload: re-run config, reapply input settings to all existing devices, reapply appearance, arrange all monitors |  |

---
title: Lua

---
# bonsaiwm Lua API Reference

This document describes the Lua API exposed by bonsaiwm.

## `bonsaiwm.set_gaps`

Set outer and inner gaps on the selected monitor. Values are in pixels.
Outer gaps pad the screen edges. Inner gaps go between windows.

| Parameter | Type | Description |
|-----------|------|-------------|
| `oh` | `integer` | Outer gap, horizontal (left/right screen edge) |
| `ov` | `integer` | Outer gap, vertical (top/bottom screen edge) |
| `ih` | `integer` | Inner gap, horizontal (between columns) |
| `iv` | `integer` | Inner gap, vertical (between rows) |

---

## `bonsaiwm.adjust_gaps`

Adjust all four gaps by the same delta, in pixels.
Positive values grow the gaps, negative values shrink them. Clamped to 0.

| Parameter | Type | Description |
|-----------|------|-------------|
| `delta` | `integer` | Pixels to add (or subtract) from every gap |

---

## `bonsaiwm.default_gaps`

Reset all gaps to the config defaults.

---

## `bonsaiwm.set_border_width`

Set border width in pixels on all clients.

| Parameter | Type | Description |
|-----------|------|-------------|
| `px` | `integer` | Border width in pixels |

---

## `bonsaiwm.set_border_color`

Set border color.

| Parameter | Type | Description |
|-----------|------|-------------|
| `r` | `integer` | Red (0-255) |
| `g` | `integer` | Green (0-255) |
| `b` | `integer` | Blue (0-255) |
| `a` | `number` | Alpha (0-1) |

---

## `bonsaiwm.set_focus_color`

Set focused border color.

| Parameter | Type | Description |
|-----------|------|-------------|
| `r` | `integer` | Red (0-255) |
| `g` | `integer` | Green (0-255) |
| `b` | `integer` | Blue (0-255) |
| `a` | `number` | Alpha (0-1) |

---

## `bonsaiwm.set_urgent_color`

Set urgent border color.

| Parameter | Type | Description |
|-----------|------|-------------|
| `r` | `integer` | Red (0-255) |
| `g` | `integer` | Green (0-255) |
| `b` | `integer` | Blue (0-255) |
| `a` | `number` | Alpha (0-1) |

---

## `bonsaiwm.set_root_color`

Set desktop background color.

| Parameter | Type | Description |
|-----------|------|-------------|
| `r` | `integer` | Red (0-255) |
| `g` | `integer` | Green (0-255) |
| `b` | `integer` | Blue (0-255) |
| `a` | `number` | Alpha (0-1) |

---

## `bonsaiwm.log`

Print a message to stderr with a [bonsaiwm] prefix.
Use this for debugging your config.lua.

| Parameter | Type | Description |
|-----------|------|-------------|
| `msg` | `string` | Message to print |

---

## `bonsaiwm.exec`

Run a shell command. Runs every time the config is (re)loaded.
For long-running daemons you only want once, use exec_once instead.

| Parameter | Type | Description |
|-----------|------|-------------|
| `cmd` | `string` | Shell command to execute |

---

## `bonsaiwm.exec_once`

Run a shell command, but only on the first config load.
Skipped on reloads so you don't respawn waybar, wallpaper daemons, etc.

| Parameter | Type | Description |
|-----------|------|-------------|
| `cmd` | `string` | Shell command to execute once |

---

## `bonsaiwm.spawn`

Spawn a long-lived process via /bin/sh -c.
Forks and detaches so the child can't block the compositor.

| Parameter | Type | Description |
|-----------|------|-------------|
| `cmd` | `string` | Shell command to execute |

---

## `bonsaiwm.bind_key`

Register a keybinding. Calls fn when (mod, key) is pressed.
key is resolved via xkb_keysym_from_name at bind time (e.g. "Return", "a").

| Parameter | Type | Description |
|-----------|------|-------------|
| `mod` | `integer` | Modifier bitmask; see the `bonsaiwm.mod` constants below |
| `key` | `string` | Key name, resolved via xkb_keysym_from_name |
| `fn` | `function` | Called with no arguments when the key is pressed |

---

## `bonsaiwm.on`

Register a Lua callback for a compositor event.
Only one hook per event is kept — registering again replaces the old one.
If the callback errors, the message is logged to stderr and execution continues.
Available events:
- `"arrange"`: layout changed. Receives the layout symbol string.
- `"tag_switch"`: viewed tags changed. Receives no arguments.
- `"client_mapped"`: a new client window appeared. Receives app_id and title.
- `"client_unmapped"`: a client window was hidden/unmapped. Receives app_id and title.
- `"client_destroyed"`: a client window was destroyed. Receives app_id and title.
- `"focus_change"`: focused client changed. Receives new and old app_id/title (nil if none).
- `"monitor_created"`: a new output was connected. Receives the monitor name.
- `"monitor_destroyed"`: an output was disconnected. Receives the monitor name.

```lua
function bonsaiwm.on(event: "arrange", callback: fun(layout: string))
```

```lua
function bonsaiwm.on(event: "tag_switch", callback: fun())
```

```lua
function bonsaiwm.on(event: "client_mapped", callback: fun(app_id: string, title: string))
```

```lua
function bonsaiwm.on(event: "client_unmapped", callback: fun(app_id: string, title: string))
```

```lua
function bonsaiwm.on(event: "client_destroyed", callback: fun(app_id: string, title: string))
```

```lua
function bonsaiwm.on(event: "focus_change", callback: fun(new_app: string?, new_title: string?, old_app: string?, old_title: string?))
```

```lua
function bonsaiwm.on(event: "monitor_created", callback: fun(name: string))
```

```lua
function bonsaiwm.on(event: "monitor_destroyed", callback: fun(name: string))
```

| Parameter | Type | Description |
|-----------|------|-------------|
| `event` | `string` | Event name |
| `callback` | `function` | Called when the event fires |

---

## Constants: `bonsaiwm.mod`

| Name | Type | Description |
|------|------|-------------|
| `shift` | `integer` | Left or right Shift key. |
| `caps` | `integer` | Caps Lock key. |
| `ctrl` | `integer` | Left or right Control key. |
| `alt` | `integer` | Left or right Alt key. The default MODKEY used by config.h. |
| `mod2` | `integer` | Mod2 — typically Num Lock. |
| `logo` | `integer` | Super/Windows/Logo key. |

---

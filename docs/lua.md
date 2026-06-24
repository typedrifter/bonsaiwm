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

| Parameter | Type | Description |
|-----------|------|-------------|
| `event` | `string` | Event name |
| `callback` | `function` | Called when the event fires |

---

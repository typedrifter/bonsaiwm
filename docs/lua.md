---
title: Lua

---
# bonsaiwm Lua API Reference

This document describes the Lua API exposed by bonsaiwm.

For planned API additions, see the [Lua API Roadmap](LUA-ROADMAP.md).

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
- **"arrange"** — layout changed. Receives the layout symbol string.
- **"tag_switch"** — viewed tags changed. Receives no arguments.

```lua
function bonsaiwm.on(event: "arrange", callback: fun(layout: string))
```

```lua
function bonsaiwm.on(event: "tag_switch", callback: fun())
```

| Parameter | Type | Description |
|-----------|------|-------------|
| `event` | `string` | Event name |
| `callback` | `function` | Called when the event fires |

---

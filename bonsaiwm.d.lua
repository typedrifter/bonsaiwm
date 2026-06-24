---@class bonsaiwm
bonsaiwm = {}

---Set outer and inner gaps on the selected monitor. Values are in pixels.
---
---Outer gaps pad the screen edges. Inner gaps go between windows.
---@param oh integer Outer gap, horizontal (left/right screen edge)
---@param ov integer Outer gap, vertical (top/bottom screen edge)
---@param ih integer Inner gap, horizontal (between columns)
---@param iv integer Inner gap, vertical (between rows)
function bonsaiwm.set_gaps(oh, ov, ih, iv) end

---Adjust all four gaps by the same delta, in pixels.
---
---Positive values grow the gaps, negative values shrink them. Clamped to 0.
---@param delta integer Pixels to add (or subtract) from every gap
function bonsaiwm.adjust_gaps(delta) end

---Reset all gaps to the config defaults.
function bonsaiwm.default_gaps() end

---Set the master area ratio absolutely.
---
---Only meaningful on tiling layouts. Values outside 0.1-0.9 are rejected
---and logged to stderr; the existing mfact is left unchanged.
---@param f number Master area ratio (0.1 to 0.9)
function bonsaiwm.set_mfact(f) end

---Adjust the master area ratio by a delta.
---
---Only meaningful on tiling layouts. If the result would fall outside
---0.1-0.9 the change is rejected and logged to stderr.
---@param delta number Ratio delta (e.g. +0.05 or -0.05)
function bonsaiwm.adjust_mfact(delta) end

---Set the number of master windows absolutely on the selected monitor.
---
---Values are clamped to a minimum of 0.
---@param n integer Number of master windows (>= 0)
function bonsaiwm.set_nmaster(n) end

---Adjust the number of master windows by a delta on the selected monitor.
---
---Values are clamped to a minimum of 0.
---@param delta integer Number of master windows to add or remove (e.g. +1 or -1)
function bonsaiwm.adjust_nmaster(delta) end

---Set border width in pixels on all clients.
---@param px integer Border width in pixels
function bonsaiwm.set_border_width(px) end

---Set border color.
---@param r integer Red (0-255)
---@param g integer Green (0-255)
---@param b integer Blue (0-255)
---@param a number Alpha (0-1)
function bonsaiwm.set_border_color(r, g, b, a) end

---Set focused border color.
---@param r integer Red (0-255)
---@param g integer Green (0-255)
---@param b integer Blue (0-255)
---@param a number Alpha (0-1)
function bonsaiwm.set_focus_color(r, g, b, a) end

---Set urgent border color.
---@param r integer Red (0-255)
---@param g integer Green (0-255)
---@param b integer Blue (0-255)
---@param a number Alpha (0-1)
function bonsaiwm.set_urgent_color(r, g, b, a) end

---Set desktop background color.
---@param r integer Red (0-255)
---@param g integer Green (0-255)
---@param b integer Blue (0-255)
---@param a number Alpha (0-1)
function bonsaiwm.set_root_color(r, g, b, a) end

---Print a message to stderr with a [bonsaiwm] prefix.
---Use this for debugging your config.lua.
---@param msg string Message to print
function bonsaiwm.log(msg) end

---Run a shell command. Runs every time the config is (re)loaded.
---For long-running daemons you only want once, use exec_once instead.
---@param cmd string Shell command to execute
function bonsaiwm.exec(cmd) end

---Run a shell command, but only on the first config load.
---Skipped on reloads so you don't respawn waybar, wallpaper daemons, etc.
---@param cmd string Shell command to execute once
function bonsaiwm.exec_once(cmd) end

---Spawn a long-lived process via /bin/sh -c.
---Forks and detaches so the child can't block the compositor.
---@param cmd string Shell command to execute
function bonsaiwm.spawn(cmd) end

---Register a keybinding. Calls fn when (mod, key) is pressed.
---key is resolved via xkb_keysym_from_name at bind time (e.g. "Return", "a").
---@param mod integer Modifier bitmask; see the `bonsaiwm.mod` constants below
---@param key string Key name, resolved via xkb_keysym_from_name
---@param fn function Called with no arguments when the key is pressed
function bonsaiwm.bind_key(mod, key, fn) end

---Register a Lua callback for a compositor event.
---Only one hook per event is kept — registering again replaces the old one.
---If the callback errors, the message is logged to stderr and execution continues.
---
---Available events:
---
--- - `"arrange"`: layout changed. Receives the layout symbol string.
--- - `"tag_switch"`: viewed tags changed. Receives no arguments.
--- - `"client_mapped"`: a new client window appeared. Receives app_id and title.
--- - `"client_unmapped"`: a client window was hidden/unmapped. Receives app_id and title.
--- - `"client_destroyed"`: a client window was destroyed. Receives app_id and title.
--- - `"focus_change"`: focused client changed. Receives new and old app_id/title (nil if none).
--- - `"monitor_created"`: a new output was connected. Receives the monitor name.
--- - `"monitor_destroyed"`: an output was disconnected. Receives the monitor name.
---@overload fun(event: "arrange", callback: fun(layout: string))
---@overload fun(event: "tag_switch", callback: fun())
---@overload fun(event: "client_mapped", callback: fun(app_id: string, title: string))
---@overload fun(event: "client_unmapped", callback: fun(app_id: string, title: string))
---@overload fun(event: "client_destroyed", callback: fun(app_id: string, title: string))
---@overload fun(event: "focus_change", callback: fun(new_app: string?, new_title: string?, old_app: string?, old_title: string?))
---@overload fun(event: "monitor_created", callback: fun(name: string))
---@overload fun(event: "monitor_destroyed", callback: fun(name: string))
---@param event string Event name
---@param callback function Called when the event fires
function bonsaiwm.on(event, callback) end

---Modifier bitmask constants. Combine with bitwise OR (e.g. `bonsaiwm.mod.ctrl | bonsaiwm.mod.alt`).
---Referenced by `bonsaiwm.bind_key`.
---@class bonsaiwm.mod
bonsaiwm.mod = {}

---Left or right Shift key.
---@type integer
bonsaiwm.mod.shift = nil

---Caps Lock key.
---@type integer
bonsaiwm.mod.caps = nil

---Left or right Control key.
---@type integer
bonsaiwm.mod.ctrl = nil

---Left or right Alt key. The default MODKEY used by config.h.
---@type integer
bonsaiwm.mod.alt = nil

---Mod2 — typically Num Lock.
---@type integer
bonsaiwm.mod.mod2 = nil

---Super/Windows/Logo key.
---@type integer
bonsaiwm.mod.logo = nil

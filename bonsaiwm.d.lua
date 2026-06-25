---@meta
---@class bonsaiwm
bonsaiwm = {}

-- ── Static config ────────────────────────────────────────────────────────────

---@class bonsaiwm.config.appearance.colors
---@field root integer[]?
---@field border integer[]?
---@field focus integer[]?
---@field urgent integer[]?
---@field fullscreen integer[]?

---@class bonsaiwm.config.appearance
---@field border_width integer?
---@field sloppy_focus boolean?
---@field colors bonsaiwm.config.appearance.colors?

---@class bonsaiwm.config.gaps
---@field enabled boolean?
---@field smart boolean?
---@field outer_h integer?
---@field outer_v integer?
---@field inner_h integer?
---@field inner_v integer?

---@class bonsaiwm.config.keyboard.xkb_rules
---@field rules string?
---@field model string?
---@field layout string?
---@field variant string?
---@field options string?

---@class bonsaiwm.config.keyboard
---@field repeat_rate integer?
---@field repeat_delay integer?
---@field xkb_rules bonsaiwm.config.keyboard.xkb_rules?

---@class bonsaiwm.config.input
---@field tap_to_click boolean?
---@field natural_scrolling boolean?
---@field disable_while_typing boolean?
---@field accel_profile integer?
---@field accel_speed number?
---@field left_handed boolean?

---@class bonsaiwm.config
---@field appearance bonsaiwm.config.appearance?
---@field gaps bonsaiwm.config.gaps?
---@field keyboard bonsaiwm.config.keyboard?
---@field input bonsaiwm.config.input?
bonsaiwm.config = {}

-- ── Constants ────────────────────────────────────────────────────────────────

---Constant tables. Combine mod values with bitwise OR
---(e.g. `bonsaiwm.const.mod.ctrl | bonsaiwm.const.mod.alt`).
---@class bonsaiwm.const
bonsaiwm.const = {}

-- Modifier bitmask constants.
---@class bonsaiwm.const.mod
bonsaiwm.const.mod = {}

---Left or right Shift key.
---@type integer
bonsaiwm.const.mod.shift = nil

---Caps Lock key.
---@type integer
bonsaiwm.const.mod.caps = nil

---Left or right Control key.
---@type integer
bonsaiwm.const.mod.ctrl = nil

---Left or right Alt key. The default MODKEY used by config.h.
---@type integer
bonsaiwm.const.mod.alt = nil

---Mod2 — typically Num Lock.
---@type integer
bonsaiwm.const.mod.mod2 = nil

---Super/Windows/Logo key.
---@type integer
bonsaiwm.const.mod.logo = nil

---libinput acceleration profile constants.
---@class bonsaiwm.const.accel_profile
bonsaiwm.const.accel_profile = {}

---Adaptive acceleration profile.
---@type integer
bonsaiwm.const.accel_profile.adaptive = nil

---Flat acceleration profile.
---@type integer
bonsaiwm.const.accel_profile.flat = nil

-- ── Runtime imperative API ───────────────────────────────────────────────────

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

---Toggle focus-follows-mouse.
---
---When on, the window under the cursor receives focus as the pointer moves.
---@param v integer 1 = enable, 0 = disable
function bonsaiwm.set_sloppy_focus(v) end

---Toggle smart gaps.
---
---When on, outer gaps are suppressed when only one tiled window is visible.
---@param v integer 1 = enable, 0 = disable
function bonsaiwm.set_smart_gaps(v) end

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
---@param mod integer Modifier bitmask; see `bonsaiwm.const.mod`
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

---Request a config reload. Equivalent to the C MODKEY+Shift+r binding.
function bonsaiwm.reload() end

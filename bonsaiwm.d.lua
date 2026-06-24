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

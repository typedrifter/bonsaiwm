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
---- **"arrange"** — layout changed. Receives the layout symbol string.
---- **"tag_switch"** — viewed tags changed. Receives no arguments.
---@overload fun(event: "arrange", callback: fun(layout: string))
---@overload fun(event: "tag_switch", callback: fun())
---@param event string Event name
---@param callback function Called when the event fires
function bonsaiwm.on(event, callback) end

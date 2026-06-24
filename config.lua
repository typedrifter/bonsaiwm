print("lua config loaded")
bonsaiwm.log("bonsaiwm started")
bonsaiwm.set_gaps(30, 60, 20, 20)
bonsaiwm.set_border_width(3)
bonsaiwm.set_border_color(67, 245, 39, 0.1)
--
local M = bonsaiwm.mod.alt
local S = bonsaiwm.mod.shift

bonsaiwm.bind_key(M | S, "Return", function()
	bonsaiwm.log("Bind fired!")
	bonsaiwm.spawn("foot")
end)

bonsaiwm.exec_once("awww-daemon &")
bonsaiwm.exec_once("awww img ~/Pictures/Wallpapers/pexels-eberhardgross-640782.jpg &")
bonsaiwm.exec_once("waybar &")

-- bonsaiwm.on("tag_switch", function()
-- 	bonsaiwm.log("hello from hook !")
-- end)
--
-- bonsaiwm.on("client_mapped", function(app_id, title)
-- 	bonsaiwm.log("mapped: " .. (app_id or "unknown") .. " - " .. (title or "unknown"))
-- end)

bonsaiwm.on("arrange", function(a)
	if a == "Z" then
		bonsaiwm.set_gaps(60, 60, 40, 40)
	elseif a == "T" then
		bonsaiwm.set_gaps(20, 20, 20, 20)
	end
end)

-- bonsaiwm.on("focus_change", function(new_app, new_title, old_app, old_title)
-- 	if new_app then
-- 		bonsaiwm.log("focused: " .. new_app)
-- 	else
-- 		bonsaiwm.log("focus cleared (no clients left)")
-- 	end
-- 	if old_app then
-- 		bonsaiwm.log("  was: " .. old_app)
-- 	end
-- end)
--
-- bonsaiwm.on("client_unmapped", function(app_id, title)
-- 	bonsaiwm.log("unmapped: " .. (app_id or "unknown") .. " - " .. (title or "unknown"))
-- end)
--
-- bonsaiwm.on("client_destroyed", function(app_id, title)
-- 	bonsaiwm.log("destroyed: " .. (app_id or "unknown") .. " - " .. (title or "unknown"))
-- end)

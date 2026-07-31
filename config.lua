bonsaiwm = {
	enablegaps = 1,
	smartgaps = 0,
	gappoh = 40,
	gappov = 40,
	gappih = 80,
	gappiv = 80,
	sloppyfocus = 1,
	borderpx = 1,
	repeat_rate = 25,
	repeat_delay = 600,

	-- keyboard layout (XKB RMLVO); all fields optional, nil = xkbcommon default.
	-- applied live on Mod-Shift-R reload.
	--   rules:   usually "evdev" (default)
	--   model:   usually "pc104" (default)
	--   layout:  e.g. "us", "fr", "de" (comma-separated for multiple)
	--   variant: e.g. "dvorak", "colemak"
	--   options: e.g. "ctrl:nocaps" (CapsLock as Ctrl), "compose:menu"
	xkb_rules = {
		-- options = "ctrl:nocaps",
	},

	-- colors: "#RRGGBB" or "#RRGGBBAA"
	rootcolor = "#1a1b26ff",
	bordercolor = "#414868ff",
	focuscolor = "#7aa2f7ff",
	urgentcolor = "#f7768eff",
	fullscreen_bg = "#1a1b26ff",

	-- SceneFX visual effects: rounded corners, drop shadows, backdrop blur and
	-- per-client opacity. The table is optional and grouped into four logical
	-- sub-tables; every field is optional, so any subset is a valid override —
	-- omitted fields keep the compiled-in defaults shown below.
	--
	-- Reload (Mod-Shift-R) applies changes live: value tweaks (colors, radii,
	-- blur sigmas, opacity levels, blur params) and structural toggles
	-- (shadow/corner_radius/blur on/off) are re-applied to already-mapped
	-- windows.
	-- Boolean flags also accept 1/0 to match the integer-flag convention used
	-- elsewhere in this config.
	scenefx = {
		-- per-client opacity: dim unfocused windows
		opacity = {
			enabled = true, -- master switch
			active = 0.9, -- opacity of the focused client
			inactive = 0.7, -- opacity of unfocused clients
		},

		-- drop shadows behind clients
		shadow = {
			enabled = true, -- master switch
			only_floating = false, -- only shadow floating clients
			color = "#00000066", -- shadow color (unfocused clients)
			color_focus = "#00000099", -- shadow color (focused client)
			blur_sigma = 20, -- shadow blur radius (unfocused)
			blur_sigma_focus = 40, -- shadow blur radius (focused)
			-- app-ids that never get a shadow (substring match against app_id/
			-- class), e.g. { "firefox", "discord" }. Empty = shadow everything.
			ignore_list = {},
		},

		-- rounded corners
		corner_radius = {
			radius = 5, -- corner radius in pixels (0 = square)
			only_floating = false, -- only round floating clients
			no_radius_when_single = true, -- square corners with one tiled client
		},

		-- backdrop blur behind clients
		blur = {
			enabled = true, -- master switch
			xray = false, -- let transparent fullscreen/floating show bg
			ignore_transparent = true, -- don't blur transparent regions
			radius = 5,
			num_passes = 3,
			noise = 0.02,
			brightness = 0.9,
			contrast = 0.9,
			saturation = 1.1,
		},
	},

	-- window rules: id and title are substring matches (nil = match any).
	--   id:         app_id (Wayland) or class (X11)
	--   title:      window title
	--   tags:       tag NUMBER 1-9 (1 = tag 1, 9 = tag 9), 0 = keep currently
	--               visible tags
	--   isfloating: 1 = floating, 0 = tiled
	--   monitor:    0-based index, -1 = current
	--
	-- rules are rebuilt from this table on every config reload (Mod-Shift-R).
	-- edit freely; the table may be empty. examples can be changed or removed.
	rules = {
		-- start on currently visible tags, floating (not tiled):
		{ id = "Gimp_EXAMPLE", title = nil, isfloating = 1, tags = 0, monitor = -1 },
		-- start on ONLY tag "9":
		{ id = "firefox_EXAMPLE", title = nil, isfloating = 0, tags = 9, monitor = -1 },
		-- user rule:
		{ id = "helium", title = nil, isfloating = 1, tags = 0, monitor = -1 },
	},
	layouts = {
		{ symbol = "Float", arrange = 0 },
		{
			symbol = "Tiling",
			arrange = 1,
		},
		{ symbol = "Monocle", arrange = 2 },
	},

	-- keymaps: lua entries take precedence over C defaults. The C defaults are
	-- just escape hatches (Mod-Shift-R reload, Ctrl-Alt-Fn VT switch) so
	-- they're always available even if this table is broken; everything else
	-- must be defined here.
	--
	-- mod:    case-insensitive modifiers joined with "+", e.g. "Alt",
	--         "Alt+Shift", "Ctrl+Alt". "none" = no modifier required.
	-- key:    xkb keysym name, e.g. "Return", "p", "1", "XF86MonBrightnessUp".
	--         Note: Shift changes the keysym sent ("1" -> "!"), so a binding
	--         for Alt+Shift+1 needs its own entry with key="exclam".
	-- action: bonsaiwm.action.* (see config.d.lua for the list).
	-- arg:    type depends on the action. Tag actions (view/tag/toggleview/
	--         toggletag) accept a 0-9 tag NUMBER here (0 = all tags,
	--         converted internally to the TAGMASK bitmask the C actions
	--         expect, or 1<<(n-1) for a single tag).
	--
	-- Setting action = bonsaiwm.action.none is a noop (useful as a placeholder).
	keymaps = {
		-- terminal
		{ mod = "Alt+Shift", key = "Return", action = bonsaiwm.action.spawn, arg = "foot" },
		{ mod = "Alt", key = "w", action = bonsaiwm.action.killclient },
		{
			mod = "Alt",
			key = "d",
			action = bonsaiwm.action.spawn,
			arg = 'footclient -o font="monospace:size=10" -o cursor.unfocused-style=none -o pad=8x8 -a launcher-fzf ~/.local/bin/fzf-apps',
		},

		-- layout switching (matches the layouts table above: 0=Float, 1=Tiling, 2=Monocle)
		{ mod = "Alt", key = "f", action = bonsaiwm.action.setlayout, arg = 0 },
		{ mod = "Alt", key = "t", action = bonsaiwm.action.setlayout, arg = 1 },
		{ mod = "Alt", key = "m", action = bonsaiwm.action.setlayout, arg = 2 },
		-- toggle between the two layout slots (lt[0] and lt[1])
		{ mod = "Alt", key = "space", action = bonsaiwm.action.setlayout, arg = -1 },

		-- workspace (tag) switching: Alt+N views tag N, Alt+Shift+N moves the
		-- focused client to tag N. Shifted keysyms (exclam, at, ...) are
		-- needed because Shift changes the keysym sent by the keyboard.
		{ mod = "Alt", key = "1", action = bonsaiwm.action.view, arg = 1 },
		{ mod = "Alt", key = "2", action = bonsaiwm.action.view, arg = 2 },
		{ mod = "Alt", key = "3", action = bonsaiwm.action.view, arg = 3 },
		{ mod = "Alt", key = "4", action = bonsaiwm.action.view, arg = 4 },
		{ mod = "Alt", key = "5", action = bonsaiwm.action.view, arg = 5 },
		{ mod = "Alt", key = "6", action = bonsaiwm.action.view, arg = 6 },
		{ mod = "Alt", key = "7", action = bonsaiwm.action.view, arg = 7 },
		{ mod = "Alt", key = "8", action = bonsaiwm.action.view, arg = 8 },
		{ mod = "Alt", key = "9", action = bonsaiwm.action.view, arg = 9 },
		{ mod = "Alt", key = "0", action = bonsaiwm.action.view, arg = 0 },
		{ mod = "Alt+Shift", key = "exclam", action = bonsaiwm.action.tag, arg = 1 },
		{ mod = "Alt+Shift", key = "at", action = bonsaiwm.action.tag, arg = 2 },
		{ mod = "Alt+Shift", key = "numbersign", action = bonsaiwm.action.tag, arg = 3 },
		{ mod = "Alt+Shift", key = "dollar", action = bonsaiwm.action.tag, arg = 4 },
		{ mod = "Alt+Shift", key = "percent", action = bonsaiwm.action.tag, arg = 5 },
		{ mod = "Alt+Shift", key = "asciicircum", action = bonsaiwm.action.tag, arg = 6 },
		{ mod = "Alt+Shift", key = "ampersand", action = bonsaiwm.action.tag, arg = 7 },
		{ mod = "Alt+Shift", key = "asterisk", action = bonsaiwm.action.tag, arg = 8 },
		{ mod = "Alt+Shift", key = "parenleft", action = bonsaiwm.action.tag, arg = 9 },
		{ mod = "Alt+Shift", key = "parenright", action = bonsaiwm.action.tag, arg = 0 },

		-- lua-defined callback: `action` can also be a plain function.
		-- runs in the compositor's lua VM with stdlib access, no compositor api.
		{
			mod = "Alt+Shift",
			key = "p",
			action = function()
				os.execute("playerctl play-pause")
			end,
		},
	},
}

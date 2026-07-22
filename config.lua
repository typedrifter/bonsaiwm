bonsaiwm = {
	enablegaps = 1,
	smartgaps = 0,
	gappoh = 40,
	gappov = 40,
	gappih = 40,
	gappiv = 40,
	sloppyfocus = 1,
	borderpx = 1,
	repeat_rate = 25,
	repeat_delay = 600,

	-- colors: "#RRGGBB" or "#RRGGBBAA"
	rootcolor = "#1a1b26ff",
	bordercolor = "#414868ff",
	focuscolor = "#7aa2f7ff",
	urgentcolor = "#f7768eff",
	fullscreen_bg = "#1a1b26ff",

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
	--         toggletag) accept a 1-9 tag NUMBER here (converted internally
	--         to the 1<<(n-1) bitmask the C actions expect).
	--
	-- Setting action = bonsaiwm.action.none is a noop (useful as a placeholder).
	keymaps = {
		-- terminal
		{ mod = "Alt+Shift", key = "Return", action = bonsaiwm.action.spawn, arg = "foot" },
		{ mod = "Alt+Shift", key = "w", action = bonsaiwm.action.spawn, arg = "helium-browser" },

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
		{ mod = "Alt+Shift", key = "exclam", action = bonsaiwm.action.tag, arg = 1 },
		{ mod = "Alt+Shift", key = "at", action = bonsaiwm.action.tag, arg = 2 },
		{ mod = "Alt+Shift", key = "numbersign", action = bonsaiwm.action.tag, arg = 3 },
		{ mod = "Alt+Shift", key = "dollar", action = bonsaiwm.action.tag, arg = 4 },
		{ mod = "Alt+Shift", key = "percent", action = bonsaiwm.action.tag, arg = 5 },
		{ mod = "Alt+Shift", key = "asciicircum", action = bonsaiwm.action.tag, arg = 6 },
		{ mod = "Alt+Shift", key = "ampersand", action = bonsaiwm.action.tag, arg = 7 },
		{ mod = "Alt+Shift", key = "asterisk", action = bonsaiwm.action.tag, arg = 8 },
		{ mod = "Alt+Shift", key = "parenleft", action = bonsaiwm.action.tag, arg = 9 },
	},
}

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
	--   tags:       bitmask, 0 = keep currently visible tags
	--   isfloating: 1 = floating, 0 = tiled
	--   monitor:    0-based index, -1 = current
	--
	-- rules are rebuilt from this table on every config reload (Mod-Shift-R).
	-- edit freely; the table may be empty. examples can be changed or removed.
	rules = {
		-- start on currently visible tags, floating (not tiled):
		{ id = "Gimp_EXAMPLE", title = nil, isfloating = 1, tags = 0, monitor = -1 },
		-- start on ONLY tag "9":
		{ id = "firefox_EXAMPLE", title = nil, isfloating = 0, tags = 1 << 8, monitor = -1 },
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
}

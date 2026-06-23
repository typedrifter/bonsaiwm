---
title: lua integration roadmap
---

# Lua API Roadmap

Move appearance, behavior, rules, monitor config, and keybinds to `config.lua`, toward zero `config.h` edits.

| Status | Step | Description | Commit |
|--------|------|-------------|--------|
|        | Window lifecycle & focus hooks | `client_mapped`, `focus_change`, `client_unmapped`, `client_destroyed` hooks; replaces `rules[]` |  |
|        | Monitor hooks | `monitor_created`, `monitor_destroyed` hooks; replaces `monrules[]` |  |
|        | Configuration functions | Appearance setters (`set_border_width`, `set_border_color`, `set_focus_color`, `set_urgent_color`, `set_root_color`) and layout/behavior setters (`set_mfact`, `set_nmaster`, `set_layout`, `toggle_gaps`, `set_smart_gaps`, `set_sloppy_focus`) |  |
|        | Additional hooks | Window state (`client_floating`, `client_fullscreen`, `client_urgent`, `client_title`, `client_monitor_changed`), tags (`tag_toggle`, `client_tagged`, `client_tag_toggled`, enhanced `tag_switch`), layout (`mfact_changed`, `nmaster_changed`), session (`session_locked`, `session_unlocked`) |  |
|        | config.h extermination | Move remaining `config.h` values to Lua (`borderpx`, colors, `sloppyfocus`, gaps, `rules[]`, `monrules[]`, `log_level`); keep only `layouts[]`, `TAGCOUNT`, `xkb_rules`, input device settings in C |  |
|        | Keybindings in Lua | `bonsaiwm.bind_key()` + action functions + `bonsaiwm.mod` table; removes `keys[]`, `MODKEY`, `TAGKEYS`, `SHCMD`, `CHVT` from config.h |  |

# Reload semantics

`bonsaiwm.reload()` (and the C `MODKEY+Shift+r` fallback binding) re-runs `config.lua`.

- **Refreshes immediately:** keybindings (`bind_key`), hooks (`on`), `exec`/`exec_once`
  behavior.
- **Re-sinks to globals:** declarative `bonsaiwm.config.*` fields re-write the C global
  values, so the *next* monitor or input device to be created picks up the new values.
- **Does not affect existing state:** existing monitors keep their current gap, mfact and
  nmaster values; existing clients keep their border colors. This matches sway's posture:
  "Config reload won't affect existing windows, only newly created ones after the reload."
- **To update existing state:** restart BonsaiWM. The `arrange` hook does re-fire on reload
  (once per monitor), so hook-driven per-layout mutations
  (e.g. `set_gaps(60, 60, 40, 40)` when on layout "Z") self-restore that way across
  reloads.

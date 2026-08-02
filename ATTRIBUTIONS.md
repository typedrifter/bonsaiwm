# Attributions

BonsaiWM is built on the shoulders of several upstream projects. This file
credits their authors and reproduces licensing information.

## Patches

### pertag

Per-tag layout settings (layout, mfact, nmaster remembered for each tag
individually), ported from the dwl pertag patch.

- Original author: wochap `<gean.marroquin@gmail.com>`
- Repository: <https://codeberg.org/dwl/dwl-patches/src/branch/main/patches/pertag>
- AI assistance was used to port the patch to BonsaiWM's codebase.

### scenefx

Rounded corners, drop shadows, blur and per-client opacity, ported from the
dwl scenefx patch. The patch was adapted to scenefx 0.4.1 (wlroots-0.19)
rather than scenefx-0.2 (wlroots-0.18), and config knobs were split into
BonsaiWM's `config.h`/`config.c` extern/const layout instead of dwl's
`config.def.h` `static const` style.

- Original author: wochap `<gean.marroquin@gmail.com>`
- Repository: <https://codeberg.org/dwl/dwl-patches/src/branch/main/stale-patches/scenefx>
- AI assistance was used to port the patch to BonsaiWM's codebase.

## Upstream projects

### dwl — dwm for Wayland

dwl is the direct parent of BonsaiWM. The project began by extending dwl, and
substantial portions of the codebase originate there.

- Repository: <https://codeberg.org/dwl/dwl>
- License: MIT/X Consortium — see [`LICENSE.dwl`](./LICENSE.dwl)

### dwm — dynamic window manager

dwl and therefore BonsaiWM use code from dwm (the classic X11 window manager).

- Repository: <https://dwm.suckless.org/>
- License: MIT/X Consortium — see [`LICENSE.dwm`](./LICENSE.dwm)
- © 2006–2019 Anselm R Garbe

### TinyWL

dwl started as an extension of TinyWL, the example Wayland compositor from the
wlroots project.

- Repository: <https://gitlab.freedesktop.org/wlroots/wlroots/-/tree/master/tinywl>
- License: CC0 (public domain) — see [`LICENSE.tinywl`](./LICENSE.tinywl)

### Sway / wlroots

Protocol implementations and architectural patterns from Sway (the i3-compatible
Wayland compositor) and the wlroots library are used throughout.

- Repository: <https://github.com/swaywm/sway>
- License: MIT — see [`LICENSE.sway`](./LICENSE.sway)
- © 2016–2017 Drew DeVault

### wlroots — ext-workspace-v1

The ext-workspace-v1 protocol implementation is vendored from wlroots because
BonsaiWM targets wlroots 0.19, which does not yet ship it. The vendored files
(`ext-protocol/wlr_ext_workspace_v1.c` and `.h`) are taken verbatim from the
wlroots 0.20.2 tree; the `.c` only includes the vendored header by relative
path, and `meson.build` adds `-DWLR_PRIVATE=` to the compiler flags exactly as
wlroots' own build does. These files are a temporary measure: they can be
deleted on the wlroots 0.20 migration (once scenefx 0.5 is stable), keeping
only the protocol XML generation in `protocols/meson.build`. The integration
glue in `ext-protocol/ext-workspace.h` is BonsaiWM-specific code that drives
the workspace handles from BonsaiWM's tag state.

- Repository: <https://gitlab.freedesktop.org/wlroots/wlroots>
- Protocol: <https://wayland.app/protocols/ext-workspace-v1>
- License: MIT — see [`LICENSE.wlroots`](./LICENSE.wlroots)

### SceneFX

SceneFX is a wlroots fork that extends the scene-graph API with rounded
corners, shadows, blur and opacity. BonsaiWM links against it as a drop-in
replacement for wlroots' renderer and `wlr_scene` headers.

- Repository: <https://github.com/wlrfx/scenefx>
- License: MIT — see [`LICENSE.scenefx`](./LICENSE.scenefx)
- © 2017–2018 Drew DeVault, © 2014 Jari Vetoniemi

## Inspiration

- [MangoWM] — a compact, Lua-driven Wayland compositor that inspired the
  design of BonsaiWM's configuration system and its use of wlroots'
  ext-workspace-v1 API to drive a tag-based status bar.
- [vanitygaps] — the dwm patch that inspired BonsaiWM's gaps implementation.

# Attributions

BonsaiWM is built on the shoulders of several upstream projects. This file
credits their authors and reproduces licensing information.

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

## Inspiration

- [MangoWM] — a compact, Lua-driven Wayland compositor that inspired the
  design of BonsaiWM's configuration system.
- [vanitygaps] — the dwm patch that inspired BonsaiWM's gaps implementation.

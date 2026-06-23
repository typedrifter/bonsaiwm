# How to build

BonsaiWM is built with Meson.

## Dependencies

- `wayland-server` ≥ 1.25.0
- `wayland-protocols`
- `wlroots-0.19` ≥ 0.19.0
- `xkbcommon`
- `libinput` ≥ 1.27.1
- `lua5.4`
- `wayland-scanner`

Optional, for XWayland support:

- `xcb`
- `xcb-icccm`

## Build

```sh
git clone https://github.com/typedrifter/bonsaiwm.git
cd bonsaiwm
meson setup build
ninja -C build
sudo ninja -C build install
```

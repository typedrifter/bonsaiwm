# BonsaiWM

BonsaiWM is a compact, opinionated [Wayland] compositor based on [dwl].

- Ready to use with sane, opinionated defaults
- Minimal external dependencies
- Extensible via Lua

## Configuration

BonsaiWM loads `config.lua` from the first existing location:

1. `$XDG_CONFIG_HOME/bonsaiwm/config.lua`
2. `$HOME/.config/bonsaiwm/config.lua`

If neither file is found, builtin defaults are used. The config is re-read on every reload (`Mod-Shift-R`), so environment variable changes are honored.

See [`config.d.lua`](./config.d.lua) for the full `bonsaiwm` global type definitions, and [`config.lua`](./config.lua) for a commented example.

## Roadmap

See [the roadmap](./ROADMAP.md) for what's planned.

## Development

This project is mainly *tradcoded*. I use AI assistance for exploring ideas, prototyping, learning, code reviews and reorganizing the codebase.

## Acknowledgements

BonsaiWM began by extending dwl, which itself started from the TinyWL example provided (CC0) by the sway/wlroots developers. [MangoWM] was a major inspiration for this project.

Huge thanks to the dwl developers and community for their work, which made BonsaiWM possible.

[dwl]: https://codeberg.org/dwl/dwl
[MangoWM]: https://github.com/mangowm/mango
[Wayland]: https://wayland.freedesktop.org/

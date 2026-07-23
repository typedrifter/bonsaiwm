# Changelog

All notable changes to this project will be documented in this file.

The format is based on [Keep a Changelog](https://keepachangelog.com/en/2.0.0/),
and this project adheres to [Semantic Versioning](https://semver.org/spec/v2.0.0.html).

## [Unreleased]

### Added

- Added Meson build system as an alternative to the Makefile-based build.
- Added configurable window gaps between tiled clients.
- Added Astro-based documentation site under `site/`.
- Added XDG Base Directory lookup for `config.lua`: searches `$XDG_CONFIG_HOME/bonsaiwm/` then `$HOME/.config/bonsaiwm/`, falling back to builtin defaults if neither is found.
- Added Lua-driven configuration via `config.lua` and a new `lua.c` module: runtime-reloadable keymaps (including Lua functions as actions), window rules, layouts, colors, and `xkb_rules` with live keymap reload.

### Changed

- **Rebranded project from dwl to BonsaiWM.** All source files, binaries, man page, and desktop entry have been renamed accordingly.
- Reformatted entire C codebase using `clang-format` for consistent style.
- Updated `README.md` to reflect the new project identity and goals.
- Tidied and consolidated license files.

### Removed

- Removed legacy upstream changelog entries (starting fresh for BonsaiWM).
- Removed `config.ref.h` example file (superseded by inline documentation and `config.h`).

### Notes

- On the `feat/lua` branch, AI-assisted tooling was used for repetitive refactors, bug hunting, and prototyping. All changes were reviewed and accepted by the maintainer.

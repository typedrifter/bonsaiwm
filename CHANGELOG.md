# Changelog

All notable changes to this project will be documented in this file.

The format is based on [Keep a Changelog](https://keepachangelog.com/en/2.0.0/),
and this project adheres to [Semantic Versioning](https://semver.org/spec/v2.0.0.html).

## [Unreleased]

### Added

- Added Meson build system as an alternative to the Makefile-based build.
- Added configurable window gaps between tiled clients.
- Added Astro-based documentation site under `site/`.
- Added Lua keybinding support: `bonsaiwm.bind_key()`, `bonsaiwm.spawn()`, and `bonsaiwm.mod` constants.
- Added `monitor_created` and `monitor_destroyed` Lua hooks.
- Added color/border configuration functions: `set_border_width`, `set_border_color`, `set_focus_color`, `set_urgent_color`, `set_root_color`.

### Changed

- **Rebranded project from dwl to BonsaiWM.** All source files, binaries, man page, and desktop entry have been renamed accordingly.
- Reformatted entire C codebase using `clang-format` for consistent style.
- Updated `README.md` to reflect the new project identity and goals.
- Tidied and consolidated license files.

### Removed

- Removed legacy upstream changelog entries (starting fresh for BonsaiWM).
- Removed `config.ref.h` example file (superseded by inline documentation and `config.h`).

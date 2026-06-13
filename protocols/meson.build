# Protocol generation using wayland-scanner.
# wlroots requires us to generate C headers from protocol XML files.
# generator() creates reusable scanner wrappers — call .process() on any XML.

wayland_scanner = find_program('wayland-scanner')
wayland_protos_dep = dependency('wayland-protocols')
wl_protocol_dir = wayland_protos_dep.get_variable(pkgconfig: 'pkgdatadir')

# generator for enum-only headers (used by cursor-shape, pointer-constraints)
wayland_scanner_enum = generator(
  wayland_scanner,
  output: '@BASENAME@-protocol.h',
  arguments: ['enum-header', '@INPUT@', '@OUTPUT@'],
)

# generator for full server-side headers (used by xdg-shell, layer-shell, etc.)
wayland_scanner_server = generator(
  wayland_scanner,
  output: '@BASENAME@-protocol.h',
  arguments: ['server-header', '@INPUT@', '@OUTPUT@'],
)

wayland_xmls = {
  # System protocols — installed by wayland-protocols package
  'cursor-shape-v1': wl_protocol_dir / 'staging/cursor-shape/cursor-shape-v1.xml',
  'pointer-constraints-unstable-v1': wl_protocol_dir / 'unstable/pointer-constraints/pointer-constraints-unstable-v1.xml',
  'xdg-shell': wl_protocol_dir / 'stable/xdg-shell/xdg-shell.xml',
  # Local protocols — shipped with bonsaiwm
  'wlr-layer-shell-unstable-v1': 'wlr-layer-shell-unstable-v1.xml',
  'wlr-output-power-management-unstable-v1': 'wlr-output-power-management-unstable-v1.xml',
}

wayland_sources = []

# enum-header modes
wayland_sources += wayland_scanner_enum.process(
  wayland_xmls['cursor-shape-v1'],
  wayland_xmls['pointer-constraints-unstable-v1'],
)

# server-header modes
wayland_sources += wayland_scanner_server.process(
  wayland_xmls['xdg-shell'],
  wayland_xmls['wlr-layer-shell-unstable-v1'],
  wayland_xmls['wlr-output-power-management-unstable-v1'],
)

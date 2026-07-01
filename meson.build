project('bonsaiwm', 'c',
  version: '0.8-dev',
  meson_version: '>=1.1',
  default_options: [
    'c_std=c11',
    'warning_level=2',
  ]
)

# Pull in protocol generation (defines wayland_sources)
subdir('protocols')

# ── Dependencies ──────────────────────────────────────────────────────────────
# Core Wayland and input libraries required by bonsaiwm.
wayland_server = dependency('wayland-server', version: '>=1.25.0')
xkbcommon = dependency('xkbcommon')
libinput = dependency('libinput', version: '>=1.27.1')
wlroots = dependency('wlroots-0.19', version: '>=0.19.0')
lua = dependency('lua5.4', required: true)

# libm is needed for math functions used in cursor/monitor calculations.
cc = meson.get_compiler('c')
libm = cc.find_library('m')

# XWayland support is optional. When enabled (or auto-detected), we need the
# xcb and xcb-icccm libraries for X11 window management.
xcb = dependency('xcb', required: get_option('xwayland'))
xcb_icccm = dependency('xcb-icccm', required: get_option('xwayland'))

# ── Git version embedding ────────────────────────────────────────────────────
# Embed the short commit hash in the binary so users can report which version
# they're running. Falls back to project version if not in a git repo.
git = find_program('git', required: false)
if git.found()
  git_status = run_command(git, 'rev-parse', '--is-inside-work-tree', check: false)
  if git_status.returncode() == 0 and git_status.stdout().strip() == 'true'
    commit_hash = run_command(git, 'rev-parse', '--short', 'HEAD', check: false).stdout().strip()
  else
    commit_hash = meson.project_version()
  endif
else
  commit_hash = meson.project_version()
endif

# ── Compiler flags ───────────────────────────────────────────────────────────
c_args = [
  '-DWLR_USE_UNSTABLE',
  '-D_POSIX_C_SOURCE=200809L',
  '-DVERSION="@0@(@1@)"'.format(meson.project_version(), commit_hash),
  '-Wno-unused-parameter',
]

# XWayland: the source uses #ifdef XWAYLAND guards for X11 client handling.
if xcb.found() and xcb_icccm.found()
  c_args += ['-DXWAYLAND']
endif

# ── Linker flags ──────────────────────────────────────────────────────────────
link_args = []

# ── AddressSanitizer (optional) ───────────────────────────────────────────────
# Useful for catching memory errors during development. Enable with:
#   meson setup -Dasan=true build
if get_option('asan')
  c_args += ['-fsanitize=address', '-fno-omit-frame-pointer', '-fno-optimize-sibling-calls']
  link_args += ['-fsanitize=address']
endif

# ── Source files ──────────────────────────────────────────────────────────────
sources = files(
  'bonsaiwm.c',
  'config.c',
  'util.c',
  'lua.c',
)

# ── Executable ────────────────────────────────────────────────────────────────
bonsaiwm = executable('bonsaiwm',
  sources,
  wayland_sources,
  dependencies: [wayland_server, xkbcommon, libinput, wlroots, libm, xcb, xcb_icccm, lua],
  c_args: c_args,
  link_args: link_args,
  install: true,
)

# ── Install ───────────────────────────────────────────────────────────────────
install_man('bonsaiwm.1')
install_data('bonsaiwm.desktop', install_dir: get_option('datadir') / 'wayland-sessions')

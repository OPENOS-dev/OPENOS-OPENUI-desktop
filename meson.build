project('openos-de', 'c', 'cpp',
  version: 'DEV2026.1',
  default_options: ['c_std=c11', 'cpp_std=c++17', 'warning_level=2'])

cc = meson.get_compiler('c')

wlroots       = dependency('wlroots')
wayland_client = dependency('wayland-client')
wayland_server = dependency('wayland-server')
wayland_protos = dependency('wayland-protocols')
xkb           = dependency('xkbcommon')
pixman        = dependency('pixman-1')
libinput      = dependency('libinput')
math          = cc.find_library('m')

# Qt 6 (外壳): Core/Gui/Quick/Qml + WaylandClient(协议绑定)
qt6      = import('qt6')
qt6_dep  = dependency('qt6', modules: ['Core', 'Gui', 'Quick', 'Qml', 'WaylandClient'])

inc = include_directories('src')

wl_proto_dir = wayland_protos.get_variable(pkgconfig: 'pkgdatadir')
wayland_scanner = find_program('wayland-scanner')

gen_client = generator(wayland_scanner,
  output: ['@BASENAME@-client-protocol.h'],
  arguments: ['client-header', '@INPUT@', '@OUTPUT@'])
gen_server = generator(wayland_scanner,
  output: ['@BASENAME@-server-protocol.h'],
  arguments: ['server-header', '@INPUT@', '@OUTPUT@'])
gen_code = generator(wayland_scanner,
  output: ['@BASENAME@-protocol.c'],
  arguments: ['private-code', '@INPUT@', '@OUTPUT@'])

xdg_xml    = wl_proto_dir / 'stable/xdg-shell/xdg-shell.xml'
layer_xml  = files('protocols/wlr-layer-shell-unstable-v1.xml')
ft_xml     = files('protocols/wlr-foreign-toplevel-management-unstable-v1.xml')
ws_xml     = files('protocols/openos-workspace-unstable-v1.xml')

xdg_code   = gen_code.process(xdg_xml)
layer_cli  = gen_client.process(layer_xml)
layer_srv  = gen_server.process(layer_xml)
layer_code = gen_code.process(layer_xml)
ft_cli     = gen_client.process(ft_xml)
ft_code    = gen_code.process(ft_xml)
ws_cli     = gen_client.process(ws_xml)
ws_srv     = gen_server.process(ws_xml)
ws_code    = gen_code.process(ws_xml)

# 合成器 (窗口管理 + 多工作区 + 动态模糊 + 任务栏/工作区协议 + NUI2 主题)
executable('openos-compositor',
  sources: ['src/compositor.c', 'src/blur.c', 'src/workspace.c',
            xdg_code, layer_code, layer_srv, ws_code, ws_srv],
  dependencies: [wlroots, wayland_server, xkb, pixman, libinput, math],
  include_directories: inc,
  install: true)

# 面板 layer 层 (LayerShellQt, KDE) — 可选; 缺失时面板保持普通置顶窗口
layer_shell_qt = dependency('LayerShellQt', required: false)

# 外壳 (Qt Quick: 任务栏/工作区/菜单/通知)
# 数据协议 (foreign-toplevel / 自研 workspace) 用原生 wayland-client 绑定。
# 注: 旧 cairo 客户端 panel.c / notifyd.c 已由 Qt 外壳取代, 保留源码但不编译。
qt6_shell = qt6.preprocess(
  moc_headers: ['src/shell/shellbackend.h', 'src/shell/waylandbridge.h'],
  qresources:  ['src/shell/qml/qml.qrc'],
)
executable('openos-shell',
  sources: ['src/shell/main.cpp', 'src/shell/shellbackend.cpp',
            'src/shell/waylandbridge.cpp', qt6_shell,
            ft_cli, ft_code, ws_cli, ws_code],
  dependencies: [qt6_dep, wayland_client, math, layer_shell_qt],
  include_directories: inc,
  cpp_args: layer_shell_qt.found() ? ['-DOPENOS_USE_LAYERSHELLQT'] : [],
  install: true)

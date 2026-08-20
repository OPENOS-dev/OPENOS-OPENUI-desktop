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

# Qt 6 (外壳): Core/Gui/Quick/Qml + WaylandClient(协议绑定) + Svg(图标着色)
qt6      = import('qt6')
qt6_dep  = dependency('qt6', modules: ['Core', 'Gui', 'Quick', 'Qml', 'WaylandClient', 'Svg'])

inc = include_directories('src')

# wayland-scanner 生成的协议头 (如 xdg-shell-protocol.h) 位于 build 目录,
# compositor 用尖括号 include, 需显式加入 include 路径
protocol_inc = include_directories(meson.current_build_dir())

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

# 合成器 (窗口管理 + 多工作区 + 动态模糊 + 任务栏/工作区协议 + NUI2 主题
#        + 窗口装饰(SSD) + 窗口动画 + 触摸板手势 + 分屏平铺)
executable('openos-compositor',
  sources: ['src/compositor.c', 'src/blur.c', 'src/workspace.c',
            'src/decorations.c', 'src/tiling.c', 'src/gestures.c',
            xdg_code, layer_code, layer_srv, ws_code, ws_srv],
  dependencies: [wlroots, wayland_server, xkb, pixman, libinput, math],
  include_directories: [inc, protocol_inc],
  c_args: ['-DWLR_USE_UNSTABLE'],
  install: true)

# 面板 layer 层 (LayerShellQt, KDE) — 可选; 缺失时面板保持普通置顶窗口
layer_shell_qt = dependency('LayerShellQt', required: false)

# 外壳 (Qt Quick: 任务栏/工作区/菜单/通知)
# 数据协议 (foreign-toplevel / 自研 workspace) 用原生 wayland-client 绑定。
# 注: 旧 cairo 客户端 panel.c / notifyd.c 已由 Qt 外壳取代, 保留源码但不编译。
qt6_shell = qt6.preprocess(
  moc_headers: ['src/shell/shellbackend.h', 'src/shell/waylandbridge.h',
                'src/shell/iconloader.h', 'src/shell/iconprovider.h'],
  qresources:  ['src/shell/qml/qml.qrc'],
)
executable('openos-shell',
  sources: ['src/shell/main.cpp', 'src/shell/shellbackend.cpp',
            'src/shell/waylandbridge.cpp',
            'src/shell/iconloader.cpp', 'src/shell/iconprovider.cpp',
            qt6_shell,
            ft_cli, ft_code, ws_cli, ws_code],
  dependencies: [qt6_dep, wayland_client, math, layer_shell_qt],
  include_directories: inc,
  cpp_args: layer_shell_qt.found() ? ['-DOPENOS_USE_LAYERSHELLQT'] : [],
  install: true)

# 内置 freedesktop 图标主题 (NUI 线条风, 102 个 SVG, 8 个 Context)
# 安装到 /usr/share/icons/OpenOS/, 由 IconLoader 优先查找, hicolor 兜底
install_subdir('icons/OpenOS',
  install_dir: get_option('datadir') / 'icons',
  strip_directory: true)

# 把图标主题注册为系统默认 (通过 XDG_DATA_DIRS 生效)
meson.add_install_script('meson/install-icon-theme.sh')

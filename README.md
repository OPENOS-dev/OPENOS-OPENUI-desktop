# OPENOS 桌面环境 (openos-de) — Wayland(wlroots) 合成器 + Qt Quick 外壳

桌面环境采用 **KDE Plasma 式分层架构**（站在巨人肩膀上）：
- **合成器**：C + wlroots（自研，含动态模糊/多工作区/自研协议）
- **外壳**：**Qt 6 / Qt Quick (QML)**（任务栏/工作区切换器/启动器/通知）
- 视觉遵循 **NUI2 + OPENUI** 设计体系（见 `OPENUI.md`、`src/openui.h`、QML 令牌）

> **架构原则（2026-08-16 用户确立）**：
> - **OPENUI 只是 GUI 框架**，不内置具体 app 功能。
> - 需要交互功能的是**独立 app**（`src/openos-*`），各自可单独 `meson setup` 构建：
>   `openos-settings` / `openos-oak` / `openos-opt` /
  `openos-calendar` / `openos-run` / `openos-welcome`。
> - 每个独立 app 自包含 `main.cpp + QML + qml.qrc + meson.build + OpenUI.qml`（令牌副本）。

目标架构：arm32 / arm64 / x86 / x86-64（构建在 OPENOS / Linux 上，macOS 无法编译运行）。

## 架构
- `openos-compositor` — wlroots 合成器 = 窗口管理器核心（保留，Qt 化范围外）
  - `xdg-shell`：普通应用窗口（居中、拖拽移动、Esc 关闭、最大化/全屏）
  - `wlr-layer-shell-v1`：桌面层（面板、启动器、通知）
  - **多工作区**：4 个工作区（`src/compositor.c`），窗口按打开时所在工作区挂载；
    快捷键 `Ctrl+1..4` 直达、`Ctrl+Tab` 循环切换
  - **动态模糊**（NUI2 核心）：`src/blur.c` 两阶段渲染管线，面板/菜单/通知挂
    `blur_tree` 其下方内容毛玻璃化；失败自动回退普通渲染
  - **任务栏数据源**：`wlr-foreign-toplevel-management` 协议广播窗口列表，
    处理激活/关闭/最大化请求
  - **工作区协议**：自研 `protocols/openos-workspace-unstable-v1.xml`
    （`src/workspace.c`），广播工作区状态并接收切换请求
- `openos-shell` — **Qt Quick 外壳**（`src/shell/`）
  - `main.cpp`：入口，注入 `shell`(ShellBackend) / `wayland`(WaylandBridge) /
    `OpenUI`(设计令牌) 到 QML 上下文
  - `ShellBackend`：数据模型（任务栏窗口 / 工作区 / 通知）+ 应用启动
  - `WaylandBridge`：layer-shell 面板 + foreign-toplevel 任务栏 + workspace
    协议桥（骨架，见下）
  - `qml/` 完整组件集：
    - 面板：`Panel.qml`（任务栏+工作区+时钟+托盘+电源+Menu）
    - 系统层：`Tray.qml`（托盘）、`QuickSettings.qml`（快捷设置）、
      `PowerMenu.qml`（电源菜单）、`LockScreen.qml`（锁屏，对接 OAK 解锁）、
      `WelcomeScreen.qml`（首次启动欢迎）
    - 效率层：`CommandPalette.qml`（Ctrl+K 命令面板）、`CalendarPopup.qml`（日历）、
      `AltTabSwitcher.qml`（窗口切换）、`RunDialog.qml`（Alt+F2 运行）
    - 管理层：`SettingsCenter.qml` + `SettingsPages.qml`（设置中心含 OAK 安全页/
      opt 包管理/隔离管理）、`OptManagerUI.qml`（opt 图形前端）、
      `VmappManager.qml`（软件隔离管理）、`ThemeCustomizer.qml`（主题自定义）
    - `Notifications.qml`（通知）、`OpenUI.qml`（设计令牌）
    - 启动器（Win10 开始菜单风格：pacman 系统应用 + vmapp 隔离应用）已迁移到独立
      应用 `openos-run`（`OPENOS-run` 仓库），由面板 Menu 按钮 `shell.launchApp("openos-run")` 调起
  - 快捷键：`Ctrl+K` 命令面板 / `Alt+F2` 运行 / `Ctrl+L` 锁屏 / `Alt+Tab` 切换 /
    `Ctrl+T` 主题 / `Ctrl+S` 设置 / `Ctrl+P` opt / `Ctrl+V` 隔离管理
  - `AppsModel`（`shellbackend.h`）：应用条目模型（name/exec/vmapp），
    `ShellBackend::refreshApps()` 扫描宿主 `/usr/share/applications` 与
    `/vmapp/<app>/usr/share/applications`；`launchInVmapp()` 在隔离环境启动
- 旧 cairo 客户端 `panel.c` / `notifyd.c` 已由 Qt 外壳取代，保留源码但**不再编译**。

## 设计令牌（双端一致）
- C 端：`src/nui2.h`（NUI2 基线）、`src/openui.h`（OPENUI 完整令牌）
- QML 端：`src/shell/qml/OpenUI.qml`（与 openui.h 一一对应）
- 规范文档：`OPENUI.md`

## 构建（在 OPENOS / Linux 上）
依赖：`wlroots`、`wayland-client/server`、`wayland-protocols`、`libxkbcommon`、
`pixman`、`libinput`、`wayland-scanner` + **Qt 6**（`qt6-base-dev`、`qt6-declarative-dev`、
`qt6-wayland-dev`）。

```
meson setup build
ninja -C build
sudo ninja -C build install
```

## 运行
```
openos-compositor &
sleep 1
openos-shell &
```

## WaylandBridge（已接入协议绑定，待 OPENOS/Linux 验证）
`src/shell/waylandbridge.{h,cpp}`：
- **数据协议用原生 libwayland-client**（`wayland-client` 依赖）：
  - `zwlr_foreign_toplevel_manager_v1` → 任务栏窗口列表（title/app_id/state/closed，
    bind 时合成器推送全部已开窗口；activate/close 请求回合成器）
  - `openos_workspace_manager_v1`（自研）→ 工作区胶囊（name/state，activate 请求）
- **面板 layer 层用 LayerShellQt（KDE）**：`setWindowLayerSurface()` 经
  `LayerShellQt::Shell` 把面板窗口注册为 layer-surface（锚定顶部全宽/右上角、
  exclusive zone）。依赖 `LayerShellQt`（可选，缺失时面板保持普通置顶窗口，
  `-DOPENOS_USE_LAYERSHELLQT` 由 meson 按依赖自动加）。
- **事件循环集成**：`QSocketNotifier` 监听 `wl_display_get_fd()`，接入 Qt 事件循环，
  无需额外线程。
- 协议事件经信号驱动 `ShellBackend` 模型（windowAdded/Updated/Removed、
  workspaceAdded/Updated/Activated 六路信号）。
- 需在 OPENOS/Linux 上编译验证（macOS 无 wayland/Qt wayland 无法编译）。

## 已知约束 / 后续路线
- [x] wlroots 合成器：窗口管理 + 多工作区 + 动态模糊 + layer-shell + 自研协议
- [x] OPENUI 设计规范（C + QML 双端令牌）
- [x] Qt Quick 外壳：面板/任务栏/工作区胶囊/启动器/通知（UI + 数据模型完整）
- [x] WaylandBridge 协议接线（foreign-toplevel / workspace 原生绑定 + LayerShellQt 面板）
- [ ] 设置守护 / 键盘优先快捷键（Ctrl+K 命令面板）
- [ ] 动效落地（Qt 动画引擎驱动 OPENUI 时长/缓动令牌）
- [ ] 四架构交叉构建编排（BUILD_SYSTEM 已就绪）

> 注：旧版 X11 与 cairo 客户端实现已废弃（源码保留），桌面 = wlroots 合成器 + Qt 外壳。

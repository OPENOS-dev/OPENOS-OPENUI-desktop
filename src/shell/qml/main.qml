import QtQuick 2.15
import QtQuick.Window 2.15

/* OPENOS 桌面外壳主窗口
 * 面板 (任务栏/托盘/时钟) + 覆盖层 (锁屏/命令面板/Alt+Tab/运行/主题)
 * + 独立窗口 (设置中心/opt 管理/隔离管理)
 */
Window {
    id: shellWindow
    width: Screen.width
    height: Screen.height
    x: 0; y: 0
    flags: Qt.FramelessWindowHint | Qt.WindowStaysOnTopHint
    color: "transparent"
    visible: true

    // ===== 全局快捷键 (键盘优先) =====
    Shortcut { sequence: "Ctrl+K"; onActivated: palette.visible = true }
    Shortcut { sequence: "Alt+F2"; onActivated: runDlg.visible = true }
    Shortcut { sequence: "Ctrl+L"; onActivated: lockScreen.visible = true }
    Shortcut { sequence: "Alt+Tab"; onActivated: switcher.visible = true }
    Shortcut { sequence: "Ctrl+T";  onActivated: themeWin.visible = true }
    Shortcut { sequence: "Ctrl+S";  onActivated: settingsWin.visible = true }
    Shortcut { sequence: "Ctrl+P";  onActivated: optWin.visible = true }
    Shortcut { sequence: "Ctrl+V";  onActivated: vmappWin.visible = true }
    Shortcut { sequence: "Ctrl+M";  onActivated: sysMonWin.visible = true }
    Shortcut { sequence: "Shift+Ctrl+S"; onActivated: screenshotWin.visible = true }
    Shortcut { sequence: "Shift+Ctrl+C"; onActivated: clipboardWin.visible = true }
    Shortcut { sequence: "Ctrl+R";  onActivated: recorderWin.visible = true }
    Shortcut { sequence: "Shift+Ctrl+F"; onActivated: fileSearch.visible = true }
    Shortcut { sequence: "Shift+Ctrl+T"; onActivated: tilingPanel.visible = !tilingPanel.visible }
    Shortcut { sequence: "Ctrl+N";  onActivated: notesWin.visible = !notesWin.visible }
    Shortcut { sequence: "Alt+C";   onActivated: calcWin.visible = !calcWin.visible }
    Shortcut { sequence: "Alt+F";   onActivated: fmWin.visible = !fmWin.visible }
    Shortcut { sequence: "Alt+T";   onActivated: termWin.visible = !termWin.visible }
    Shortcut { sequence: "Ctrl+E";  onActivated: editorWin.visible = !editorWin.visible }
    Shortcut { sequence: "Ctrl+W";  onActivated: widgetWin.visible = !widgetWin.visible }
    Shortcut { sequence: "Ctrl+B";  onActivated: appearanceWin.visible = !appearanceWin.visible }
    Shortcut { sequence: "Ctrl+Shift+K"; onActivated: kbdWin.visible = !kbdWin.visible }
    Shortcut { sequence: "Shift+Ctrl+D"; onActivated: diskWin.visible = !diskWin.visible }
    Shortcut { sequence: "Alt+P"; onActivated: colorWin.visible = !colorWin.visible }
    Shortcut { sequence: "Alt+M";   onActivated: mediaWin.visible = !mediaWin.visible }
    Shortcut { sequence: "Alt+I";   onActivated: imageWin.visible = !imageWin.visible }
    Shortcut { sequence: "Alt+Shift+C"; onActivated: clockWin.visible = !clockWin.visible }

    // ===== 面板层 (顶部) =====
    Panel {
        anchors.top: parent.top
        anchors.left: parent.left
        anchors.right: parent.right
        height: OpenUI.panelHeight
    }

    // ===== 覆盖层 (全屏交互) =====
    Item {
        id: overlayLayer
        anchors.fill: parent
        z: 100

        // 锁屏 (最顶层)
        LockScreen {
            id: lockScreen
            anchors.fill: parent
            z: 10
        }

        // 登录屏 (锁屏切换用户时显示)
        LoginScreen {
            id: loginScreen
            anchors.fill: parent
            z: 11
        }

        // 命令面板 (居中)
        CommandPalette {
            id: palette
            anchors.centerIn: parent
            z: 5
            Component.onCompleted: build()
        }

        // 运行对话框
        RunDialog {
            id: runDlg
            anchors.centerIn: parent
            z: 4
        }

        // 窗口切换器
        AltTabSwitcher {
            id: switcher
            anchors.centerIn: parent
            z: 3
        }

        // 主题自定义
        ThemeCustomizer {
            id: themeWin
            anchors.centerIn: parent
            z: 2
        }
    }

    // ===== 独立窗口 (设置/opt/隔离/系统工具) =====
    SettingsCenter  { id: settingsWin }
    OptManagerUI    { id: optWin }
    VmappManager    { id: vmappWin }
    SystemMonitor   { id: sysMonWin }        // Ctrl+M
    Screenshot      { id: screenshotWin }    // Shift+Ctrl+S
    Clipboard       { id: clipboardWin }     // Shift+Ctrl+C
    ScreenRecorder  { id: recorderWin }      // Ctrl+R

    // 便签
    Notes { id: notesWin }
    // 计算器
    Calculator { id: calcWin }
    // 文件管理器
    FileManager { id: fmWin }
    // 终端
    Terminal { id: termWin }
    // 文本编辑器
    TextEditor { id: editorWin }
    // 桌面小部件
    DesktopWidgets { id: widgetWin }
    // 外观设置
    AppearanceSettings { id: appearanceWin }
    // 屏幕键盘
    VirtualKeyboard { id: kbdWin }
    // 磁盘分析
    DiskUsage { id: diskWin }
    // 取色器
    ColorPicker { id: colorWin }
    // 媒体播放器
    MediaPlayer { id: mediaWin }
    // 时钟
    Clock { id: clockWin }
    // 图片查看器
    ImageViewer { id: imageWin }

    // 文件搜索 (覆盖层)
    FileSearch {
        id: fileSearch
        anchors.fill: parent
        z: 150
    }

    // 窗口分屏面板 (弹出层, 定位面板下方)
    TilingManager {
        id: tilingPanel
        z: 50
        x: (shellWindow.width - width) / 2
        y: OpenUI.panelHeight + 4
    }

    // 通知窗口 (右上角)
    Window {
        id: notifWin
        flags: Qt.FramelessWindowHint | Qt.WindowStaysOnTopHint | Qt.Tool
        width: 340
        height: notifLayer.height + 16
        x: shellWindow.screen ? shellWindow.screen.availableVirtualGeometry.x
                                + shellWindow.screen.availableVirtualGeometry.width
                                - width - 8 : 0
        y: shellWindow.height + 8
        color: "transparent"
        visible: shell.notifications.count > 0

        Notifications {
            id: notifLayer
            anchors.top: parent.top
            anchors.topMargin: 8
        }
    }

    // 首次启动欢迎 (骨架: 演示用, 生产由初始化流程控制)
    WelcomeScreen {
        id: welcome
        anchors.fill: parent
        z: 200
        visible: false   // 生产: 检测未初始化时显示
    }
}

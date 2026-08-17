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

    // ===== 独立窗口 (设置/opt/隔离) =====
    SettingsCenter { id: settingsWin }
    OptManagerUI   { id: optWin }
    VmappManager   { id: vmappWin }

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

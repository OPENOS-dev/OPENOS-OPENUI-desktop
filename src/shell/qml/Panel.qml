import QtQuick 2.15
import QtQuick.Layouts 1.15

/* OPENOS 顶部面板: 品牌标识 + 工作区胶囊 + 任务栏 + 时钟 + Menu (OPENUI)
 * 毛玻璃: 更高透明底 + 合成器动态模糊 (下方内容模糊可见) */
Rectangle {
    id: panel
    height: OpenUI.panelHeight
    color: Qt.rgba(OpenUI.surface.r, OpenUI.surface.g, OpenUI.surface.b,
                   OpenUI.glassPanelAlpha)

    // 底部柔和阴影条 (普通用户更习惯的层次感)
    Rectangle {
        anchors.left: parent.left; anchors.right: parent.right
        anchors.bottom: parent.bottom
        height: 1
        color: Qt.rgba(OpenUI.outlineVariant.r, OpenUI.outlineVariant.g,
                       OpenUI.outlineVariant.b, 0.5)
    }

    RowLayout {
        anchors.fill: parent
        anchors.leftMargin: OpenUI.sp2
        anchors.rightMargin: OpenUI.sp2
        spacing: OpenUI.sp3

        // 品牌标识
        Text {
            text: "OPENOS DEV2026.1"
            color: OpenUI.onSurface
            font.pixelSize: 12
            verticalAlignment: Text.AlignVCenter
        }

        // 工作区指示器 (4 个胶囊, 点击切换)
        Row {
            spacing: OpenUI.wsGap
            Layout.topMargin: (panel.height - OpenUI.wsCapsuleH) / 2
            Repeater {
                model: shell.workspaces
                Rectangle {
                    id: wsCapsule
                    width: hover.hovered ? OpenUI.wsCapsuleW + 6 : OpenUI.wsCapsuleW
                    height: OpenUI.wsCapsuleH
                    radius: OpenUI.shapeFull
                    color: {
                        if (model.active) return OpenUI.primary
                        if (hover.hovered)
                            return Qt.rgba(OpenUI.primary.r, OpenUI.primary.g,
                                          OpenUI.primary.b, 0.6)
                        return OpenUI.outlineVariant
                    }
                    Behavior on width { NumberAnimation { duration: OpenUI.dur100 } }
                    Behavior on color { ColorAnimation { duration: OpenUI.dur150 } }
                    MouseArea {
                        id: hover
                        anchors.fill: parent
                        hoverEnabled: true
                        onClicked: wayland.requestActivateWorkspace(index)
                    }
                }
            }
        }

        // 任务栏窗口列表
        Row {
            spacing: OpenUI.sp1
            clip: true
            Layout.maximumWidth: panel.width * 0.45
            Repeater {
                model: shell.windows
                Rectangle {
                    id: btn
                    width: Math.max(48, Math.min(160, implicitW))
                    height: OpenUI.taskButtonH
                    radius: OpenUI.shapeSm   /* 任务栏圆角加大 (8->12) */
                    color: {
                        if (model.active)
                            return Qt.rgba(OpenUI.primary.r, OpenUI.primary.g,
                                          OpenUI.primary.b, 0.22)
                        if (hover.hovered)
                            return Qt.rgba(OpenUI.onSurface.r, OpenUI.onSurface.g,
                                          OpenUI.onSurface.b, OpenUI.hoverAlpha)
                        return Qt.rgba(OpenUI.surfaceBright.r, OpenUI.surfaceBright.g,
                                       OpenUI.surfaceBright.b, OpenUI.glassTaskAlpha)
                    }
                    Behavior on color { ColorAnimation { duration: OpenUI.dur100 } }
                    readonly property int implicitW: Math.round(title.width) + 20
                    Text {
                        id: title
                        text: model.title || model.appId || "?"
                        elide: Text.ElideRight
                        anchors.centerIn: parent
                        anchors.leftMargin: 10
                        width: parent.width - 20
                        horizontalAlignment: Text.AlignHCenter
                        color: model.active ? OpenUI.primary : OpenUI.onSurfaceVariant
                        font.pixelSize: 12
                    }
                    MouseArea {
                        id: hover
                        anchors.fill: parent
                        hoverEnabled: true
                        acceptedButtons: Qt.LeftButton | Qt.RightButton
                        onClicked: {
                            if (mouse.button === Qt.RightButton)
                                wayland.requestCloseWindow(index)
                            else
                                wayland.requestActivateWindow(index)
                        }
                    }
                }
            }
        }

        Item { Layout.fillWidth: true }

        // 时钟 (每秒刷新, 点击弹日历)
        Rectangle {
            id: clockBtn
            width: clock.width + 16; height: panel.height - 8
            radius: OpenUI.shapeXs
            color: clockHover.hovered || calPopup.visible
                   ? Qt.rgba(OpenUI.onSurface.r, OpenUI.onSurface.g,
                             OpenUI.onSurface.b, OpenUI.hoverAlpha)
                   : "transparent"
            Text {
                id: clock
                anchors.centerIn: parent
                text: Qt.formatTime(new Date(), "hh:mm")
                color: OpenUI.onSurfaceVariant
                font.pixelSize: 12
                verticalAlignment: Text.AlignVCenter
                Timer {
                    interval: 1000; running: true; repeat: true
                    onTriggered: clock.text = Qt.formatTime(new Date(), "hh:mm")
                }
            }
            MouseArea {
                id: clockHover
                anchors.fill: parent
                hoverEnabled: true
                onClicked: calPopup.visible = !calPopup.visible
            }
        }

        // 系统托盘 (音量/WiFi/电池)
        Tray {
            Layout.alignment: Qt.AlignVCenter
            Layout.topMargin: 4
            quickSettings: quickSettingsPopup
        }

        // 电源按钮
        Rectangle {
            width: 28; height: panel.height - 8; radius: OpenUI.shapeXs
            color: powerHover.hovered || powerMenu.visible
                   ? Qt.rgba(OpenUI.error.r, OpenUI.error.g, OpenUI.error.b, 0.25)
                   : "transparent"
            Text {
                anchors.centerIn: parent
                text: "\u23FB"
                color: powerHover.hovered ? OpenUI.error : OpenUI.onSurfaceVariant
                font.pixelSize: 14
            }
            MouseArea {
                id: powerHover
                anchors.fill: parent
                hoverEnabled: true
                onClicked: powerMenu.visible = !powerMenu.visible
            }
        }

        // Menu 按钮 (文字符号, 禁 Emoji)
        Rectangle {
            id: menuBtn
            width: 84; height: panel.height - 8
            radius: OpenUI.shapeSm
            color: launcher.visible ? OpenUI.primaryContainer
                  : menuHover.hovered
                    ? Qt.rgba(OpenUI.primary.r, OpenUI.primary.g,
                              OpenUI.primary.b, 0.8)
                    : OpenUI.primary
            Behavior on color { ColorAnimation { duration: OpenUI.dur100 } }
            Text {
                anchors.centerIn: parent
                text: "\u2630 Menu"
                color: launcher.visible ? OpenUI.onPrimaryContainer
                                       : OpenUI.onPrimary
                font.pixelSize: 12; font.bold: true
            }
            MouseArea {
                id: menuHover
                anchors.fill: parent
                hoverEnabled: true
                onClicked: launcher.visible = !launcher.visible
            }
        }
    }

    Launcher { id: launcher; parent: panel }

    // 快捷设置弹出 (托盘触发, 定位面板右下方)
    QuickSettings {
        id: quickSettingsPopup
        parent: panel
        x: panel.width - width - 8
        y: panel.height + 4
    }

    // 日历弹出 (时钟触发)
    CalendarPopup {
        id: calPopup
        parent: panel
        x: panel.width - 320
        y: panel.height + 4
    }

    // 电源菜单
    PowerMenu {
        id: powerMenu
        parent: panel
        x: panel.width - width - 8
        y: panel.height + 4
    }
}

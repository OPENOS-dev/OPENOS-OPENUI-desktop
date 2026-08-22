/*
 * Copyright (C) 2026 OPENOS-dev
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the OPENOS-PROJECT-LICENSE (OPL) v1.2.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * OPL for more details.
 *
 * You should have received a copy of the OPL along with this program.
 * If not, see <https://github.com/OPENOS-dev/OPL>.
 */

import QtQuick 2.15

/* OPENOS 通知中心 (增强版)
 * 功能: 通知历史 / 按应用分组 / 勿扰模式 / 操作按钮
 * 右键/点击通知弹出操作菜单
 */
Rectangle {
    id: notifLayer
    visible: shell.notifications.count > 0 || !dndToggle.checked
    width: 340
    implicitHeight: col.height + 16

    // 勿扰模式
    property bool doNotDisturb: shell.doNotDisturb

    Column {
        id: col
        anchors.fill: parent
        anchors.margins: 8
        spacing: 6

        // 顶部栏: 标题 + 清除全部 + 勿扰开关
        Row {
            width: parent.width
            height: 28
            spacing: 6

            Text {
                text: "通知"; height: parent.height
                verticalAlignment: Text.AlignVCenter
                color: OpenUI.onSurface; font.pixelSize: 13; font.bold: true
            }

            Item { width: 1; height: 1; Layout.fillWidth: true }

            // 勿扰模式开关
            Rectangle {
                width: 70; height: 22; radius: OpenUI.shapeXs
                color: dndToggle.checked
                       ? Qt.rgba(OpenUI.primary.r, OpenUI.primary.g,
                                 OpenUI.primary.b, 0.25)
                       : Qt.rgba(OpenUI.surfaceBright.r, OpenUI.surfaceBright.g,
                                 OpenUI.surfaceBright.b, 0.4)
                Text {
                    anchors.centerIn: parent
                    text: dndToggle.checked ? "勿扰 开" : "勿扰"
                    color: dndToggle.checked ? OpenUI.primary : OpenUI.onSurfaceVariant
                    font.pixelSize: 11
                }
                MouseArea {
                    anchors.fill: parent; hoverEnabled: true
                    onClicked: {
                        dndToggle.checked = !dndToggle.checked
                        shell.doNotDisturb = dndToggle.checked
                    }
                }
            }

            // 清除全部按钮
            Rectangle {
                width: 50; height: 22; radius: OpenUI.shapeXs
                color: clearHover.hovered
                       ? Qt.rgba(OpenUI.error.r, OpenUI.error.g,
                                 OpenUI.error.b, 0.2)
                       : "transparent"
                visible: shell.notifications.count > 0
                Text {
                    anchors.centerIn: parent
                    text: "清除"; color: OpenUI.error; font.pixelSize: 11
                }
                MouseArea {
                    id: clearHover; anchors.fill: parent; hoverEnabled: true
                    onClicked: shell.clearAllNotifications()
                }
            }
        }

        // 通知列表 (按应用分组)
        Repeater {
            model: shell.notifications

            Column {
                width: parent.width
                spacing: 3

                // 分组标题 (首次出现某 appId 时显示)
                Rectangle {
                    visible: {
                        if (index === 0) return true
                        var prev = shell.notifications.data(
                            shell.notifications.index(index - 1),
                            NotificationModel.GroupRole)
                        return prev !== model.group
                    }
                    width: parent.width; height: 18; color: "transparent"
                    Text {
                        anchors.verticalCenter: parent.verticalCenter
                        text: model.appId && model.appId !== "system"
                              ? model.appId : "系统"
                        color: OpenUI.onSurfaceVariant
                        font.pixelSize: 10; font.bold: true
                    }
                }

                // 通知卡片
                Rectangle {
                    id: card
                    width: parent.width
                    height: Math.max(68, bodyText.height + 44)
                    radius: OpenUI.shapeSm
                    color: Qt.rgba(OpenUI.surface6.r, OpenUI.surface6.g,
                                   OpenUI.surface6.b, OpenUI.glassCardAlpha)
                    border.color: hover.hovered
                                  ? Qt.rgba(OpenUI.primary.r, OpenUI.primary.g,
                                            OpenUI.primary.b, 0.6)
                                  : Qt.rgba(OpenUI.outlineVariant.r,
                                            OpenUI.outlineVariant.g,
                                            OpenUI.outlineVariant.b, 1.0)
                    border.width: 1

                    scale: hover.hovered ? 1.02 : 1.0
                    Behavior on scale { NumberAnimation { duration: OpenUI.dur100 } }
                    Behavior on border.color { ColorAnimation { duration: OpenUI.dur100 } }

                    // accent 竖条
                    Rectangle {
                        id: accentBar
                        x: 4; y: 6
                        width: hover.hovered ? 5 : 3
                        height: parent.height - 12
                        radius: 2
                        color: OpenUI.primary
                        Behavior on width { NumberAnimation { duration: OpenUI.dur100 } }
                    }

                    // 标题
                    Text {
                        anchors { left: parent.left; leftMargin: 16; top: parent.top; topMargin: 8 }
                        text: model.title
                        color: OpenUI.onSurface
                        font.pixelSize: 13; font.bold: true
                        elide: Text.ElideRight; width: parent.width - 60
                    }

                    // 时间戳
                    Text {
                        anchors { right: parent.right; rightMargin: 8; top: parent.top; topMargin: 8 }
                        text: {
                            var secs = model.timestamp
                            if (!secs) return ""
                            var now = Math.floor(Date.now() / 1000)
                            var diff = now - secs
                            if (diff < 60) return "刚刚"
                            if (diff < 3600) return Math.floor(diff / 60) + "分钟前"
                            if (diff < 86400) return Math.floor(diff / 3600) + "小时前"
                            return Qt.formatDateTime(new Date(secs * 1000), "MM/dd")
                        }
                        color: OpenUI.onSurfaceVariant
                        font.pixelSize: 10
                    }

                    // 正文
                    Text {
                        id: bodyText
                        anchors { left: parent.left; leftMargin: 16; top: parent.top; topMargin: 28 }
                        text: model.body
                        color: OpenUI.onSurfaceVariant
                        font.pixelSize: 12
                        elide: Text.ElideRight; width: parent.width - 40
                        wrapMode: Text.WordWrap
                        maximumLineCount: 2
                    }

                    // 操作按钮 (如果有)
                    Row {
                        anchors { left: parent.left; leftMargin: 16; bottom: parent.bottom; bottomMargin: 4 }
                        spacing: 4
                        visible: model.actions && model.actions.length > 0

                        Repeater {
                            model: model.actions

                            Rectangle {
                                height: 22; width: actLabel.width + 16
                                radius: OpenUI.shapeXs
                                color: actHover.hovered
                                       ? Qt.rgba(OpenUI.primary.r, OpenUI.primary.g,
                                                 OpenUI.primary.b, 0.2)
                                       : Qt.rgba(OpenUI.surfaceBright.r,
                                                 OpenUI.surfaceBright.g,
                                                 OpenUI.surfaceBright.b, 0.4)
                                Text {
                                    id: actLabel
                                    anchors.centerIn: parent
                                    text: modelData
                                    color: OpenUI.primary; font.pixelSize: 11
                                }
                                MouseArea {
                                    id: actHover; anchors.fill: parent; hoverEnabled: true
                                    onClicked: {
                                        console.log("notification action:", modelData)
                                        // 生产: 通知 action 处理
                                    }
                                }
                            }
                        }
                    }

                    // 关闭按钮 (右上角 ×)
                    Rectangle {
                        anchors { right: parent.right; rightMargin: 4; top: parent.top; topMargin: 4 }
                        width: 16; height: 16; radius: 8
                        color: closeHover.hovered ? OpenUI.error : "transparent"
                        visible: hover.hovered
                        Text {
                            anchors.centerIn: parent
                            text: "\u00D7"; color: OpenUI.onSurfaceVariant
                            font.pixelSize: 12
                        }
                        MouseArea {
                            id: closeHover; anchors.fill: parent; hoverEnabled: true
                            onClicked: shell.dismissNotification(index)
                        }
                    }

                    // 点击关闭
                    MouseArea {
                        id: hover
                        anchors.fill: parent
                        hoverEnabled: true
                        onClicked: shell.dismissNotification(index)
                    }
                }
            }
        }

        // 空状态
        Rectangle {
            width: parent.width; height: 40; visible: shell.notifications.count === 0
            color: "transparent"
            Text {
                anchors.centerIn: parent
                text: "暂无通知"; color: OpenUI.onSurfaceDisabled
                font.pixelSize: 12
            }
        }
    }

    // 点击外部关闭
    MouseArea { anchors.fill: parent.parent; z: -1; onClicked: notifLayer.visible = false }
}
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
import QtQuick.Window 2.15
import QtQuick.Layouts 1.15

/* OPENOS 截图工具 (OPENUI)
 * 模式: 区域 / 窗口 / 全屏 + 倒计时延迟 + 保存/复制
 */
Window {
    id: screenshotWin
    visible: true
    width: 480
    height: 360
    flags: Qt.FramelessWindowHint
    title: "截图"
    color: OpenUI.background

    property int captureMode: 0  // 0:区域 1:窗口 2:全屏
    property int delaySeconds: 0 // 0不延迟, 3/5/10秒

    ColumnLayout {
        anchors.fill: parent
        anchors.margins: OpenUI.sp4
        spacing: OpenUI.sp3

        // 标题栏
        RowLayout {
            width: parent.width
            spacing: OpenUI.sp2

            Text {
                text: "截图"
                color: OpenUI.onSurface
                font.pixelSize: OpenUI.typeHeadlineM
                font.bold: true
                Layout.fillWidth: true
            }

            Rectangle {
                width: 32
                height: 32
                radius: OpenUI.shapeXs
                color: hover.hovered ? Qt.rgba(OpenUI.error.r, OpenUI.error.g,
                                               OpenUI.error.b, 0.3) : "transparent"
                ThemedIcon {
                    anchors.centerIn: parent
                    name: "window-close"; ctx: "Actions"; size: 20; color: OpenUI.onSurface
                }
                MouseArea {
                    id: hover
                    anchors.fill: parent
                    hoverEnabled: true
                    onClicked: screenshotWin.close()
                }
            }
        }

        // 捕获模式选择
        Column {
            spacing: OpenUI.sp2
            width: parent.width

            Text {
                text: "捕获模式"
                color: OpenUI.onSurfaceVariant
                font.pixelSize: OpenUI.typeLabelL
            }

            Row {
                spacing: OpenUI.sp2
                width: parent.width

                Repeater {
                    model: ListModel {
                        ListElement { icon: "screenshot-region"; label: "区域"; mode: 0 }
                        ListElement { icon: "screenshot-window"; label: "窗口"; mode: 1 }
                        ListElement { icon: "screenshot-fullscreen"; label: "全屏"; mode: 2 }
                    }

                    Rectangle {
                        width: (parent.width - OpenUI.sp2 * 2) / 3
                        height: 44
                        radius: OpenUI.shapeSm
                        color: captureMode === model.mode
                               ? Qt.rgba(OpenUI.primary.r, OpenUI.primary.g,
                                         OpenUI.primary.b, 0.25)
                               : Qt.rgba(OpenUI.surfaceBright.r, OpenUI.surfaceBright.g,
                                         OpenUI.surfaceBright.b, 0.4)
                        border.width: captureMode === model.mode ? 1 : 0
                        border.color: OpenUI.primary

                        Column {
                            anchors.centerIn: parent
                            spacing: OpenUI.sp1

                            ThemedIcon {
                                name: model.icon; ctx: "Actions"; size: 22
                                color: captureMode === model.mode ? OpenUI.primary : OpenUI.onSurfaceVariant
                            }
                            Text {
                                text: model.label
                                color: captureMode === model.mode ? OpenUI.primary : OpenUI.onSurface
                                font.pixelSize: OpenUI.typeLabelM
                                horizontalAlignment: Text.AlignHCenter
                            }
                        }

                        MouseArea {
                            anchors.fill: parent
                            hoverEnabled: true
                            onClicked: captureMode = model.mode
                        }
                    }
                }
            }
        }

        // 倒计时延迟选择
        Column {
            spacing: OpenUI.sp2
            width: parent.width

            Text {
                text: "延迟"
                color: OpenUI.onSurfaceVariant
                font.pixelSize: OpenUI.typeLabelL
            }

            Row {
                spacing: OpenUI.sp2

                Repeater {
                    model: [0, 3, 5, 10]

                    Rectangle {
                        width: 52
                        height: 28
                        radius: OpenUI.shapeXs
                        color: delaySeconds === modelData
                               ? Qt.rgba(OpenUI.primary.r, OpenUI.primary.g,
                                         OpenUI.primary.b, 0.25)
                               : Qt.rgba(OpenUI.surfaceBright.r, OpenUI.surfaceBright.g,
                                         OpenUI.surfaceBright.b, 0.4)
                        border.width: delaySeconds === modelData ? 1 : 0
                        border.color: OpenUI.primary

                        Text {
                            anchors.centerIn: parent
                            text: modelData === 0 ? "无" : modelData + "s"
                            color: delaySeconds === modelData ? OpenUI.primary : OpenUI.onSurfaceVariant
                            font.pixelSize: OpenUI.typeLabelM
                        }

                        MouseArea {
                            anchors.fill: parent
                            hoverEnabled: true
                            onClicked: delaySeconds = modelData
                        }
                    }
                }

                Item {
                    Layout.fillWidth: true
                }
            }
        }

        // 预览区域
        Rectangle {
            width: parent.width
            height: 140
            radius: OpenUI.shapeSm
            color: Qt.rgba(OpenUI.surfaceBright.r, OpenUI.surfaceBright.g,
                           OpenUI.surfaceBright.b, 0.4)
            Layout.fillHeight: true

            Text {
                anchors.centerIn: parent
                text: "截图预览"
                color: OpenUI.onSurfaceVariant
                font.pixelSize: OpenUI.typeBodyM
            }
        }

        // 底部操作栏
        Row {
            spacing: OpenUI.sp2
            width: parent.width

            Item {
                width: parent.width - 280 - OpenUI.sp2 * 2
                height: 1
            }

            Rectangle {
                width: 88
                height: 36
                radius: OpenUI.shapeSm
                color: Qt.rgba(OpenUI.surfaceBright.r, OpenUI.surfaceBright.g,
                               OpenUI.surfaceBright.b, 0.4)

                Text {
                    anchors.centerIn: parent
                    text: "取消"
                    color: OpenUI.onSurface
                    font.pixelSize: OpenUI.typeLabelL
                }

                MouseArea {
                    anchors.fill: parent
                    hoverEnabled: true
                    onClicked: screenshotWin.close()
                }
            }

            Rectangle {
                width: 130
                height: 36
                radius: OpenUI.shapeSm
                color: Qt.rgba(OpenUI.surfaceBright.r, OpenUI.surfaceBright.g,
                               OpenUI.surfaceBright.b, 0.4)

                Row {
                    anchors.centerIn: parent; spacing: OpenUI.sp1
                    ThemedIcon { name: "checkmark"; ctx: "Actions"; size: 14; color: OpenUI.onSurface
                                 anchors.verticalCenter: parent.verticalCenter }
                    Text {
                        text: "复制到剪贴板"
                        color: OpenUI.onSurface
                        font.pixelSize: OpenUI.typeLabelL
                        anchors.verticalCenter: parent.verticalCenter
                    }
                }

                MouseArea {
                    anchors.fill: parent
                    hoverEnabled: true
                    onClicked: console.log("screenshot: copy to clipboard, mode=", captureMode, "delay=", delaySeconds)
                }
            }

            Rectangle {
                width: 60
                height: 36
                radius: OpenUI.shapeSm
                color: Qt.rgba(OpenUI.primary.r, OpenUI.primary.g,
                               OpenUI.primary.b, 0.9)

                Row {
                    anchors.centerIn: parent; spacing: OpenUI.sp1
                    ThemedIcon { name: "document-save"; ctx: "Actions"; size: 14; color: OpenUI.onPrimary
                                 anchors.verticalCenter: parent.verticalCenter }
                    Text {
                        text: "保存"
                        color: OpenUI.onPrimary
                        font.pixelSize: OpenUI.typeLabelL
                        anchors.verticalCenter: parent.verticalCenter
                    }
                }

                MouseArea {
                    anchors.fill: parent
                    hoverEnabled: true
                    onClicked: console.log("screenshot: save to file, mode=", captureMode, "delay=", delaySeconds)
                }
            }
        }
    }

    // 快捷键
    Shortcut {
        sequence: "Escape"
        onActivated: screenshotWin.close()
    }
}

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

/* OPENOS 窗口切换器 (Alt+Tab)
 * 显示当前工作区窗口缩略 (经 shell.windows), 左右选择
 */
Rectangle {
    id: switcher
    visible: false
    width: 520
    height: 160
    radius: OpenUI.shapeLg
    color: Qt.rgba(OpenUI.surface6.r, OpenUI.surface6.g, OpenUI.surface6.b, 0.85)
    border.color: OpenUI.outlineVariant; border.width: 1

    // 居中的顶层覆盖
    anchors.centerIn: parent

    Row {
        anchors.centerIn: parent
        spacing: OpenUI.sp3
        Repeater {
            model: shell.windows
            Rectangle {
                width: 130; height: 100; radius: OpenUI.shapeSm
                color: switcher.currentIndex === index
                       ? Qt.rgba(OpenUI.primary.r, OpenUI.primary.g,
                                 OpenUI.primary.b, 0.25)
                       : Qt.rgba(OpenUI.surfaceBright.r, OpenUI.surfaceBright.g,
                                 OpenUI.surfaceBright.b, 0.4)
                border.width: switcher.currentIndex === index ? 2 : 0
                border.color: OpenUI.primary
                Text {
                    anchors.centerIn: parent
                    text: model.title || model.appId || "?"
                    color: OpenUI.onSurface
                    font.pixelSize: OpenUI.typeLabelM
                    elide: Text.ElideRight; width: parent.width - 12
                    horizontalAlignment: Text.AlignHCenter
                }
            }
        }
    }

    property int currentIndex: 0

    Keys.onLeftPressed: currentIndex = Math.max(0, currentIndex - 1)
    Keys.onRightPressed: currentIndex = Math.min(shell.windows.count - 1, currentIndex + 1)
    Keys.onReturnPressed: {
        console.log("switch to", currentIndex)
        wayland.requestActivateWindow(currentIndex)
        visible = false
    }
    Keys.onEscapePressed: visible = false
}

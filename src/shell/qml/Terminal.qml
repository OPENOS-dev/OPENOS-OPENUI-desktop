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

/* OPENOS 终端启动器 (壳内版)
 * 启动独立应用 openos-terminal
 */
Window {
    id: termWin
    visible: false
    width: 240; height: 100
    flags: Qt.FramelessWindowHint
    title: "终端"
    color: "transparent"

    Rectangle {
        anchors.fill: parent; anchors.margins: 1
        radius: OpenUI.shapeLg
        color: Qt.rgba(OpenUI.surface6.r, OpenUI.surface6.g, OpenUI.surface6.b, OpenUI.glassMenuAlpha)
        border.color: OpenUI.outlineVariant; border.width: 1

        Column {
            anchors.centerIn: parent; spacing: OpenUI.sp2
            Row {
                spacing: OpenUI.sp2; anchors.horizontalCenter: parent.horizontalCenter
                ThemedIcon { name: "utilities-terminal"; ctx: "Apps"; size: 22; color: OpenUI.primary }
                Text { text: "终端"; color: OpenUI.onSurface; font.pixelSize: OpenUI.typeTitle
                       anchors.verticalCenter: parent.verticalCenter }
            }
            Text { text: "启动独立应用..."; anchors.horizontalCenter: parent.horizontalCenter
                   color: OpenUI.onSurfaceVariant; font.pixelSize: OpenUI.typeLabelM }
        }
    }
    Component.onCompleted: {
        shell.launchApp("openos-terminal")
        termWin.visible = false
    }
}
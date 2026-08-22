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

/* OPENOS 文本编辑器 (OPENUI 浮窗)
 * 简单文本编辑, 支持多行输入/搜索/替换
 */
Window {
    id: editorWin
    visible: false
    width: 500; height: 400
    flags: Qt.FramelessWindowHint
    title: "文本编辑器"
    color: "transparent"

    property string fileName: "未命名.txt"
    property string searchText: ""

    Rectangle {
        anchors.fill: parent; anchors.margins: 1
        radius: OpenUI.shapeLg
        color: Qt.rgba(OpenUI.surface6.r, OpenUI.surface6.g, OpenUI.surface6.b, OpenUI.glassMenuAlpha)
        border.color: OpenUI.outlineVariant; border.width: 1
        clip: true

        Column {
            anchors.fill: parent; anchors.margins: OpenUI.sp3; spacing: OpenUI.sp2

            // 标题栏
            Row { width: parent.width; spacing: OpenUI.sp2
                ThemedIcon { name: "document-edit"; ctx: "Actions"; size: 16; color: OpenUI.onSurface
                             anchors.verticalCenter: parent.verticalCenter }
                Text { text: fileName; color: OpenUI.onSurface
                       font.pixelSize: OpenUI.typeTitle; font.bold: true
                       verticalAlignment: Text.AlignVCenter }
                Item { width: parent.width - 190; height: 1 }
                Rectangle {
                    width: 24; height: 24; radius: OpenUI.shapeXs
                    color: searchHover.hovered ? Qt.rgba(OpenUI.onSurface.r,OpenUI.onSurface.g,OpenUI.onSurface.b,OpenUI.hoverAlpha) : "transparent"
                    ThemedIcon { anchors.centerIn: parent; name: "edit-find"; ctx: "Actions"; size: 14; color: OpenUI.onSurfaceVariant }
                    MouseArea { id: searchHover; anchors.fill: parent; hoverEnabled: true
                        onClicked: searchBar.visible = !searchBar.visible }
                }
            }

            // 搜索栏
            Rectangle {
                id: searchBar; visible: false
                width: parent.width; height: 28; radius: OpenUI.shapeXs
                color: Qt.rgba(OpenUI.surfaceBright.r, OpenUI.surfaceBright.g, OpenUI.surfaceBright.b, 0.3)
                TextInput {
                    anchors.fill: parent; anchors.margins: 6
                    color: OpenUI.onSurface; font.pixelSize: 12
                    placeholderText: "搜索..."
                    onTextChanged: editorWin.searchText = text
                }
            }

            // 编辑区
            Rectangle {
                width: parent.width; height: parent.height - 80
                radius: OpenUI.shapeXs
                color: Qt.rgba(OpenUI.surfaceBright.r, OpenUI.surfaceBright.g, OpenUI.surfaceBright.b, 0.2)
                Flickable {
                    anchors.fill: parent; anchors.margins: 2
                    contentWidth: editor.width; contentHeight: editor.height
                    clip: true
                    TextEdit {
                        id: editor
                        width: Math.max(parent.width, 400)
                        height: Math.max(parent.height, 400)
                        color: OpenUI.onSurface
                        font.pixelSize: 13
                        font.family: "monospace"
                        wrapMode: Text.WrapAtWordBoundaryOrAnywhere
                        selectByMouse: true
                        text: ""
                    }
                }
            }
        }
    }
}
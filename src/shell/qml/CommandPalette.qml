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

/* OPENOS 命令面板 (Ctrl+K 唤起, 键盘优先 NUI2 §8.1)
 * 模糊搜索: 启动应用 / 系统命令 / 切换工作区
 */
Rectangle {
    id: palette
    visible: false
    width: 480
    height: 320
    radius: OpenUI.shapeLg
    color: Qt.rgba(OpenUI.surface6.r, OpenUI.surface6.g, OpenUI.surface6.b,
                   OpenUI.glassMenuAlpha)
    border.color: OpenUI.outlineVariant; border.width: 1

    property var commands: ListModel {}

    opacity: 0; scale: 0.95
    onVisibleChanged: {
        if (visible) { opacity = 0; scale = 0.95; anim.restart(); input.forceActiveFocus() }
    }
    ParallelAnimation {
        id: anim
        NumberAnimation { target: palette; property: "opacity"; to: 1;
            duration: OpenUI.dur150; easing.type: Easing.OutCubic }
        NumberAnimation { target: palette; property: "scale"; to: 1;
            duration: OpenUI.dur150; easing.type: Easing.OutCubic }
    }

    Column {
        anchors.fill: parent
        anchors.margins: OpenUI.sp3
        spacing: OpenUI.sp2

        TextField {
            id: input
            width: parent.width; height: 36
            placeholderText: "输入命令或搜索…"
            placeholderTextColor: OpenUI.onSurfaceVariant
            color: OpenUI.onSurface; font.pixelSize: OpenUI.typeBodyM
            Rectangle {
                z: -1; anchors.fill: parent; radius: OpenUI.shapeXs
                color: Qt.rgba(OpenUI.surfaceBright.r, OpenUI.surfaceBright.g,
                               OpenUI.surfaceBright.b, 0.5)
            }
            Keys.onEscapePressed: palette.visible = false
            onTextChanged: filter()
        }

        ListView {
            id: list
            width: parent.width; height: parent.height - 50
            clip: true
            model: filteredModel
            delegate: Rectangle {
                width: list.width; height: 30; radius: OpenUI.shapeXs
                color: list.currentIndex === index
                       ? Qt.rgba(OpenUI.onSurface.r, OpenUI.onSurface.g,
                                 OpenUI.onSurface.b, OpenUI.hoverAlpha)
                       : "transparent"
                Row {
                    anchors.fill: parent; anchors.leftMargin: OpenUI.sp3
                    spacing: OpenUI.sp2
                    ThemedIcon {
                        width: 20; height: parent.height
                        name: model.icon; ctx: "Actions"; size: 14; color: OpenUI.primary
                        anchors.verticalCenter: parent.verticalCenter
                    }
                    Text {
                        height: parent.height; verticalAlignment: Text.AlignVCenter
                        text: model.label; color: OpenUI.onSurface; font.pixelSize: 13
                    }
                }
                MouseArea {
                    anchors.fill: parent
                    onClicked: { list.currentIndex = index; run() }
                }
            }
            Keys.onReturnPressed: run()
            Keys.onUpPressed: list.currentIndex = Math.max(0, list.currentIndex - 1)
            Keys.onDownPressed: list.currentIndex = Math.min(list.count - 1, list.currentIndex + 1)
        }
    }

    ListModel { id: filteredModel }

    function build() {
        commands.clear()
        commands.append({ icon: "open-menu",        label: "打开应用抽屉", action: "launcher" })
        commands.append({ icon: "system-reboot",   label: "重启", action: "reboot" })
        commands.append({ icon: "system-power",    label: "关机", action: "poweroff" })
        commands.append({ icon: "applications-system", label: "切换工作区", action: "workspace" })
        commands.append({ icon: "preferences-system", label: "打开设置", action: "settings" })
        filter()
    }
    function filter() {
        filteredModel.clear()
        for (var i = 0; i < commands.count; i++) {
            var it = commands.get(i)
            if (it.label.toLowerCase().indexOf(input.text.toLowerCase()) >= 0)
                filteredModel.append(it)
        }
        list.currentIndex = 0
    }
    function run() {
        var it = filteredModel.get(list.currentIndex)
        console.log("palette:", it.action)
        palette.visible = false
    }
}

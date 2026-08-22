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

import QtQuick 2.15; import QtQuick.Window 2.15
/* OPENOS 取色器 (OPENUI)
 * 屏幕取色 + 色板浏览 + 颜色值复制 (HEX / RGB / HSL)
 * 快捷键: Shift+Ctrl+C
 */
Window {
    id: picker
    visible: false
    width: 360; height: 400
    flags: Qt.FramelessWindowHint
    title: "取色器"
    color: OpenUI.background

    property int dragX: 0; property int dragY: 0; property bool dragging: false
    property string hexColor: "#00BCD4"
    property int rVal: 0; property int gVal: 180; property int bVal: 212
    property int tab: 0

    // 预设色板
    property var palette: [
        "#F44336","#E91E63","#9C27B0","#673AB7","#3F51B5","#2196F3","#03A9F4","#00BCD4",
        "#009688","#4CAF50","#8BC34A","#CDDC39","#FFEB3B","#FFC107","#FF9800","#FF5722",
        "#607D8B","#795548","#9E9E9E","#000000","#FFFFFF","#F5F5F5"
    ]

    function updateFromHex(h) {
        hexColor = h
        rVal = parseInt(h.substr(1,2), 16)
        gVal = parseInt(h.substr(3,2), 16)
        bVal = parseInt(h.substr(5,2), 16)
    }

    function hexToHsl(h) {
        var r = parseInt(h.substr(1,2), 16) / 255
        var g = parseInt(h.substr(3,2), 16) / 255
        var b = parseInt(h.substr(5,2), 16) / 255
        var max = Math.max(r,g,b), min = Math.min(r,g,b), h2 = 0, s = 0, l = (max+min)/2
        if (max !== min) {
            var d = max - min
            s = l > 0.5 ? d / (2 - max - min) : d / (max + min)
            switch (max) {
                case r: h2 = ((g-b)/d + (g<b?6:0)) / 6; break
                case g: h2 = ((b-r)/d + 2) / 6; break
                case b: h2 = ((r-g)/d + 4) / 6; break
            }
        }
        return {h: Math.round(h2*360), s: Math.round(s*100), l: Math.round(l*100)}
    }

    function copyToClipboard(text) {
        // 生产: 对接剪贴板子系统
        console.log("Copied: " + text)
    }

    // 关闭
    Rectangle { anchors.top: parent.top; anchors.right: parent.right; anchors.margins: 6; z: 10; width: 22; height: 22; radius: OpenUI.shapeFull
        color: ch.hovered ? Qt.rgba(OpenUI.error.r,OpenUI.error.g,OpenUI.error.b,0.3) : "transparent"
        Text { anchors.centerIn: parent; text: "\u00D7"; color: ch.hovered ? OpenUI.error : OpenUI.onSurfaceVariant; font.pixelSize: 14 }
        MouseArea { id: ch; anchors.fill: parent; hoverEnabled: true; onClicked: picker.visible = false } }

    Rectangle { anchors.fill: parent; anchors.margins: 1; radius: OpenUI.shapeLg
        color: Qt.rgba(OpenUI.neutral10.r,OpenUI.neutral10.g,OpenUI.neutral10.b,0.95); border.color: OpenUI.outlineVariant; border.width: 1; clip: true

        // 拖拽
        Rectangle { anchors.top: parent.top; anchors.left: parent.left; anchors.right: parent.right; height: 28; color: "transparent"
            Text { anchors.left: parent.left; anchors.leftMargin: OpenUI.sp3; anchors.verticalCenter: parent.verticalCenter; text: "取色器"; color: OpenUI.onSurface; font.pixelSize: OpenUI.typeLabelL }
            MouseArea { anchors.fill: parent; onPressed: { dragX = mouse.x; dragY = mouse.y; dragging = true }; onMouseXChanged: { if (dragging) { picker.x += mouse.x - dragX; picker.y += mouse.y - dragY } }; onReleased: dragging = false } }

        Column { anchors.fill: parent; anchors.topMargin: 32; anchors.margins: OpenUI.sp3; spacing: OpenUI.sp3

            // 当前颜色预览
            Rectangle { width: parent.width; height: 80; radius: OpenUI.shapeMd; color: hexColor; border.color: OpenUI.outlineVariant; border.width: 1 }

            // 色板
            Flow { width: parent.width; spacing: 6
                Repeater { model: palette
                    Rectangle { width: 32; height: 32; radius: 6; color: modelData; border.color: hexColor === modelData ? OpenUI.primary : OpenUI.outlineVariant; border.width: hexColor === modelData ? 2 : 1
                        MouseArea { anchors.fill: parent; hoverEnabled: true; onClicked: updateFromHex(modelData) } } } }

            // 颜色值显示
            Row { width: parent.width; spacing: OpenUI.sp2
                Column { width: parent.width / 3; spacing: 4
                    Text { text: "HEX"; color: OpenUI.onSurfaceVariant; font.pixelSize: OpenUI.typeLabelS }
                    Rectangle { width: parent.width; height: 28; radius: OpenUI.shapeXs; color: Qt.rgba(OpenUI.surfaceBright.r,OpenUI.surfaceBright.g,OpenUI.surfaceBright.b,0.15)
                        Text { anchors.centerIn: parent; text: hexColor; color: OpenUI.onSurface; font.pixelSize: OpenUI.typeLabelM }
                        MouseArea { anchors.fill: parent; onClicked: copyToClipboard(hexColor) } } }
                Column { width: parent.width / 3; spacing: 4
                    Text { text: "RGB"; color: OpenUI.onSurfaceVariant; font.pixelSize: OpenUI.typeLabelS }
                    Rectangle { width: parent.width; height: 28; radius: OpenUI.shapeXs; color: Qt.rgba(OpenUI.surfaceBright.r,OpenUI.surfaceBright.g,OpenUI.surfaceBright.b,0.15)
                        Text { anchors.centerIn: parent; text: rVal+","+gVal+","+bVal; color: OpenUI.onSurface; font.pixelSize: OpenUI.typeLabelM }
                        MouseArea { anchors.fill: parent; onClicked: copyToClipboard(rVal+","+gVal+","+bVal) } } }
                Column { width: parent.width / 3; spacing: 4
                    Text { text: "HSL"; color: OpenUI.onSurfaceVariant; font.pixelSize: OpenUI.typeLabelS }
                    Rectangle { width: parent.width; height: 28; radius: OpenUI.shapeXs; color: Qt.rgba(OpenUI.surfaceBright.r,OpenUI.surfaceBright.g,OpenUI.surfaceBright.b,0.15)
                        Text { anchors.centerIn: parent; text: { var h = hexToHsl(hexColor); return h.h+"\u00B0 "+h.s+"% "+h.l+"%" }; color: OpenUI.onSurface; font.pixelSize: OpenUI.typeLabelM }
                        MouseArea { anchors.fill: parent; onClicked: { var h = hexToHsl(hexColor); copyToClipboard(h.h+","+h.s+"%,"+h.l+"%") } } } } }

            // 调节滑块
            Column { width: parent.width; spacing: OpenUI.sp1
                Repeater { model: [{l:"R",v:rVal},{l:"G",v:gVal},{l:"B",v:bVal}]
                    Row { width: parent.width; spacing: OpenUI.sp2
                        Text { text: modelData.l; width: 16; color: OpenUI.onSurface; font.pixelSize: OpenUI.typeLabelM; verticalAlignment: Text.AlignVCenter }
                        Rectangle { width: parent.width - 70; height: 6; radius: 3; anchors.verticalCenter: parent.verticalCenter; color: Qt.rgba(OpenUI.surfaceBright.r,OpenUI.surfaceBright.g,OpenUI.surfaceBright.b,0.2)
                            Rectangle { width: modelData.v / 255 * parent.width; height: 6; radius: 3; color: modelData.l === "R" ? "#F44336" : modelData.l === "G" ? "#4CAF50" : "#2196F3" } }
                        Text { text: Math.round(modelData.v); width: 30; color: OpenUI.onSurfaceVariant; font.pixelSize: OpenUI.typeLabelS; verticalAlignment: Text.AlignVCenter; horizontalAlignment: Text.AlignRight } } } } } } }
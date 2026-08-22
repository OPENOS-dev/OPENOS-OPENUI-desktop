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
/* OPENOS 磁盘分析器 (OPENUI)
 * 显示磁盘分区 / 目录占用 / 可视化饼图
 * 快捷键: Shift+Ctrl+D
 */
Window {
    id: du
    visible: false
    width: 520; height: 400
    flags: Qt.FramelessWindowHint
    title: "磁盘分析"
    color: OpenUI.background

    property int dragX: 0; property int dragY: 0; property bool dragging: false
    property int tab: 0

    property var partitions: ListModel {
        ListElement { mount:"/";      total:256;  used:187;  fs:"ext4";  label:"系统根分区" }
        ListElement { mount:"/home";  total:512;  used:214;  fs:"ext4";  label:"用户数据" }
        ListElement { mount:"/boot";  total:1;    used:0.3;  fs:"vfat";  label:"启动分区" }
        ListElement { mount:"/var";   total:32;   used:18;   fs:"ext4";  label:"可变数据" }
    }

    property var dirs: ListModel {
        ListElement { path:"/usr";     size:6.2;  files:14230; pct:28 }
        ListElement { path:"/home";    size:4.8;  files:8920;  pct:22 }
        ListElement { path:"/var";     size:3.1;  files:5610;  pct:14 }
        ListElement { path:"/opt";     size:2.5;  files:1230;  pct:11 }
        ListElement { path:"/etc";     size:1.8;  files:4720;  pct:8 }
        ListElement { path:"/lib";     size:1.5;  files:3420;  pct:7 }
        ListElement { path:"/boot";    size:0.3;  files:340;   pct:1 }
    }

    // 关闭
    Rectangle { anchors.top: parent.top; anchors.right: parent.right; anchors.margins: 6; z: 10; width: 22; height: 22; radius: OpenUI.shapeFull
        color: ch.hovered ? Qt.rgba(OpenUI.error.r,OpenUI.error.g,OpenUI.error.b,0.3) : "transparent"
        Text { anchors.centerIn: parent; text: "\u00D7"; color: ch.hovered ? OpenUI.error : OpenUI.onSurfaceVariant; font.pixelSize: 14 }
        MouseArea { id: ch; anchors.fill: parent; hoverEnabled: true; onClicked: du.visible = false } }

    Rectangle { anchors.fill: parent; anchors.margins: 1; radius: OpenUI.shapeLg
        color: Qt.rgba(OpenUI.neutral10.r,OpenUI.neutral10.g,OpenUI.neutral10.b,0.95); border.color: OpenUI.outlineVariant; border.width: 1; clip: true

        // 拖拽
        Rectangle { anchors.top: parent.top; anchors.left: parent.left; anchors.right: parent.right; height: 28; color: "transparent"
            Text { anchors.left: parent.left; anchors.leftMargin: OpenUI.sp3; anchors.verticalCenter: parent.verticalCenter; text: "磁盘分析"; color: OpenUI.onSurface; font.pixelSize: OpenUI.typeLabelL }
            MouseArea { anchors.fill: parent; onPressed: { dragX = mouse.x; dragY = mouse.y; dragging = true }; onMouseXChanged: { if (dragging) { du.x += mouse.x - dragX; du.y += mouse.y - dragY } }; onReleased: dragging = false } }

        // 标签切换
        Row { anchors.top: parent.top; anchors.topMargin: 32; anchors.horizontalCenter: parent.horizontalCenter; spacing: OpenUI.sp2
            Repeater { model: ["分区","目录"]
                Rectangle { width: 60; height: 26; radius: 13; color: tab === index ? Qt.rgba(OpenUI.primary.r,OpenUI.primary.g,OpenUI.primary.b,0.2) : "transparent"
                    Text { anchors.centerIn: parent; text: modelData; color: tab === index ? OpenUI.primary : OpenUI.onSurfaceVariant; font.pixelSize: OpenUI.typeLabelM }
                    MouseArea { anchors.fill: parent; onClicked: tab = index } } } }

        // 分区视图
        ListView { visible: tab === 0; anchors.top: parent.top; anchors.topMargin: 64; anchors.bottom: parent.bottom; anchors.left: parent.left; anchors.right: parent.right; anchors.margins: OpenUI.sp3; spacing: OpenUI.sp2
            model: partitions; clip: true
            delegate: Rectangle { width: parent.width; height: 56; radius: OpenUI.shapeXs; color: Qt.rgba(OpenUI.surfaceBright.r,OpenUI.surfaceBright.g,OpenUI.surfaceBright.b,0.1)
                Column { anchors.fill: parent; anchors.margins: OpenUI.sp2; spacing: 4
                    Row { width: parent.width; spacing: OpenUI.sp2
                        Text { text: modelData.mount; color: OpenUI.onSurface; font.pixelSize: OpenUI.typeTitle; font.bold: true }
                        Text { text: "(" + modelData.label + ")"; color: OpenUI.onSurfaceVariant; font.pixelSize: OpenUI.typeLabelM; anchors.verticalCenter: parent.verticalCenter }
                        Item { width: parent.width - 300; height: 1 }
                        Text { text: modelData.fs; color: OpenUI.onSurfaceDisabled; font.pixelSize: OpenUI.typeLabelS; anchors.verticalCenter: parent.verticalCenter }
                        Text { text: Math.round(modelData.used) + "G / " + modelData.total + "G"; color: OpenUI.onSurface; font.pixelSize: OpenUI.typeLabelM; anchors.verticalCenter: parent.verticalCenter } }
                    Rectangle { width: parent.width; height: 6; radius: 3; color: Qt.rgba(OpenUI.surfaceBright.r,OpenUI.surfaceBright.g,OpenUI.surfaceBright.b,0.2)
                        Rectangle { width: (modelData.used / modelData.total) * parent.width; height: 6; radius: 3; color: modelData.used / modelData.total > 0.85 ? OpenUI.error : OpenUI.primary } } } } }

        // 目录视图
        ListView { visible: tab === 1; anchors.top: parent.top; anchors.topMargin: 64; anchors.bottom: parent.bottom; anchors.left: parent.left; anchors.right: parent.right; anchors.margins: OpenUI.sp3; spacing: OpenUI.sp2
            model: dirs; clip: true
            delegate: Rectangle { width: parent.width; height: 44; radius: OpenUI.shapeXs; color: Qt.rgba(OpenUI.surfaceBright.r,OpenUI.surfaceBright.g,OpenUI.surfaceBright.b,0.1)
                Row { anchors.fill: parent; anchors.margins: OpenUI.sp2; spacing: OpenUI.sp2
                    Text { text: modelData.path; width: 120; color: OpenUI.onSurface; font.pixelSize: OpenUI.typeLabelM; verticalAlignment: Text.AlignVCenter }
                    Rectangle { width: parent.width - 280; height: 8; radius: 4; anchors.verticalCenter: parent.verticalCenter; color: Qt.rgba(OpenUI.surfaceBright.r,OpenUI.surfaceBright.g,OpenUI.surfaceBright.b,0.2)
                        Rectangle { width: modelData.pct / 100 * parent.width; height: 8; radius: 4; color: OpenUI.secondary } }
                    Text { text: modelData.size + " GB"; width: 50; color: OpenUI.onSurface; font.pixelSize: OpenUI.typeLabelM; verticalAlignment: Text.AlignVCenter; horizontalAlignment: Text.AlignRight }
                    Text { text: modelData.files + " 文件"; width: 70; color: OpenUI.onSurfaceVariant; font.pixelSize: OpenUI.typeLabelS; verticalAlignment: Text.AlignVCenter; horizontalAlignment: Text.AlignRight } } } } } } }
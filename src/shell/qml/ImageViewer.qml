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
Window { id: win; visible: false; width: 600; height: 420; minimumWidth: 360; minimumHeight: 280; flags: Qt.FramelessWindowHint; title: "图片查看器"; color: "transparent"
  property bool gridMode: true; property int currentIndex: -1; property int zoomLevel: 100; property int dragX: 0; property int dragY: 0; property bool dragging: false
  property var images: [{n:"示例 1",c:"#E65100"},{n:"示例 2",c:"#00695C"},{n:"示例 3",c:"#1B5E20"},{n:"示例 4",c:"#37474F"},{n:"示例 5",c:"#1A237E"},{n:"示例 6",c:"#AD1457"},{n:"示例 7",c:"#33691E"},{n:"示例 8",c:"#0D47A1"}]
  Rectangle { anchors.top: parent.top; anchors.right: parent.right; anchors.margins: 6; z: 10; width: 22; height: 22; radius: OpenUI.shapeFull
    color: ch.hovered ? Qt.rgba(OpenUI.error.r,OpenUI.error.g,OpenUI.error.b,0.3) : "transparent"
    Text { anchors.centerIn: parent; text: "\u00D7"; color: ch.hovered ? OpenUI.error : OpenUI.onSurfaceVariant; font.pixelSize: 14 }
    MouseArea { id: ch; anchors.fill: parent; hoverEnabled: true; onClicked: win.visible = false } }
  Rectangle { anchors.fill: parent; anchors.margins: 1; radius: OpenUI.shapeLg
    color: Qt.rgba(OpenUI.neutral10.r,OpenUI.neutral10.g,OpenUI.neutral10.b,0.95); border.color: OpenUI.outlineVariant; border.width: 1; clip: true
    Rectangle { anchors.top: parent.top; anchors.left: parent.left; anchors.right: parent.right; height: 28; color: "transparent"
      Text { anchors.left: parent.left; anchors.leftMargin: OpenUI.sp3; anchors.verticalCenter: parent.verticalCenter; text: "图片查看器"; color: OpenUI.onSurface; font.pixelSize: OpenUI.typeLabelL }
      MouseArea { anchors.fill: parent; onPressed: { dragX = mouse.x; dragY = mouse.y; dragging = true }; onMouseXChanged: { if (dragging) { win.x += mouse.x - dragX; win.y += mouse.y - dragY } }; onReleased: dragging = false } }
    // 工具栏
    Row { anchors.top: parent.top; anchors.topMargin: 32; anchors.left: parent.left; anchors.right: parent.right; anchors.margins: OpenUI.sp2; height: 28; spacing: OpenUI.sp2
      Text { text: images.length + " 张图片"; anchors.verticalCenter: parent.verticalCenter; color: OpenUI.onSurfaceVariant; font.pixelSize: OpenUI.typeLabelM }
      Item { width: parent.width - 220; height: 1 }
      Rectangle { width: 24; height: 24; radius: OpenUI.shapeXs; color: gh.hovered ? Qt.rgba(OpenUI.onSurface.r,OpenUI.onSurface.g,OpenUI.onSurface.b,OpenUI.hoverAlpha) : "transparent"; anchors.verticalCenter: parent.verticalCenter
        ThemedIcon { anchors.centerIn: parent; name: "view-grid"; ctx: "Actions"; size: 14; color: gridMode ? OpenUI.primary : OpenUI.onSurfaceVariant }
        MouseArea { id: gh; anchors.fill: parent; hoverEnabled: true; onClicked: gridMode = true } }
      Rectangle { width: 24; height: 24; radius: OpenUI.shapeXs; color: pv.hovered ? Qt.rgba(OpenUI.onSurface.r,OpenUI.onSurface.g,OpenUI.onSurface.b,OpenUI.hoverAlpha) : "transparent"; anchors.verticalCenter: parent.verticalCenter
        ThemedIcon { anchors.centerIn: parent; name: "view-grid-filled"; ctx: "Actions"; size: 14; color: !gridMode ? OpenUI.primary : OpenUI.onSurfaceVariant }
        MouseArea { id: pv; anchors.fill: parent; hoverEnabled: true; onClicked: gridMode = false } } }
    // 缩略图网格
    GridView { visible: gridMode; anchors.top: parent.top; anchors.topMargin: 64; anchors.bottom: parent.bottom; anchors.left: parent.left; anchors.right: parent.right; anchors.margins: OpenUI.sp3; model: images; cellWidth: 110; cellHeight: 110; interactive: true
      delegate: Rectangle { width: 90; height: 90; radius: OpenUI.shapeMd; color: Qt.rgba(OpenUI.surfaceBright.r,OpenUI.surfaceBright.g,OpenUI.surfaceBright.b,0.15); border.color: currentIndex === index ? OpenUI.primary : "transparent"; border.width: 2
        Column { anchors.centerIn: parent; spacing: 2
          Rectangle { width: 48; height: 48; radius: OpenUI.shapeSm; anchors.horizontalCenter: parent.horizontalCenter; color: modelData.c
            ThemedIcon { anchors.centerIn: parent; name: "image-missing"; ctx: "Status"; size: 24; color: Qt.rgba(1,1,1,0.4) } }
          Text { text: modelData.n; anchors.horizontalCenter: parent.horizontalCenter; color: OpenUI.onSurface; font.pixelSize: 10; elide: Text.ElideRight; width: 80; horizontalAlignment: Text.AlignHCenter } }
        MouseArea { anchors.fill: parent; hoverEnabled: true; onClicked: { currentIndex = index; gridMode = false; zoomLevel = 100 } } } }
    // 单图预览
    Item { visible: !gridMode; anchors.top: parent.top; anchors.topMargin: 64; anchors.bottom: parent.bottom; anchors.left: parent.left; anchors.right: parent.right
      Rectangle { anchors.left: parent.left; anchors.leftMargin: 8; anchors.verticalCenter: parent.verticalCenter; width: 28; height: 28; radius: 14; color: pr.hovered ? Qt.rgba(OpenUI.onSurface.r,OpenUI.onSurface.g,OpenUI.onSurface.b,0.2) : Qt.rgba(0,0,0,0.4); z: 2
        ThemedIcon { anchors.centerIn: parent; name: "chevron-left"; ctx: "Navigation"; size: 14; color: OpenUI.onSurface }
        MouseArea { id: pr; anchors.fill: parent; hoverEnabled: true; onClicked: { currentIndex = (currentIndex - 1 + images.length) % images.length } } }
      Rectangle { anchors.right: parent.right; anchors.rightMargin: 8; anchors.verticalCenter: parent.verticalCenter; width: 28; height: 28; radius: 14; color: nx.hovered ? Qt.rgba(OpenUI.onSurface.r,OpenUI.onSurface.g,OpenUI.onSurface.b,0.2) : Qt.rgba(0,0,0,0.4); z: 2
        ThemedIcon { anchors.centerIn: parent; name: "chevron-right"; ctx: "Navigation"; size: 14; color: OpenUI.onSurface }
        MouseArea { id: nx; anchors.fill: parent; hoverEnabled: true; onClicked: { currentIndex = (currentIndex + 1) % images.length } } }
      Rectangle { anchors.centerIn: parent; width: 200; height: 160; radius: OpenUI.shapeSm; color: currentIndex >= 0 ? images[currentIndex].c : OpenUI.surfaceDim
        ThemedIcon { anchors.centerIn: parent; name: "image-missing"; ctx: "Status"; size: 60; color: Qt.rgba(1,1,1,0.3) } } } } }
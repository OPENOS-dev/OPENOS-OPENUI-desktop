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

/* OPENOS 系统监视器 (OPENUI)
 * 分类: 进程 / CPU+内存 / 磁盘
 * 对接: openos-sysmond (OAK 加密 API) — 生产经 liboak
 */
Window {
    id: sysMon
    visible: false
    width: 640; height: 480
    flags: Qt.FramelessWindowHint
    title: "系统监视器"
    color: OpenUI.background

    property int currentPage: 0

    // ---- 模拟数据 ----
    property variant processModel: ListModel {
        ListElement { name: "init";          pid: 1;     cpu: 0.1; mem: 0.3 }
        ListElement { name: "kthreadd";      pid: 2;     cpu: 0.0; mem: 0.0 }
        ListElement { name: "rcu_gp";        pid: 3;     cpu: 0.2; mem: 0.0 }
        ListElement { name: "systemd";       pid: 1;     cpu: 0.5; mem: 1.2 }
        ListElement { name: "kworker/0:0";   pid: 6;     cpu: 0.8; mem: 0.0 }
        ListElement { name: "ksoftirqd/0";   pid: 7;     cpu: 0.3; mem: 0.0 }
        ListElement { name: "migration/0";   pid: 8;     cpu: 0.0; mem: 0.0 }
        ListElement { name: "openos-shell";  pid: 1024;  cpu: 2.1; mem: 4.5 }
        ListElement { name: "openos-sysmond";pid: 1056;  cpu: 1.3; mem: 2.8 }
        ListElement { name: "openos-panel";  pid: 1088;  cpu: 1.8; mem: 3.2 }
        ListElement { name: "Xorg";          pid: 1200;  cpu: 5.2; mem: 6.7 }
        ListElement { name: "picom";         pid: 1248;  cpu: 3.6; mem: 2.1 }
        ListElement { name: "firefox";       pid: 2048;  cpu: 12.4;mem: 18.5 }
        ListElement { name: "alacritty";     pid: 2100;  cpu: 1.0; mem: 1.5 }
        ListElement { name: "bash";          pid: 2156;  cpu: 0.0; mem: 0.2 }
        ListElement { name: "sshd";          pid: 1800;  cpu: 0.0; mem: 0.4 }
        ListElement { name: "NetworkManager";pid: 900;   cpu: 0.6; mem: 1.1 }
        ListElement { name: "pulseaudio";    pid: 950;   cpu: 1.5; mem: 2.3 }
    }

    property variant diskModel: ListModel {
        ListElement { fs: "/dev/nvme0n1p2"; mount: "/";        total: 256;  used: 98;  avail: 147 }
        ListElement { fs: "/dev/nvme0n1p1"; mount: "/boot";    total: 1;    used: 0.3; avail: 0.6 }
        ListElement { fs: "/dev/sda1";      mount: "/mnt/data"; total: 1024; used: 623; avail: 352 }
        ListElement { fs: "tmpfs";          mount: "/tmp";     total: 8;    used: 1.2; avail: 6.5 }
        ListElement { fs: "zram0";          mount: "swap";     total: 4;    used: 0.8; avail: 3.1 }
    }

    // ---- 排序后进程模型 ----
    property var sortedProcesses: ListModel {}
    function resortProcesses() {
        sortedProcesses.clear();
        var arr = [];
        for (var i = 0; i < processModel.count; i++) {
            var item = processModel.get(i);
            // 搜索过滤
            if (searchText.length > 0 && item.name.indexOf(searchText.toLowerCase()) < 0)
                continue;
            arr.push({ name: item.name, pid: item.pid, cpu: item.cpu, mem: item.mem });
        }
        // 按 CPU 降序
        arr.sort(function(a, b) { return b.cpu - a.cpu; });
        for (var j = 0; j < arr.length; j++)
            sortedProcesses.append(arr[j]);
    }
    onProcessModelChanged: resortProcesses()
    property string searchText: ""

    Rectangle {
        anchors.fill: parent
        color: OpenUI.background

        // ---- 侧栏导航 ----
        Rectangle {
            width: 160; anchors.top: parent.top; anchors.bottom: parent.bottom
            anchors.left: parent.left
            color: Qt.rgba(OpenUI.surface.r, OpenUI.surface.g, OpenUI.surface.b, 0.9)
            Column {
                anchors.fill: parent; anchors.topMargin: OpenUI.sp4
                Repeater {
                    model: ListModel {
                        ListElement { icon: "preferences-system"; label: "进程" }
                        ListElement { icon: "cpu"; label: "CPU/内存" }
                        ListElement { icon: "drive-harddisk"; label: "磁盘" }
                    }
                    Rectangle {
                        width: 160; height: 40; radius: OpenUI.shapeXs
                        color: sysMon.currentPage === index
                               ? Qt.rgba(OpenUI.primary.r, OpenUI.primary.g,
                                         OpenUI.primary.b, 0.2)
                               : (hover.hovered
                                  ? Qt.rgba(OpenUI.onSurface.r, OpenUI.onSurface.g,
                                            OpenUI.onSurface.b, OpenUI.hoverAlpha)
                                  : "transparent")
                        Row {
                            anchors.fill: parent; anchors.leftMargin: OpenUI.sp4
                            spacing: OpenUI.sp2
                            ThemedIcon {
                                width: 24; height: parent.height
                                name: model.icon; ctx: "Apps"; size: 16; color: OpenUI.primary
                                anchors.verticalCenter: parent.verticalCenter
                            }
                            Text {
                                height: parent.height
                                verticalAlignment: Text.AlignVCenter
                                text: model.label; color: OpenUI.onSurface
                                font.pixelSize: OpenUI.typeLabelL
                            }
                        }
                        MouseArea {
                            id: hover; anchors.fill: parent; hoverEnabled: true
                            onClicked: sysMon.currentPage = index
                        }
                    }
                }
                // 底部留白
                Item { width: 1; height: 1; Layout.fillHeight: true }
            }
        }

        // ---- 内容区 ----
        Rectangle {
            anchors.left: parent.left; anchors.leftMargin: 160
            anchors.right: parent.right; anchors.top: parent.top; anchors.bottom: parent.bottom
            color: "transparent"
            clip: true

            StackLayout {
                anchors.fill: parent; anchors.margins: OpenUI.sp4
                currentIndex: sysMon.currentPage

                // ============ 页面 1: 进程 ============
                Item {
                    Column {
                        anchors.fill: parent
                        spacing: OpenUI.sp3

                        // 搜索栏 + 菜单按钮
                        Row {
                            width: parent.width; height: 32
                            spacing: OpenUI.sp2

                            Rectangle {
                                height: 32; width: parent.width - 40
                                radius: OpenUI.shapeXs
                                color: Qt.rgba(OpenUI.surface6.r, OpenUI.surface6.g,
                                               OpenUI.surface6.b, 0.4)
                                border.color: Qt.rgba(OpenUI.outline.r, OpenUI.outline.g,
                                                      OpenUI.outline.b, 0.3)
                                border.width: 1
                                TextInput {
                                    id: searchInput
                                    anchors.fill: parent; anchors.leftMargin: OpenUI.sp3
                                    verticalAlignment: Text.AlignVCenter
                                    color: OpenUI.onSurface
                                    font.pixelSize: OpenUI.typeBodyM
                                    onTextChanged: {
                                        sysMon.searchText = text.toLowerCase();
                                        sysMon.resortProcesses();
                                    }
                                }
                                Text {
                                    anchors.fill: parent; anchors.leftMargin: OpenUI.sp3
                                    verticalAlignment: Text.AlignVCenter
                                    text: "搜索进程..."
                                    color: OpenUI.onSurfaceDisabled
                                    font.pixelSize: OpenUI.typeBodyM
                                    visible: searchInput.text.length === 0
                                }
                            }

                            // 菜单按钮
                            Rectangle {
                                width: 32; height: 32; radius: OpenUI.shapeXs
                                color: menuHover.hovered
                                       ? Qt.rgba(OpenUI.onSurface.r, OpenUI.onSurface.g,
                                                 OpenUI.onSurface.b, OpenUI.hoverAlpha)
                                       : "transparent"
                                Text {
                                    anchors.centerIn: parent
                                    text: "\u2630"; color: OpenUI.onSurfaceVariant
                                    font.pixelSize: 16
                                }
                                MouseArea {
                                    id: menuHover; anchors.fill: parent; hoverEnabled: true
                                    onClicked: {/* 菜单待实现 */}
                                }
                            }
                        }

                        // 表头
                        Row {
                            width: parent.width; height: 28
                            spacing: 0
                            Rectangle {
                                width: (parent.width - 40) * 0.40; height: parent.height
                                color: "transparent"
                                Text {
                                    anchors.left: parent.left; anchors.leftMargin: OpenUI.sp2
                                    anchors.verticalCenter: parent.verticalCenter
                                    text: "名称"; color: OpenUI.onSurfaceVariant
                                    font.pixelSize: OpenUI.typeLabelS; font.bold: true
                                }
                            }
                            Rectangle {
                                width: (parent.width - 40) * 0.20; height: parent.height
                                color: "transparent"
                                Text {
                                    anchors.left: parent.left; anchors.leftMargin: OpenUI.sp2
                                    anchors.verticalCenter: parent.verticalCenter
                                    text: "PID"; color: OpenUI.onSurfaceVariant
                                    font.pixelSize: OpenUI.typeLabelS; font.bold: true
                                }
                            }
                            Rectangle {
                                width: (parent.width - 40) * 0.20; height: parent.height
                                color: "transparent"
                                Text {
                                    anchors.left: parent.left; anchors.leftMargin: OpenUI.sp2
                                    anchors.verticalCenter: parent.verticalCenter
                                    text: "CPU%"; color: OpenUI.onSurfaceVariant
                                    font.pixelSize: OpenUI.typeLabelS; font.bold: true
                                }
                            }
                            Rectangle {
                                width: (parent.width - 40) * 0.20; height: parent.height
                                color: "transparent"
                                Text {
                                    anchors.left: parent.left; anchors.leftMargin: OpenUI.sp2
                                    anchors.verticalCenter: parent.verticalCenter
                                    text: "MEM%"; color: OpenUI.onSurfaceVariant
                                    font.pixelSize: OpenUI.typeLabelS; font.bold: true
                                }
                            }
                        }

                        // 进程列表
                        Rectangle {
                            width: parent.width
                            height: parent.height - 32 - 28 - OpenUI.sp3 * 2
                            radius: OpenUI.shapeXs
                            color: Qt.rgba(OpenUI.surface.r, OpenUI.surface.g,
                                           OpenUI.surface.b, 0.5)
                            clip: true

                            ListView {
                                anchors.fill: parent
                                model: sysMon.sortedProcesses
                                delegate: Rectangle {
                                    width: parent.width; height: 32
                                    color: procHover.hovered
                                           ? Qt.rgba(OpenUI.onSurface.r, OpenUI.onSurface.g,
                                                     OpenUI.onSurface.b, OpenUI.hoverAlpha)
                                           : "transparent"
                                    Row {
                                        anchors.fill: parent; spacing: 0
                                        Rectangle {
                                            width: (parent.width - 40) * 0.40; height: parent.height
                                            color: "transparent"
                                            Row {
                                                anchors.left: parent.left; anchors.leftMargin: OpenUI.sp2
                                                anchors.verticalCenter: parent.verticalCenter
                                                spacing: OpenUI.sp1
                                                ThemedIcon {
                                                    anchors.verticalCenter: parent.verticalCenter
                                                    name: "preferences-system"; ctx: "Apps"; size: 12; color: OpenUI.primary
                                                }
                                                Text {
                                                    anchors.verticalCenter: parent.verticalCenter
                                                    text: model.name; color: OpenUI.onSurface
                                                    font.pixelSize: OpenUI.typeLabelM
                                                }
                                            }
                                        }
                                        Rectangle {
                                            width: (parent.width - 40) * 0.20; height: parent.height
                                            color: "transparent"
                                            Text {
                                                anchors.left: parent.left; anchors.leftMargin: OpenUI.sp2
                                                anchors.verticalCenter: parent.verticalCenter
                                                text: model.pid; color: OpenUI.onSurfaceVariant
                                                font.pixelSize: OpenUI.typeLabelM
                                            }
                                        }
                                        Rectangle {
                                            width: (parent.width - 40) * 0.20; height: parent.height
                                            color: "transparent"
                                            Text {
                                                anchors.left: parent.left; anchors.leftMargin: OpenUI.sp2
                                                anchors.verticalCenter: parent.verticalCenter
                                                text: model.cpu.toFixed(1); color: OpenUI.primary
                                                font.pixelSize: OpenUI.typeLabelM
                                            }
                                        }
                                        Rectangle {
                                            width: (parent.width - 40) * 0.20; height: parent.height
                                            color: "transparent"
                                            Text {
                                                anchors.left: parent.left; anchors.leftMargin: OpenUI.sp2
                                                anchors.verticalCenter: parent.verticalCenter
                                                text: model.mem.toFixed(1); color: OpenUI.onSurface
                                                font.pixelSize: OpenUI.typeLabelM
                                            }
                                        }
                                    }
                                    MouseArea {
                                        id: procHover; anchors.fill: parent; hoverEnabled: true
                                    }
                                }
                            }
                        }
                    }
                }

                // ============ 页面 2: CPU/内存 ============
                Item {
                    Column {
                        anchors.fill: parent
                        anchors.topMargin: OpenUI.sp2
                        spacing: OpenUI.sp5

                        // CPU 使用率
                        Column {
                            width: parent.width; spacing: OpenUI.sp2
                            Row {
                                width: parent.width
                                Text {
                                    text: "CPU 使用率"; color: OpenUI.onSurface
                                    font.pixelSize: OpenUI.typeTitle
                                }
                                Text {
                                    anchors.right: parent.right
                                    text: "42.5%"; color: OpenUI.primary
                                    font.pixelSize: OpenUI.typeTitle
                                }
                            }
                            Rectangle {
                                width: parent.width; height: 20; radius: OpenUI.shapeFull
                                color: Qt.rgba(OpenUI.surface6.r, OpenUI.surface6.g,
                                               OpenUI.surface6.b, 0.3)
                                Rectangle {
                                    height: parent.height; width: parent.width * 0.425
                                    radius: OpenUI.shapeFull
                                    color: OpenUI.primary
                                    Behavior on width { SmoothedAnimation { duration: OpenUI.dur200 } }
                                }
                            }
                        }

                        // 内存使用率
                        Column {
                            width: parent.width; spacing: OpenUI.sp2
                            Row {
                                width: parent.width
                                Text {
                                    text: "内存使用率"; color: OpenUI.onSurface
                                    font.pixelSize: OpenUI.typeTitle
                                }
                                Text {
                                    anchors.right: parent.right
                                    text: "6.2 / 15.6 GB"; color: OpenUI.onSurfaceVariant
                                    font.pixelSize: OpenUI.typeTitle
                                }
                            }
                            Rectangle {
                                width: parent.width; height: 20; radius: OpenUI.shapeFull
                                color: Qt.rgba(OpenUI.surface6.r, OpenUI.surface6.g,
                                               OpenUI.surface6.b, 0.3)
                                Rectangle {
                                    height: parent.height; width: parent.width * 0.40
                                    radius: OpenUI.shapeFull
                                    color: OpenUI.primary
                                    Behavior on width { SmoothedAnimation { duration: OpenUI.dur200 } }
                                }
                            }
                            Text {
                                text: "已用: 6.2 GB \u00B7 可用: 9.4 GB"
                                color: OpenUI.onSurfaceVariant
                                font.pixelSize: OpenUI.typeLabelM
                            }
                        }

                        // Swap 使用率
                        Column {
                            width: parent.width; spacing: OpenUI.sp2
                            Row {
                                width: parent.width
                                Text {
                                    text: "Swap 使用率"; color: OpenUI.onSurface
                                    font.pixelSize: OpenUI.typeTitle
                                }
                                Text {
                                    anchors.right: parent.right
                                    text: "0.8 / 4.0 GB"; color: OpenUI.onSurfaceVariant
                                    font.pixelSize: OpenUI.typeTitle
                                }
                            }
                            Rectangle {
                                width: parent.width; height: 20; radius: OpenUI.shapeFull
                                color: Qt.rgba(OpenUI.surface6.r, OpenUI.surface6.g,
                                               OpenUI.surface6.b, 0.3)
                                Rectangle {
                                    height: parent.height; width: parent.width * 0.20
                                    radius: OpenUI.shapeFull
                                    color: Qt.rgba(OpenUI.primary.r, OpenUI.primary.g,
                                                   OpenUI.primary.b, 0.6)
                                    Behavior on width { SmoothedAnimation { duration: OpenUI.dur200 } }
                                }
                            }
                            Text {
                                text: "已用: 0.8 GB \u00B7 可用: 3.1 GB"
                                color: OpenUI.onSurfaceVariant
                                font.pixelSize: OpenUI.typeLabelM
                            }
                        }
                    }
                }

                // ============ 页面 3: 磁盘 ============
                Item {
                    Column {
                        anchors.fill: parent
                        anchors.topMargin: OpenUI.sp2
                        spacing: OpenUI.sp4

                        Repeater {
                            model: sysMon.diskModel
                            Rectangle {
                                width: parent.width
                                height: 72
                                radius: OpenUI.shapeSm
                                color: Qt.rgba(OpenUI.surface.r, OpenUI.surface.g,
                                               OpenUI.surface.b, 0.5)
                                Column {
                                    anchors.fill: parent; anchors.margins: OpenUI.sp3
                                    spacing: OpenUI.sp2

                                    // 名称行
                                    Row {
                                        width: parent.width; spacing: OpenUI.sp2
                                        Text {
                                            text: model.mount; color: OpenUI.onSurface
                                            font.pixelSize: OpenUI.typeTitle
                                        }
                                        Text {
                                            text: "(" + model.fs + ")"; color: OpenUI.onSurfaceVariant
                                            font.pixelSize: OpenUI.typeLabelM
                                            anchors.verticalCenter: parent.verticalCenter
                                        }
                                        Text {
                                            anchors.right: parent.right
                                            text: model.used.toFixed(1) + " / " + model.total.toFixed(1) + " GB"
                                            color: OpenUI.onSurfaceVariant
                                            font.pixelSize: OpenUI.typeLabelM
                                        }
                                    }

                                    // 进度条 + 可用空间
                                    Column {
                                        width: parent.width; spacing: OpenUI.sp1
                                        Rectangle {
                                            width: parent.width; height: 10; radius: OpenUI.shapeFull
                                            color: Qt.rgba(OpenUI.surface6.r, OpenUI.surface6.g,
                                                           OpenUI.surface6.b, 0.3)
                                            Rectangle {
                                                height: parent.height
                                                width: parent.width * (model.used / model.total)
                                                radius: OpenUI.shapeFull
                                                color: model.used / model.total > 0.85
                                                       ? OpenUI.error
                                                       : OpenUI.primary
                                                Behavior on width { SmoothedAnimation { duration: OpenUI.dur200 } }
                                            }
                                        }
                                        Text {
                                            text: "可用: " + model.avail.toFixed(1) + " GB"
                                            color: OpenUI.onSurfaceVariant
                                            font.pixelSize: OpenUI.typeLabelS
                                        }
                                    }
                                }
                            }
                        }
                    }
                }
            }
        }

        // ---- 关闭按钮 (右上) ----
        Rectangle {
            x: parent.width - 40; y: 8; width: 32; height: 32; radius: OpenUI.shapeXs
            color: closeHover.hovered
                   ? Qt.rgba(OpenUI.error.r, OpenUI.error.g, OpenUI.error.b, 0.3)
                   : "transparent"
            ThemedIcon {
                anchors.centerIn: parent
                name: "window-close"; ctx: "Actions"; size: 14; color: OpenUI.onSurface
            }
            MouseArea {
                id: closeHover; anchors.fill: parent; hoverEnabled: true
                onClicked: sysMon.visible = false
            }
        }
    }

    Component.onCompleted: resortProcesses()
}
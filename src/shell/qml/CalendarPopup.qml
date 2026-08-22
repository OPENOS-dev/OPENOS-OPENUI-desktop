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

/* OPENOS 日历弹出 (点面板时钟)
 * 月历网格, 今日高亮, 可切月
 */
Rectangle {
    id: calPopup
    visible: false
    width: 260
    height: 300
    radius: OpenUI.shapeLg
    color: Qt.rgba(OpenUI.surface6.r, OpenUI.surface6.g, OpenUI.surface6.b,
                   OpenUI.glassMenuAlpha)
    border.color: OpenUI.outlineVariant; border.width: 1

    property var shownDate: new Date()

    opacity: 0; scale: 0.95
    onVisibleChanged: {
        if (visible) { opacity = 0; scale = 0.95; anim.restart() }
    }
    ParallelAnimation {
        id: anim
        NumberAnimation { target: calPopup; property: "opacity"; to: 1;
            duration: OpenUI.dur200; easing.type: Easing.OutCubic }
        NumberAnimation { target: calPopup; property: "scale"; to: 1;
            duration: OpenUI.dur200; easing.type: Easing.OutCubic }
    }

    Column {
        anchors.fill: parent; anchors.margins: OpenUI.sp4; spacing: OpenUI.sp2

        // 标题 + 切月
        Row { width: parent.width; spacing: OpenUI.sp2
            Text {
                text: Qt.formatDate(calPopup.shownDate, "yyyy年M月")
                color: OpenUI.onSurface; font.pixelSize: OpenUI.typeTitle; font.bold: true
            }
            Item { width: parent.width - 90; height: 1 }
            Repeater {
                model: ["chevron-left", "chevron-right"]
                Rectangle {
                    width: 26; height: 26; radius: OpenUI.shapeXs
                    color: hover.hovered
                           ? Qt.rgba(OpenUI.onSurface.r, OpenUI.onSurface.g,
                                     OpenUI.onSurface.b, OpenUI.hoverAlpha)
                           : "transparent"
                    ThemedIcon { anchors.centerIn: parent; name: modelData; ctx: "Navigation"; size: 16; color: OpenUI.onSurfaceVariant }
                    MouseArea {
                        id: hover; anchors.fill: parent; hoverEnabled: true
                        onClicked: {
                            var d = calPopup.shownDate
                            d.setMonth(d.getMonth() + (index === 0 ? -1 : 1))
                            calPopup.shownDate = d
                        }
                    }
                }
            }
        }

        // 星期表头
        Row { width: parent.width; spacing: 1
            Repeater {
                model: ["日","一","二","三","四","五","六"]
                Text { width: parent.width/7; text: modelData; horizontalAlignment: Text.AlignHCenter
                       color: OpenUI.onSurfaceVariant; font.pixelSize: OpenUI.typeLabelM }
            }
        }

        // 日期网格 (6x7)
        Grid {
            columns: 7; spacing: 1; width: parent.width
            Repeater {
                model: 42
                Rectangle {
                    width: parent.width / 7 - 2; height: 30; radius: OpenUI.shapeXs
                    color: isToday() ? Qt.rgba(OpenUI.primary.r, OpenUI.primary.g,
                                              OpenUI.primary.b, 0.3)
                                    : "transparent"
                    function isToday() {
                        var d = calPopup.shownDate
                        var first = new Date(d.getFullYear(), d.getMonth(), 1)
                        var dayNum = index - first.getDay() + 1
                        var now = new Date()
                        return dayNum === now.getDate() &&
                               d.getMonth() === now.getMonth() &&
                               d.getFullYear() === now.getFullYear()
                    }
                    Text {
                        anchors.centerIn: parent
                        text: {
                            var d = calPopup.shownDate
                            var first = new Date(d.getFullYear(), d.getMonth(), 1)
                            return index - first.getDay() + 1
                        }
                        color: isToday() ? OpenUI.primary : OpenUI.onSurfaceVariant
                        font.pixelSize: OpenUI.typeLabelM
                    }
                }
            }
        }
    }
    MouseArea { anchors.fill: parent.parent; z: -1; onClicked: calPopup.visible = false }
}

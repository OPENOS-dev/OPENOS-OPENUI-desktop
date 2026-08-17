import QtQuick 2.15

/* OPENOS 通知层: 右上角通知卡片列 (OPENUI) */
Column {
    id: notifLayer
    spacing: 6
    visible: shell.notifications.count > 0

    Repeater {
        model: shell.notifications
        Rectangle {
            id: card
            width: 320
            height: 68
            radius: OpenUI.shapeSm   /* 圆角加大 (8->12) */
            color: Qt.rgba(OpenUI.surface6.r, OpenUI.surface6.g, OpenUI.surface6.b,
                           OpenUI.glassCardAlpha)   /* 更高透明度 (毛玻璃) */
            border.color: hover.hovered
                          ? Qt.rgba(OpenUI.primary.r, OpenUI.primary.g,
                                    OpenUI.primary.b, 0.6)
                          : Qt.rgba(OpenUI.outlineVariant.r, OpenUI.outlineVariant.g,
                                    OpenUI.outlineVariant.b, 1.0)
            border.width: 1

            // hover: 整体提亮 + 轻微上浮 (亲和交互)
            scale: hover.hovered ? 1.02 : 1.0
            Behavior on scale { NumberAnimation { duration: OpenUI.dur100 } }
            Behavior on border.color { ColorAnimation { duration: OpenUI.dur100 } }

            // accent 竖条 (hover 加宽)
            Rectangle {
                id: accentBar
                x: 4; y: 6
                width: hover.hovered ? 5 : 3
                height: parent.height - 12
                radius: 2
                color: OpenUI.primary
                Behavior on width { NumberAnimation { duration: OpenUI.dur100 } }
            }
            Text {
                anchors { left: parent.left; leftMargin: 16; top: parent.top; topMargin: 10 }
                text: model.title
                color: OpenUI.onSurface
                font.pixelSize: 13; font.bold: true
                elide: Text.ElideRight; width: parent.width - 40
            }
            Text {
                anchors { left: parent.left; leftMargin: 16; top: parent.top; topMargin: 30 }
                text: model.body
                color: OpenUI.onSurfaceVariant
                font.pixelSize: 12
                elide: Text.ElideRight; width: parent.width - 40
            }
            MouseArea {
                id: hover
                anchors.fill: parent
                hoverEnabled: true
                onClicked: shell.dismissNotification(index)
            }
        }
    }
}

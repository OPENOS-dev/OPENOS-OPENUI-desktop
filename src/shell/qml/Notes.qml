import QtQuick 2.15
import QtQuick.Window 2.15

/* OPENOS 便签启动器 (壳内版)
 * 启动独立应用 openos-notes
 */
Window {
    id: notesWin
    visible: false
    width: 240; height: 100
    flags: Qt.FramelessWindowHint
    title: "便签"
    color: "transparent"

    Rectangle {
        anchors.fill: parent; anchors.margins: 1
        radius: OpenUI.shapeLg
        color: Qt.rgba(OpenUI.surface6.r, OpenUI.surface6.g, OpenUI.surface6.b, OpenUI.glassMenuAlpha)
        border.color: OpenUI.outlineVariant; border.width: 1

        Column {
            anchors.centerIn: parent; spacing: OpenUI.sp2
            Text { text: "\u2712 便签"; anchors.horizontalCenter: parent.horizontalCenter
                   color: OpenUI.onSurface; font.pixelSize: OpenUI.typeTitle }
            Text { text: "启动独立应用..."; anchors.horizontalCenter: parent.horizontalCenter
                   color: OpenUI.onSurfaceVariant; font.pixelSize: OpenUI.typeLabelM }
        }
    }
    Component.onCompleted: {
        shell.launchApp("openos-notes")
        notesWin.visible = false
    }
}
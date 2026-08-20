import QtQuick 2.15
import QtQuick.Window 2.15

/* OPENOS 计算器启动器 (壳内版)
 * 启动独立应用 openos-calc
 */
Window {
    id: calcWin
    visible: false
    width: 240; height: 100
    flags: Qt.FramelessWindowHint
    title: "计算器"
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
                ThemedIcon { name: "accessories-calculator"; ctx: "apps"; size: 22; color: OpenUI.primary }
                Text { text: "计算器"; color: OpenUI.onSurface; font.pixelSize: OpenUI.typeTitle
                       anchors.verticalCenter: parent.verticalCenter }
            }
            Text { text: "启动独立应用..."; anchors.horizontalCenter: parent.horizontalCenter
                   color: OpenUI.onSurfaceVariant; font.pixelSize: OpenUI.typeLabelM }
        }
    }
    Component.onCompleted: {
        shell.launchApp("openos-calc")
        calcWin.visible = false
    }
}
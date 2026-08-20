import QtQuick 2.15
import QtQuick.Window 2.15

/* OPENOS 桌面小部件 (OPENUI 浮窗)
 * 天气/时钟/快捷便签组合
 */
Window {
    id: widgetWin
    visible: false
    width: 280; height: 320
    flags: Qt.FramelessWindowHint
    title: "桌面小部件"
    color: "transparent"

    Rectangle {
        anchors.fill: parent; anchors.margins: 1
        radius: OpenUI.shapeLg
        color: Qt.rgba(OpenUI.surface6.r, OpenUI.surface6.g, OpenUI.surface6.b, OpenUI.glassMenuAlpha)
        border.color: OpenUI.outlineVariant; border.width: 1

        Column {
            anchors.fill: parent; anchors.margins: OpenUI.sp4; spacing: OpenUI.sp3

            // 时钟
            Column { width: parent.width; spacing: OpenUI.sp1
                Text {
                    id: clockDisplay
                    text: Qt.formatTime(new Date(), "HH:mm:ss")
                    color: OpenUI.onSurface; font.pixelSize: 36; font.weight: Font.Light
                    Timer { interval: 1000; running: true; repeat: true
                        onTriggered: clockDisplay.text = Qt.formatTime(new Date(), "HH:mm:ss") }
                }
                Text {
                    text: Qt.formatDate(new Date(), "yyyy年M月d日 dddd")
                    color: OpenUI.onSurfaceVariant; font.pixelSize: OpenUI.typeLabelM
                }
            }

            // 分隔线
            Rectangle { width: parent.width; height: 1; color: Qt.rgba(OpenUI.outlineVariant.r,OpenUI.outlineVariant.g,OpenUI.outlineVariant.b,0.3) }

            // 模拟天气
            Row { width: parent.width; spacing: OpenUI.sp3
                Text { text: "\u2600"; color: OpenUI.statusWarning; font.pixelSize: 28 }
                Column { spacing: 2
                    Text { text: "25\u00B0C 晴"; color: OpenUI.onSurface; font.pixelSize: 18 }
                    Text { text: "湿度: 45%  |  风速: 3m/s"; color: OpenUI.onSurfaceVariant; font.pixelSize: OpenUI.typeLabelS }
                }
            }

            // 分隔线
            Rectangle { width: parent.width; height: 1; color: Qt.rgba(OpenUI.outlineVariant.r,OpenUI.outlineVariant.g,OpenUI.outlineVariant.b,0.3) }

            // 快捷便签
            Column { width: parent.width; spacing: OpenUI.sp1
                Text { text: "\u2712 快捷便签"; color: OpenUI.onSurfaceVariant; font.pixelSize: OpenUI.typeLabelM }
                Rectangle {
                    width: parent.width; height: 60; radius: OpenUI.shapeXs
                    color: Qt.rgba(OpenUI.surfaceBright.r, OpenUI.surfaceBright.g, OpenUI.surfaceBright.b, 0.25)
                    TextInput {
                        anchors.fill: parent; anchors.margins: OpenUI.sp2
                        color: OpenUI.onSurface; font.pixelSize: 12
                        wrapMode: Text.WordWrap
                        placeholderText: "写点什么..."
                    }
                }
            }
        }
    }
}
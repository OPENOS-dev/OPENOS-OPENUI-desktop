import QtQuick 2.15
import QtQuick.Window 2.15

/* OPENOS 外观设置 (OPENUI 浮窗)
 * 壁纸选择/主题色/透明度/字体缩放
 */
Window {
    id: appearanceWin
    visible: false
    width: 360; height: 320
    flags: Qt.FramelessWindowHint
    title: "外观设置"
    color: "transparent"

    property int selectedWallpaper: 0
    property double glassOpacity: 0.72
    property double fontSizeScale: 1.0

    ListModel { id: wallpaperModel
        ListElement { name: "暗色抽象"; color: "#090909" }
        ListElement { name: "深海"; color: "#0D1B2A" }
        ListElement { name: "森林"; color: "#1B3A2D" }
        ListElement { name: "日落"; color: "#2D1B1B" }
        ListElement { name: "极光"; color: "#1B1B2D" }
    }

    Rectangle {
        anchors.fill: parent; anchors.margins: 1
        radius: OpenUI.shapeLg
        color: Qt.rgba(OpenUI.surface6.r, OpenUI.surface6.g, OpenUI.surface6.b, OpenUI.glassMenuAlpha)
        border.color: OpenUI.outlineVariant; border.width: 1

        Column {
            anchors.fill: parent; anchors.margins: OpenUI.sp4; spacing: OpenUI.sp3

            Row {
                spacing: OpenUI.sp2
                Text { text: "外观设置"; color: OpenUI.onSurface; font.pixelSize: OpenUI.typeTitle; font.bold: true
                       anchors.verticalCenter: parent.verticalCenter }
                ThemedIcon { name: "preferences-desktop"; ctx: "apps"; size: 18
                             color: OpenUI.primary; anchors.verticalCenter: parent.verticalCenter }
            }

            // 壁纸选择
            Column { width: parent.width; spacing: OpenUI.sp1
                Text { text: "壁纸"; color: OpenUI.onSurfaceVariant; font.pixelSize: OpenUI.typeLabelM }
                Row { width: parent.width; spacing: OpenUI.sp1; height: 36
                    Repeater {
                        model: wallpaperModel
                        Rectangle {
                            width: (parent.width - 16) / 5; height: 36; radius: OpenUI.shapeXs
                            color: model.color
                            border.width: selectedWallpaper === index ? 2 : 0
                            border.color: selectedWallpaper === index ? OpenUI.primary : "transparent"
                            MouseArea { anchors.fill: parent; hoverEnabled: true
                                onClicked: selectedWallpaper = index }
                        }
                    }
                }
            }

            // 毛玻璃透明度
            Column { width: parent.width; spacing: OpenUI.sp1
                Text { text: "毛玻璃透明度: " + Math.round(glassOpacity * 100) + "%"; color: OpenUI.onSurfaceVariant; font.pixelSize: OpenUI.typeLabelM }
                Rectangle { width: parent.width; height: 6; radius: 3; color: Qt.rgba(OpenUI.surfaceBright.r,OpenUI.surfaceBright.g,OpenUI.surfaceBright.b,0.3)
                    Rectangle { width: glassOpacity * parent.width; height: 6; radius: 3; color: OpenUI.primary }
                    MouseArea { anchors.fill: parent; hoverEnabled: true
                        onPositionChanged: glassOpacity = Math.max(0.3, Math.min(0.95, mouse.x / width)) }
                }
            }

            // 字体缩放
            Column { width: parent.width; spacing: OpenUI.sp1
                Text { text: "字体缩放: " + Math.round(fontSizeScale * 100) + "%"; color: OpenUI.onSurfaceVariant; font.pixelSize: OpenUI.typeLabelM }
                Rectangle { width: parent.width; height: 6; radius: 3; color: Qt.rgba(OpenUI.surfaceBright.r,OpenUI.surfaceBright.g,OpenUI.surfaceBright.b,0.3)
                    Rectangle { width: (fontSizeScale - 0.5) / 1.0 * parent.width; height: 6; radius: 3; color: OpenUI.primary }
                    MouseArea { anchors.fill: parent; hoverEnabled: true
                        onPositionChanged: fontSizeScale = Math.max(0.5, Math.min(1.5, 0.5 + mouse.x / width * 1.0)) }
                }
            }
        }
    }
}
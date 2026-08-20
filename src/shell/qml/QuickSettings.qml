import QtQuick 2.15

/* OPENOS 快捷设置面板 (点托盘弹出)
 * 音量/亮度滑条 + Wi-Fi/勿扰/夜间模式开关
 */
Rectangle {
    id: quickSettings
    visible: false
    width: 280
    height: 220
    radius: OpenUI.shapeLg
    color: Qt.rgba(OpenUI.surface6.r, OpenUI.surface6.g, OpenUI.surface6.b,
                   OpenUI.glassMenuAlpha)
    border.color: OpenUI.outlineVariant; border.width: 1

    // 打开动画
    opacity: 0; scale: 0.95
    onVisibleChanged: {
        if (visible) { opacity = 0; scale = 0.95; anim.restart() }
    }
    ParallelAnimation {
        id: anim
        NumberAnimation { target: quickSettings; property: "opacity"; to: 1;
            duration: OpenUI.dur200; easing.type: Easing.OutCubic }
        NumberAnimation { target: quickSettings; property: "scale"; to: 1;
            duration: OpenUI.dur200; easing.type: Easing.OutCubic }
    }

    Column {
        anchors.fill: parent
        anchors.margins: OpenUI.sp4
        spacing: OpenUI.sp3

        Text { text: "快捷设置"; color: OpenUI.onSurface;
               font.pixelSize: OpenUI.typeTitle; font.bold: true }

        // 音量
        Row { spacing: OpenUI.sp2; width: parent.width
            ThemedIcon { name: "audio-volume-high"; ctx: "Panel"; size: 16; color: OpenUI.onSurfaceVariant }
            Slider {
                width: parent.width - 40; height: 24
                from: 0; to: 100; value: 70
                onMoved: console.log("volume", value)
            }
        }
        // 亮度
        Row { spacing: OpenUI.sp2; width: parent.width
            ThemedIcon { name: "weather-clear"; ctx: "Panel"; size: 16; color: OpenUI.onSurfaceVariant }
            Slider {
                width: parent.width - 40; height: 24
                from: 0; to: 100; value: 80
                onMoved: console.log("brightness", value)
            }
        }

        // 开关网格
        Grid { columns: 2; spacing: OpenUI.sp2; width: parent.width
            Repeater {
                model: ListModel {
                    ListElement { label: "Wi-Fi";   on: true }
                    ListElement { label: "蓝牙";    on: false }
                    ListElement { label: "勿扰";    on: false }
                    ListElement { label: "夜间模式"; on: false }
                }
                Rectangle {
                    width: (parent.width - OpenUI.sp2) / 2; height: 34
                    radius: OpenUI.shapeXs
                    color: model.on
                           ? Qt.rgba(OpenUI.primary.r, OpenUI.primary.g,
                                     OpenUI.primary.b, 0.2)
                           : Qt.rgba(OpenUI.surfaceBright.r,
                                     OpenUI.surfaceBright.g,
                                     OpenUI.surfaceBright.b, 0.4)
                    Text {
                        anchors.centerIn: parent
                        text: model.label
                        color: model.on ? OpenUI.primary : OpenUI.onSurfaceVariant
                        font.pixelSize: 13
                    }
                    MouseArea {
                        anchors.fill: parent
                        hoverEnabled: true
                        onClicked: model.on = !model.on
                    }
                }
            }
        }
    }

    // 点击外部关闭
    MouseArea { anchors.fill: parent.parent; z: -1; onClicked: quickSettings.visible = false }
}

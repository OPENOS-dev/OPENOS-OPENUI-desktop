import QtQuick 2.15
import QtQuick.Window 2.15

/* OPENOS 主题自定义 (浮窗)
 * 强调色 + 圆角 + 玻璃透明度 (实时预览 OPENUI 令牌)
 * 生产: 通过 C++ 更新 openui.h 令牌 / QML OpenUI 实例属性
 */
Rectangle {
    id: themeWin
    visible: false
    width: 360; height: 300
    radius: OpenUI.shapeLg
    color: Qt.rgba(OpenUI.surface6.r, OpenUI.surface6.g, OpenUI.surface6.b,
                   OpenUI.glassMenuAlpha)
    border.color: OpenUI.outlineVariant; border.width: 1
    anchors.centerIn: parent

    opacity: 0; scale: 0.95
    onVisibleChanged: {
        if (visible) { opacity = 0; scale = 0.95; anim.restart() }
    }
    ParallelAnimation {
        id: anim
        NumberAnimation { target: themeWin; property: "opacity"; to: 1;
            duration: OpenUI.dur200; easing.type: Easing.OutCubic }
        NumberAnimation { target: themeWin; property: "scale"; to: 1;
            duration: OpenUI.dur200; easing.type: Easing.OutCubic }
    }

    Column {
        anchors.fill: parent; anchors.margins: OpenUI.sp5; spacing: OpenUI.sp4
        Row { width: parent.width
            Text { text: "主题自定义"; color: OpenUI.onSurface
                   font.pixelSize: OpenUI.typeTitle; font.bold: true }
            Item { width: parent.width - 90; height: 1 }
            Rectangle { width: 28; height: 28; radius: OpenUI.shapeXs
                color: hover.hovered ? Qt.rgba(OpenUI.error.r, OpenUI.error.g,
                                               OpenUI.error.b, 0.3) : "transparent"
                Text { anchors.centerIn: parent; text: "\u2715"; color: OpenUI.onSurface }
                MouseArea { id: hover; anchors.fill: parent; hoverEnabled: true
                    onClicked: themeWin.visible = false }
            }
        }

        // 强调色
        Text { text: "强调色"; color: OpenUI.onSurfaceVariant; font.pixelSize: OpenUI.typeLabelL }
        Row { spacing: OpenUI.sp2
            Repeater {
                model: ["#00BCD4", "#9FC85F", "#F44336", "#6EB3C0", "#FFB300"]
                Rectangle { width: 30; height: 30; radius: OpenUI.shapeFull
                    color: modelData; border.color: OpenUI.onSurface; border.width: 2
                    MouseArea { anchors.fill: parent; hoverEnabled: true
                        onClicked: console.log("accent:", modelData) }
                }
            }
        }

        // 圆角
        Text { text: "圆角: " + cornerSlider.value.toFixed(0) + "px"
               color: OpenUI.onSurfaceVariant; font.pixelSize: OpenUI.typeLabelL }
        Slider { id: cornerSlider; from: 0; to: 32; value: OpenUI.shapeSm
            width: parent.width; onMoved: console.log("corner", value) }

        // 玻璃透明度
        Text { text: "玻璃透明度: " + (glassSlider.value * 100).toFixed(0) + "%"
               color: OpenUI.onSurfaceVariant; font.pixelSize: OpenUI.typeLabelL }
        Slider { id: glassSlider; from: 0.3; to: 0.95; value: OpenUI.glassPanelAlpha
            width: parent.width; onMoved: console.log("glass", value) }

        // 预览
        Rectangle { width: parent.width; height: 40; radius: cornerSlider.value
            color: Qt.rgba(OpenUI.surfaceBright.r, OpenUI.surfaceBright.g,
                           OpenUI.surfaceBright.b, glassSlider.value)
            Text { anchors.centerIn: parent; text: "实时预览"; color: OpenUI.onSurface }
        }
    }
    MouseArea { anchors.fill: parent.parent; z: -1; onClicked: themeWin.visible = false }
}

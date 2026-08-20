import QtQuick 2.15

/* OPENOS 系统托盘 (面板右侧)
 * 显示: 音量 / Wi-Fi / 电池 / 蓝牙 (SVG 主题图标, NUI 线条风)
 * 点击任一图标弹出快捷设置 QuickSettings
 */
Row {
    id: tray
    spacing: 4

    property bool anyActive: false

    // 托盘图标按钮
    Repeater {
        model: [
            { icon: "audio-volume-high", ctx: "Panel", name: "volume",   active: false },
            { icon: "network-wireless",  ctx: "Panel", name: "wifi",     active: true  },
            { icon: "battery-good",      ctx: "Panel", name: "battery",  active: true  },
            { icon: "bluetooth",          ctx: "Panel", name: "bluetooth", active: false },
        ]
        Rectangle {
            width: 24; height: 24; radius: OpenUI.shapeXs
            color: hover.hovered || quickSettings.visible
                   ? Qt.rgba(OpenUI.onSurface.r, OpenUI.onSurface.g,
                             OpenUI.onSurface.b, OpenUI.hoverAlpha)
                   : "transparent"
            ThemedIcon {
                anchors.centerIn: parent
                name: modelData.icon; ctx: modelData.ctx; size: 14
                color: modelData.active ? OpenUI.primary : OpenUI.onSurfaceVariant
            }
            MouseArea {
                id: hover
                anchors.fill: parent
                hoverEnabled: true
                onClicked: quickSettings.visible = !quickSettings.visible
            }
        }
    }

    // 快捷设置弹出 (由父级 Panel 引用)
    property var quickSettings: null
}

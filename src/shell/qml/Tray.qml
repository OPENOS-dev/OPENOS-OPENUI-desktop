import QtQuick 2.15

/* OPENOS 系统托盘 (面板右侧)
 * 显示: 音量 / Wi-Fi / 电池 / 蓝牙 (文字符号, 禁 Emoji)
 * 点击任一图标弹出快捷设置 QuickSettings
 */
Row {
    id: tray
    spacing: 4

    property bool anyActive: false

    // 托盘图标按钮
    Repeater {
        model: [
            { icon: "\u266A", name: "volume",   active: false },   /* ♪ 音量 */
            { icon: "\u263C", name: "wifi",     active: true  },   /* ☼ Wi-Fi */
            { icon: "\u26A1", name: "battery",  active: true  },   /* ⚡ 电池 */
            { icon: "\u2767", name: "bluetooth", active: false },  /* ❧ 蓝牙 */
        ]
        Rectangle {
            width: 24; height: 24; radius: OpenUI.shapeXs
            color: hover.hovered || quickSettings.visible
                   ? Qt.rgba(OpenUI.onSurface.r, OpenUI.onSurface.g,
                             OpenUI.onSurface.b, OpenUI.hoverAlpha)
                   : "transparent"
            Text {
                anchors.centerIn: parent
                text: modelData.icon
                color: modelData.active ? OpenUI.primary : OpenUI.onSurfaceVariant
                font.pixelSize: 14
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

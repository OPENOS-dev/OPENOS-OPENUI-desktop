import QtQuick 2.15
import QtQuick.Window 2.15

/* OPENOS 电源菜单
 * 关机 / 重启 / 锁屏 / 注销 (文字符号, 禁 Emoji)
 * 实际命令由合成器/系统执行 (systemctl / loginctl)
 */
Rectangle {
    id: powerMenu
    visible: false
    width: 220
    height: 4 * 40 + 24
    radius: OpenUI.shapeLg
    color: Qt.rgba(OpenUI.surface6.r, OpenUI.surface6.g, OpenUI.surface6.b,
                   OpenUI.glassMenuAlpha)
    border.color: OpenUI.outlineVariant; border.width: 1

    opacity: 0; scale: 0.95
    onVisibleChanged: {
        if (visible) { opacity = 0; scale = 0.95; anim.restart() }
    }
    ParallelAnimation {
        id: anim
        NumberAnimation { target: powerMenu; property: "opacity"; to: 1;
            duration: OpenUI.dur200; easing.type: Easing.OutCubic }
        NumberAnimation { target: powerMenu; property: "scale"; to: 1;
            duration: OpenUI.dur200; easing.type: Easing.OutCubic }
    }

    Column {
        anchors.fill: parent
        anchors.margins: OpenUI.sp3
        spacing: 2

        Repeater {
            model: ListModel {
                ListElement { icon: "system-power"; label: "关机"; cmd: "systemctl poweroff" }
                ListElement { icon: "system-reboot"; label: "重启"; cmd: "systemctl reboot" }
                ListElement { icon: "system-lock-screen"; label: "锁屏"; cmd: "openos-lock" }
                ListElement { icon: "system-log-out"; label: "注销"; cmd: "loginctl terminate-session $XDG_SESSION_ID" }
            }
            Rectangle {
                width: parent.width; height: 38; radius: OpenUI.shapeXs
                color: hover.hovered || pressed.pressed
                       ? Qt.rgba(OpenUI.onSurface.r, OpenUI.onSurface.g,
                                 OpenUI.onSurface.b,
                                 pressed.pressed ? OpenUI.pressedAlpha
                                                 : OpenUI.hoverAlpha)
                       : "transparent"
                Row {
                    anchors.fill: parent
                    anchors.leftMargin: OpenUI.sp3
                    spacing: OpenUI.sp2
                    ThemedIcon {
                        width: 22; height: parent.height
                        name: model.icon; ctx: "Actions"; size: 16; color: OpenUI.primary
                        anchors.verticalCenter: parent.verticalCenter
                    }
                    Text {
                        height: parent.height; verticalAlignment: Text.AlignVCenter
                        text: model.label; color: OpenUI.onSurface; font.pixelSize: 13
                    }
                }
                MouseArea {
                    id: hover; anchors.fill: parent; hoverEnabled: true
                }
                MouseArea {
                    id: pressed; anchors.fill: parent
                    onClicked: { console.log("power:", model.cmd); powerMenu.visible = false }
                }
            }
        }
    }

    // 点击外部关闭
    MouseArea { anchors.fill: parent.parent; z: -1; onClicked: powerMenu.visible = false }
}

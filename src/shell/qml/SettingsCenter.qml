import QtQuick 2.15
import QtQuick.Window 2.15

/* OPENOS 设置中心 (OPENUI)
 * 分类: 外观 / 安全 / 软件包 / 系统 / 网络
 * 对接: openos-settingsd (OAK 加密 API) — 生产经 liboak
 */
Window {
    id: settingsWin
    visible: false
    width: 640; height: 480
    flags: Qt.FramelessWindowHint
    title: "设置"
    color: OpenUI.background

    property int currentPage: 0

    Rectangle {
        anchors.fill: parent
        color: OpenUI.background

        // 侧栏导航
        Rectangle {
            width: 160; anchors.top: parent.top; anchors.bottom: parent.bottom
            anchors.left: parent.left
            color: Qt.rgba(OpenUI.surface.r, OpenUI.surface.g, OpenUI.surface.b, 0.9)
            Column {
                anchors.fill: parent; anchors.topMargin: OpenUI.sp4
                Repeater {
                    model: ListModel {
                        ListElement { icon: "preferences-system"; label: "外观" }
                        ListElement { icon: "preferences-desktop-security"; label: "安全" }
                        ListElement { icon: "system-software-install"; label: "软件包" }
                        ListElement { icon: "applications-system"; label: "系统" }
                        ListElement { icon: "preferences-system-network"; label: "网络" }
                    }
                    Rectangle {
                        width: 160; height: 40; radius: OpenUI.shapeXs
                        color: settingsWin.currentPage === index
                               ? Qt.rgba(OpenUI.primary.r, OpenUI.primary.g,
                                         OpenUI.primary.b, 0.2)
                               : (hover.hovered
                                  ? Qt.rgba(OpenUI.onSurface.r, OpenUI.onSurface.g,
                                            OpenUI.onSurface.b, OpenUI.hoverAlpha)
                                  : "transparent")
                        Row {
                            anchors.fill: parent; anchors.leftMargin: OpenUI.sp4
                            spacing: OpenUI.sp2
                            ThemedIcon {
                                width: 24; height: parent.height
                                name: model.icon; ctx: "Apps"; size: 16; color: OpenUI.primary
                                anchors.verticalCenter: parent.verticalCenter
                            }
                            Text { height: parent.height; verticalAlignment: Text.AlignVCenter
                                   text: model.label; color: OpenUI.onSurface
                                   font.pixelSize: OpenUI.typeLabelL }
                        }
                        MouseArea {
                            id: hover; anchors.fill: parent; hoverEnabled: true
                            onClicked: settingsWin.currentPage = index
                        }
                    }
                }
            }
        }

        // 内容区
        Rectangle {
            anchors.left: parent.left; anchors.leftMargin: 160
            anchors.right: parent.right; anchors.top: parent.top; anchors.bottom: parent.bottom
            color: "transparent"
            clip: true
            StackLayout {
                anchors.fill: parent; anchors.margins: OpenUI.sp5
                currentIndex: settingsWin.currentPage
                AppearancePage {}
                SecurityPage {}
                PackagesPage {}
                SystemPage {}
                NetworkPage {}
            }
        }

        // 关闭按钮 (右上)
        Rectangle {
            x: parent.width - 40; y: 8; width: 32; height: 32; radius: OpenUI.shapeXs
            color: hover.hovered ? Qt.rgba(OpenUI.error.r, OpenUI.error.g,
                                           OpenUI.error.b, 0.3) : "transparent"
            Text { anchors.centerIn: parent; text: "\u2715"; color: OpenUI.onSurface }
            MouseArea { id: hover; anchors.fill: parent; hoverEnabled: true
                onClicked: settingsWin.visible = false }
        }
    }
}

import QtQuick 2.15
import QtQuick.Window 2.15

/* OPENOS 包管理器 UI (opt 图形前端)
 * 后端列表 + 安装/卸载/更新
 * 生产: 经 opt 命令 / liboak 与 openos-settingsd 通信
 */
Window {
    id: optWin
    visible: false
    width: 560; height: 420
    flags: Qt.FramelessWindowHint
    title: "软件包管理"
    color: OpenUI.background

    Column {
        anchors.fill: parent; anchors.margins: OpenUI.sp4; spacing: OpenUI.sp3

        Row { width: parent.width; spacing: OpenUI.sp2
            Text { text: "软件包管理"; color: OpenUI.onSurface
                   font.pixelSize: OpenUI.typeHeadlineM; font.bold: true }
            Item { width: parent.width - 120; height: 1 }
            Rectangle {
                width: 32; height: 32; radius: OpenUI.shapeXs
                color: hover.hovered ? Qt.rgba(OpenUI.error.r, OpenUI.error.g,
                                               OpenUI.error.b, 0.3) : "transparent"
                Text { anchors.centerIn: parent; text: "\u2715"; color: OpenUI.onSurface }
                MouseArea { id: hover; anchors.fill: parent; hoverEnabled: true
                    onClicked: optWin.visible = false }
            }
        }

        // 后端卡片
        Rectangle {
            width: parent.width; height: 64; radius: OpenUI.shapeSm
            color: Qt.rgba(OpenUI.surfaceBright.r, OpenUI.surfaceBright.g,
                           OpenUI.surfaceBright.b, 0.4)
            Row { anchors.fill: parent; anchors.margins: OpenUI.sp3; spacing: OpenUI.sp2
                Text { width: 30; height: parent.height; verticalAlignment: Text.AlignVCenter
                       text: "\u2630"; color: OpenUI.primary; font.pixelSize: 18 }
                Column { width: parent.width - 130; height: parent.height; spacing: 2
                    verticalAlignment: Text.AlignVCenter
                    Text { text: "apt"; color: OpenUI.onSurface; font.pixelSize: OpenUI.typeTitle; font.bold: true }
                    Text { text: "已安装 (内置)"; color: OpenUI.primary; font.pixelSize: OpenUI.typeLabelM }
                }
                Rectangle { width: 90; height: 30; radius: OpenUI.shapeXs; anchors.verticalCenter: parent.verticalCenter
                    color: Qt.rgba(OpenUI.primary.r, OpenUI.primary.g, OpenUI.primary.b, 0.2)
                    Text { anchors.centerIn: parent; text: "检查更新"; color: OpenUI.primary
                           font.pixelSize: OpenUI.typeLabelM }
                    MouseArea { anchors.fill: parent; hoverEnabled: true
                        onClicked: console.log("opt: update apt") }
                }
            }
        }

        // 动作栏
        Row { spacing: OpenUI.sp2
            Repeater {
                model: ListModel {
                    ListElement { label: "安装软件"; icon: "\u2715" }
                    ListElement { label: "卸载";     icon: "\u2212" }
                    ListElement { label: "搜索";     icon: "\u2315" }
                }
                Rectangle { width: 110; height: 36; radius: OpenUI.shapeXs
                    color: Qt.rgba(OpenUI.primary.r, OpenUI.primary.g, OpenUI.primary.b, 0.2)
                    Row { anchors.centerIn: parent; spacing: 4
                        Text { text: model.icon; color: OpenUI.primary }
                        Text { text: model.label; color: OpenUI.primary; font.pixelSize: OpenUI.typeLabelM }
                    }
                    MouseArea { anchors.fill: parent; hoverEnabled: true; onClicked: {} }
                }
            }
        }

        // 已装软件列表 (占位)
        Rectangle {
            width: parent.width; height: parent.height - 150; radius: OpenUI.shapeSm
            color: Qt.rgba(OpenUI.surfaceBright.r, OpenUI.surfaceBright.g,
                           OpenUI.surfaceBright.b, 0.3)
            Text { anchors.centerIn: parent; text: "软件列表\n(生产: 经 opt 查询已装包)"
                   color: OpenUI.onSurfaceDisabled; font.pixelSize: OpenUI.typeLabelM
                   horizontalAlignment: Text.AlignHCenter }
        }
    }
}

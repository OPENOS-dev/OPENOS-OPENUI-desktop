import QtQuick 2.15
import QtQuick.Window 2.15

/* OPENOS 软件隔离管理 (vmapp)
 * 列出 /vmapp 下的虚拟化环境, 查看/删除
 * 生产: 经 libvmapp 枚举 /vmapp 子目录
 */
Window {
    id: vmappWin
    visible: false
    width: 480; height: 380
    flags: Qt.FramelessWindowHint
    title: "软件隔离管理"
    color: OpenUI.background

    Column {
        anchors.fill: parent; anchors.margins: OpenUI.sp4; spacing: OpenUI.sp3
        Row { width: parent.width; spacing: OpenUI.sp2
            Text { text: "软件隔离 (vmapp)"; color: OpenUI.onSurface
                   font.pixelSize: OpenUI.typeHeadlineM; font.bold: true }
            Item { width: parent.width - 160; height: 1 }
            Rectangle { width: 32; height: 32; radius: OpenUI.shapeXs
                color: hover.hovered ? Qt.rgba(OpenUI.error.r, OpenUI.error.g,
                                               OpenUI.error.b, 0.3) : "transparent"
                Text { anchors.centerIn: parent; text: "\u2715"; color: OpenUI.onSurface }
                MouseArea { id: hover; anchors.fill: parent; hoverEnabled: true
                    onClicked: vmappWin.visible = false }
            }
        }
        Text { text: "每个软件有独立文件系统视图 (/vmapp/<name>)"
               color: OpenUI.onSurfaceVariant; font.pixelSize: OpenUI.typeBodyM }

        // 环境列表
        Rectangle {
            width: parent.width; height: parent.height - 90; radius: OpenUI.shapeSm
            color: Qt.rgba(OpenUI.surfaceBright.r, OpenUI.surfaceBright.g,
                           OpenUI.surfaceBright.b, 0.3)
            ListView {
                anchors.fill: parent; anchors.margins: OpenUI.sp2
                clip: true
                model: ListModel {
                    ListElement { name: "opt";      size: "128 MB" }
                    ListElement { name: "firefox";  size: "450 MB" }
                    ListElement { name: "code";     size: "320 MB" }
                }
                delegate: Rectangle {
                    width: parent.width; height: 44; radius: OpenUI.shapeXs
                    color: hover.hovered
                           ? Qt.rgba(OpenUI.onSurface.r, OpenUI.onSurface.g,
                                     OpenUI.onSurface.b, OpenUI.hoverAlpha)
                           : "transparent"
                    Row { anchors.fill: parent; anchors.leftMargin: OpenUI.sp3
                        spacing: OpenUI.sp2
                        Text { width: 30; height: parent.height; verticalAlignment: Text.AlignVCenter
                               text: "\u25A2"; color: OpenUI.primary; font.pixelSize: 16 }
                        Text { width: 140; height: parent.height; verticalAlignment: Text.AlignVCenter
                               text: model.name; color: OpenUI.onSurface; font.pixelSize: OpenUI.typeBodyM }
                        Text { width: 80; height: parent.height; verticalAlignment: Text.AlignVCenter
                               text: model.size; color: OpenUI.onSurfaceVariant; font.pixelSize: OpenUI.typeLabelM }
                        Rectangle { width: 60; height: 26; radius: OpenUI.shapeXs
                            anchors.verticalCenter: parent.verticalCenter
                            color: Qt.rgba(OpenUI.error.r, OpenUI.error.g,
                                           OpenUI.error.b, 0.2)
                            Text { anchors.centerIn: parent; text: "移除"; color: OpenUI.error
                                   font.pixelSize: OpenUI.typeLabelS }
                            MouseArea { anchors.fill: parent; hoverEnabled: true
                                onClicked: console.log("vmapp: remove", model.name) }
                        }
                    }
                    MouseArea { id: hover; anchors.fill: parent; hoverEnabled: true }
                }
            }
        }
    }
}

import QtQuick 2.15
import QtQuick.Window 2.15

/* OPENOS 文本编辑器 (OPENUI 浮窗)
 * 简单文本编辑, 支持多行输入/搜索/替换
 */
Window {
    id: editorWin
    visible: false
    width: 500; height: 400
    flags: Qt.FramelessWindowHint
    title: "文本编辑器"
    color: "transparent"

    property string fileName: "未命名.txt"
    property string searchText: ""

    Rectangle {
        anchors.fill: parent; anchors.margins: 1
        radius: OpenUI.shapeLg
        color: Qt.rgba(OpenUI.surface6.r, OpenUI.surface6.g, OpenUI.surface6.b, OpenUI.glassMenuAlpha)
        border.color: OpenUI.outlineVariant; border.width: 1
        clip: true

        Column {
            anchors.fill: parent; anchors.margins: OpenUI.sp3; spacing: OpenUI.sp2

            // 标题栏
            Row { width: parent.width; spacing: OpenUI.sp2
                Text { text: "\u270E " + fileName; color: OpenUI.onSurface
                       font.pixelSize: OpenUI.typeTitle; font.bold: true
                       verticalAlignment: Text.AlignVCenter }
                Item { width: parent.width - 190; height: 1 }
                Rectangle {
                    width: 24; height: 24; radius: OpenUI.shapeXs
                    color: searchHover.hovered ? Qt.rgba(OpenUI.onSurface.r,OpenUI.onSurface.g,OpenUI.onSurface.b,OpenUI.hoverAlpha) : "transparent"
                    Text { anchors.centerIn: parent; text: "\u2315"; color: OpenUI.onSurfaceVariant; font.pixelSize: 14 }
                    MouseArea { id: searchHover; anchors.fill: parent; hoverEnabled: true
                        onClicked: searchBar.visible = !searchBar.visible }
                }
            }

            // 搜索栏
            Rectangle {
                id: searchBar; visible: false
                width: parent.width; height: 28; radius: OpenUI.shapeXs
                color: Qt.rgba(OpenUI.surfaceBright.r, OpenUI.surfaceBright.g, OpenUI.surfaceBright.b, 0.3)
                TextInput {
                    anchors.fill: parent; anchors.margins: 6
                    color: OpenUI.onSurface; font.pixelSize: 12
                    placeholderText: "搜索..."
                    onTextChanged: editorWin.searchText = text
                }
            }

            // 编辑区
            Rectangle {
                width: parent.width; height: parent.height - 80
                radius: OpenUI.shapeXs
                color: Qt.rgba(OpenUI.surfaceBright.r, OpenUI.surfaceBright.g, OpenUI.surfaceBright.b, 0.2)
                Flickable {
                    anchors.fill: parent; anchors.margins: 2
                    contentWidth: editor.width; contentHeight: editor.height
                    clip: true
                    TextEdit {
                        id: editor
                        width: Math.max(parent.width, 400)
                        height: Math.max(parent.height, 400)
                        color: OpenUI.onSurface
                        font.pixelSize: 13
                        font.family: "monospace"
                        wrapMode: Text.WrapAtWordBoundaryOrAnywhere
                        selectByMouse: true
                        text: ""
                    }
                }
            }
        }
    }
}
import QtQuick 2.15

/* OPENOS 全局文件搜索 (键盘优先)
 * 搜索文件和目录, 支持键盘导航与打开 (Ctrl+F 唤起)
 */
Rectangle {
    id: fileSearch
    visible: false
    anchors.fill: parent
    color: Qt.rgba(0, 0, 0, 0.6)

    // ---- data model ----
    property var results: ListModel {}

    // ---- animation ----
    opacity: 0
    onVisibleChanged: {
        if (visible) {
            opacity = 0; panel.scale = 0.95
            anim.restart()
            input.forceActiveFocus()
        }
    }
    ParallelAnimation {
        id: anim
        NumberAnimation { target: fileSearch; property: "opacity"; to: 1;
            duration: OpenUI.dur150; easing.type: Easing.OutCubic }
        NumberAnimation { target: panel; property: "scale"; to: 1;
            duration: OpenUI.dur200; easing.type: Easing.OutCubic }
    }

    focus: true

    // ---- keyboard navigation ----
    Keys.onEscapePressed: visible = false
    Keys.onUpPressed: list.currentIndex = Math.max(0, list.currentIndex - 1)
    Keys.onDownPressed: list.currentIndex = Math.min(list.count - 1, list.currentIndex + 1)
    Keys.onReturnPressed: openFile()

    // ---- search panel ----
    Rectangle {
        id: panel
        width: 560; height: 400
        radius: OpenUI.shapeLg
        color: Qt.rgba(OpenUI.surface6.r, OpenUI.surface6.g, OpenUI.surface6.b,
                       OpenUI.glassPanelAlpha)
        border.color: OpenUI.outlineVariant; border.width: 1
        anchors.centerIn: parent

        Column {
            anchors.fill: parent
            anchors.margins: OpenUI.sp3
            spacing: OpenUI.sp2

            // ---- search input row ----
            Rectangle {
                id: inputBg
                width: parent.width; height: 40
                radius: OpenUI.shapeXs
                color: Qt.rgba(OpenUI.surfaceBright.r, OpenUI.surfaceBright.g,
                               OpenUI.surfaceBright.b, 0.5)

                Text {
                    id: searchIcon
                    text: "\u2315"
                    color: OpenUI.onSurfaceVariant
                    font.pixelSize: OpenUI.typeBodyM
                    anchors.left: parent.left
                    anchors.leftMargin: OpenUI.sp3
                    anchors.verticalCenter: parent.verticalCenter
                }

                TextField {
                    id: input
                    anchors.left: searchIcon.right
                    anchors.leftMargin: OpenUI.sp2
                    anchors.right: clearBtn.left
                    anchors.rightMargin: OpenUI.sp2
                    anchors.verticalCenter: parent.verticalCenter
                    height: parent.height
                    placeholderText: "搜索文件和目录…"
                    placeholderTextColor: OpenUI.onSurfaceVariant
                    color: OpenUI.onSurface
                    font.pixelSize: OpenUI.typeBodyM
                    background: Item {}
                    verticalAlignment: TextInput.AlignVCenter
                    Keys.onEscapePressed: fileSearch.visible = false
                    onTextChanged: filter()
                    // forward navigation keys to parent
                    Keys.onPressed: function (event) {
                        if (event.key === Qt.Key_Up || event.key === Qt.Key_Down
                                || event.key === Qt.Key_Return) {
                            event.accepted = false
                        }
                    }
                }

                Text {
                    id: clearBtn
                    text: "\u2715"
                    color: OpenUI.onSurfaceVariant
                    font.pixelSize: OpenUI.typeLabelL
                    anchors.right: parent.right
                    anchors.rightMargin: OpenUI.sp3
                    anchors.verticalCenter: parent.verticalCenter
                    visible: input.text.length > 0

                    MouseArea {
                        anchors.fill: parent
                        cursorShape: Qt.PointingHandCursor
                        onClicked: { input.text = ""; input.forceActiveFocus() }
                    }
                }
            }

            // ---- results list ----
            ListView {
                id: list
                width: parent.width
                height: parent.height - inputBg.height - tips.height
                         - OpenUI.sp2 * 2
                clip: true
                model: results
                currentIndex: -1

                delegate: Rectangle {
                    width: list.width; height: 36
                    radius: OpenUI.shapeXs
                    color: {
                        if (list.currentIndex === index)
                            return Qt.rgba(OpenUI.primary.r, OpenUI.primary.g,
                                           OpenUI.primary.b, OpenUI.hoverAlpha)
                        if (hoverArea.containsMouse)
                            return Qt.rgba(OpenUI.onSurface.r, OpenUI.onSurface.g,
                                           OpenUI.onSurface.b, OpenUI.hoverAlpha * 0.5)
                        return "transparent"
                    }

                    readonly property bool isDir: model.type === "directory"

                    Row {
                        anchors.fill: parent
                        anchors.leftMargin: OpenUI.sp3
                        spacing: OpenUI.sp2

                        // file type icon
                        Text {
                            width: 24; height: parent.height
                            verticalAlignment: Text.AlignVCenter
                            horizontalAlignment: Text.AlignHCenter
                            text: isDir ? "\u25A4" : "\u25A8"
                            color: OpenUI.primary
                            font.pixelSize: OpenUI.typeLabelL
                        }

                        // name + path
                        Column {
                            width: parent.width - 24 - typeTag.width
                                   - OpenUI.sp2 * 3
                            height: parent.height
                            verticalAlignment: Text.AlignVCenter

                            Text {
                                width: parent.width
                                elide: Text.ElideRight
                                text: model.name
                                color: OpenUI.onSurface
                                font.pixelSize: OpenUI.typeBodyM
                            }
                            Text {
                                width: parent.width
                                elide: Text.ElideRight
                                text: model.path
                                color: OpenUI.onSurfaceVariant
                                font.pixelSize: OpenUI.typeLabelS
                            }
                        }

                        // type label
                        Text {
                            id: typeTag
                            height: parent.height
                            verticalAlignment: Text.AlignVCenter
                            text: isDir ? "目录" : "文件"
                            color: OpenUI.onSurfaceDisabled
                            font.pixelSize: OpenUI.typeLabelS
                        }
                    }

                    MouseArea {
                        id: hoverArea
                        anchors.fill: parent
                        hoverEnabled: true
                        cursorShape: Qt.PointingHandCursor
                        onClicked: {
                            list.currentIndex = index
                            openFile()
                        }
                    }
                }
            }

            // ---- bottom tips ----
            Text {
                id: tips
                width: parent.width
                text: "按 ↑↓ 导航 · 按 Enter 打开 · 按 Esc 关闭"
                color: OpenUI.onSurfaceVariant
                font.pixelSize: OpenUI.typeLabelS
                horizontalAlignment: Text.AlignHCenter
            }
        }
    }

    // ---- functions ----
    function filter() {
        // placeholder — shell integration provides real filtering
    }

    function openFile() {
        var it = results.get(list.currentIndex)
        if (it) {
            console.log("fileSearch: open", it.path)
        }
        fileSearch.visible = false
    }
}
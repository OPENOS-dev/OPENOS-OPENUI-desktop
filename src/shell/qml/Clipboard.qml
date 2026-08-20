import QtQuick 2.15
import QtQuick.Window 2.15

/* OPENOS 剪贴板历史管理器 (浮窗)
 * 记录复制历史, 支持搜索/固定/清空/单条删除
 */
Window {
    id: clipWin
    visible: false
    width: 400; height: 420
    flags: Qt.FramelessWindowHint
    title: "剪贴板历史"
    color: "transparent"

    // 内部数据模型 (生产对接 libclipboard)
    ListModel { id: clipModel }

    property int _hoveredIndex: -1

    // 背景卡片
    Rectangle {
        anchors.fill: parent
        anchors.margins: 1
        radius: OpenUI.shapeLg
        color: Qt.rgba(OpenUI.surface6.r, OpenUI.surface6.g, OpenUI.surface6.b,
                       OpenUI.glassMenuAlpha)
        border.color: OpenUI.outlineVariant; border.width: 1

        Column {
            anchors.fill: parent
            anchors.margins: OpenUI.sp3
            spacing: OpenUI.sp2

            // ---- 标题栏 ----
            Row {
                width: parent.width; height: 28; spacing: OpenUI.sp2

                Text {
                    text: "\u2630"; color: OpenUI.primary
                    font.pixelSize: OpenUI.typeTitle
                    verticalAlignment: Text.AlignVCenter; height: parent.height
                }
                Text {
                    text: "剪贴板历史"; color: OpenUI.onSurface
                    font.pixelSize: OpenUI.typeTitle; font.bold: true
                    verticalAlignment: Text.AlignVCenter; height: parent.height
                }
                Item { width: parent.width - 100; height: 1 }

                Rectangle {
                    width: 28; height: 28; radius: OpenUI.shapeXs
                    color: closeHover.hovered
                           ? Qt.rgba(OpenUI.error.r, OpenUI.error.g,
                                     OpenUI.error.b, 0.3) : "transparent"
                    Text {
                        anchors.centerIn: parent
                        text: "\u2715"; color: OpenUI.onSurface
                        font.pixelSize: 14
                    }
                    MouseArea {
                        id: closeHover
                        anchors.fill: parent; hoverEnabled: true
                        onClicked: clipWin.visible = false
                    }
                }
            }

            // ---- 搜索栏 ----
            Rectangle {
                width: parent.width; height: 32; radius: OpenUI.shapeXs
                color: Qt.rgba(OpenUI.surfaceBright.r, OpenUI.surfaceBright.g,
                               OpenUI.surfaceBright.b, 0.5)
                TextInput {
                    id: searchInput
                    anchors.fill: parent; anchors.leftMargin: OpenUI.sp2
                    verticalAlignment: Text.AlignVCenter
                    color: OpenUI.onSurface
                    font.pixelSize: OpenUI.typeBodyM
                    clip: true
                    onTextChanged: filter()
                }
                Text {
                    anchors.left: parent.left; anchors.leftMargin: OpenUI.sp2
                    anchors.verticalCenter: parent.verticalCenter
                    text: "搜索剪贴板…"; color: OpenUI.onSurfaceVariant
                    font.pixelSize: OpenUI.typeBodyM
                    visible: searchInput.text.length === 0
                }
            }

            // ---- 列表 ----
            Rectangle {
                width: parent.width; height: parent.height - 120; radius: OpenUI.shapeSm
                color: Qt.rgba(OpenUI.surface.r, OpenUI.surface.g,
                               OpenUI.surface.b, 0.4)
                clip: true

                // 空状态
                Column {
                    anchors.centerIn: parent
                    spacing: OpenUI.sp2
                    visible: listView.count === 0
                    Text {
                        anchors.horizontalCenter: parent.horizontalCenter
                        text: "\u25CB"; color: OpenUI.onSurfaceVariant
                        font.pixelSize: 32
                    }
                    Text {
                        text: "暂无剪贴板记录"; color: OpenUI.onSurfaceVariant
                        font.pixelSize: OpenUI.typeBodyM
                    }
                }

                ListView {
                    id: listView
                    anchors.fill: parent
                    anchors.margins: OpenUI.sp1
                    spacing: 2
                    clip: true
                    model: filteredModel
                    delegate: Rectangle {
                        id: itemBg
                        width: listView.width; height: 40; radius: OpenUI.shapeXs
                        color: itemMouse.hovered
                               ? Qt.rgba(OpenUI.onSurface.r, OpenUI.onSurface.g,
                                         OpenUI.onSurface.b, OpenUI.hoverAlpha)
                               : "transparent"

                        // primary 左侧竖条 (hover 时显示)
                        Rectangle {
                            id: leftBar
                            x: 2; y: 4
                            width: itemMouse.hovered ? 3 : 0
                            height: parent.height - 8
                            radius: 2
                            color: OpenUI.primary
                            Behavior on width { NumberAnimation { duration: OpenUI.dur100 } }
                        }

                        // 文本预览 (首行, 单行)
                        Text {
                            id: previewText
                            anchors {
                                left: parent.left; leftMargin: OpenUI.sp3
                                right: pinBtn.left; rightMargin: OpenUI.sp2
                                verticalCenter: parent.verticalCenter
                            }
                            text: model.text
                            color: OpenUI.onSurface
                            font.pixelSize: OpenUI.typeLabelL
                            elide: Text.ElideRight
                            maximumLineCount: 1
                            clip: true
                        }

                        // 固定按钮 (★)
                        Rectangle {
                            id: pinBtn
                            anchors {
                                right: deleteBtn.left; rightMargin: 2
                                verticalCenter: parent.verticalCenter
                            }
                            width: 24; height: 24; radius: OpenUI.shapeXs
                            color: pinHover.hovered
                                   ? Qt.rgba(OpenUI.onSurface.r, OpenUI.onSurface.g,
                                             OpenUI.onSurface.b, 0.1)
                                   : "transparent"
                            Text {
                                anchors.centerIn: parent
                                text: model.pinned ? "\u2605" : "\u2606"
                                color: model.pinned ? OpenUI.primary : OpenUI.onSurfaceVariant
                                font.pixelSize: 12
                            }
                            MouseArea {
                                id: pinHover
                                anchors.fill: parent; hoverEnabled: true
                                onClicked: {
                                    clipModel.setProperty(index, "pinned", !model.pinned)
                                    // 重新排序: 固定项置顶
                                    reorder()
                                }
                            }
                        }

                        // 删除按钮 (✕)
                        Rectangle {
                            id: deleteBtn
                            anchors {
                                right: parent.right; rightMargin: 2
                                verticalCenter: parent.verticalCenter
                            }
                            width: 24; height: 24; radius: OpenUI.shapeXs
                            color: delHover.hovered
                                   ? Qt.rgba(OpenUI.error.r, OpenUI.error.g,
                                             OpenUI.error.b, 0.3)
                                   : "transparent"
                            Text {
                                anchors.centerIn: parent
                                text: "\u2715"; color: OpenUI.onSurfaceVariant
                                font.pixelSize: 10
                            }
                            MouseArea {
                                id: delHover
                                anchors.fill: parent; hoverEnabled: true
                                onClicked: {
                                    clipModel.remove(index)
                                    filter()
                                }
                            }
                        }

                        // 点击复制
                        MouseArea {
                            id: itemMouse
                            anchors.fill: parent
                            hoverEnabled: true
                            onEntered: { _hoveredIndex = index }
                            onExited: { _hoveredIndex = -1 }
                            onClicked: {
                                // 复制到系统剪贴板
                                // 生产: 对接 C++ 剪贴板 API
                                clipAnim.restart()
                                previewText.color = OpenUI.primary
                                copyFeedback.visible = true
                                copyTimer.restart()
                            }
                        }

                        // 复制反馈
                        Rectangle {
                            id: copyFeedback
                            anchors.centerIn: parent
                            width: 60; height: 22; radius: OpenUI.shapeFull
                            color: Qt.rgba(OpenUI.primary.r, OpenUI.primary.g,
                                           OpenUI.primary.b, 0.9)
                            visible: false
                            Text {
                                anchors.centerIn: parent
                                text: "已复制"; color: OpenUI.onPrimary
                                font.pixelSize: OpenUI.typeLabelS
                            }
                        }
                        Timer {
                            id: copyTimer
                            interval: OpenUI.dur150 * 2
                            onTriggered: {
                                copyFeedback.visible = false
                                previewText.color = OpenUI.onSurface
                            }
                        }

                        // 点击波纹动画
                        SequentialAnimation {
                            id: clipAnim
                            NumberAnimation {
                                target: itemBg; property: "scale"; to: 0.97
                                duration: OpenUI.dur100; easing.type: Easing.OutCubic
                            }
                            NumberAnimation {
                                target: itemBg; property: "scale"; to: 1.0
                                duration: OpenUI.dur100; easing.type: Easing.OutCubic
                            }
                        }
                    }
                }
            }

            // ---- 底部操作栏 ----
            Row {
                width: parent.width; height: 28; spacing: OpenUI.sp2
                Item { width: parent.width - 100; height: 1 }
                Rectangle {
                    width: 80; height: 28; radius: OpenUI.shapeFull
                    color: clearHover.hovered
                           ? Qt.rgba(OpenUI.error.r, OpenUI.error.g,
                                     OpenUI.error.b, 0.2)
                           : Qt.rgba(OpenUI.error.r, OpenUI.error.g,
                                     OpenUI.error.b, 0.1)
                    Text {
                        anchors.centerIn: parent
                        text: "清空历史"; color: OpenUI.error
                        font.pixelSize: OpenUI.typeLabelM
                    }
                    MouseArea {
                        id: clearHover
                        anchors.fill: parent; hoverEnabled: true
                        onClicked: {
                            clipModel.clear()
                            filter()
                            previewText.color = OpenUI.onSurface
                        }
                    }
                }
            }
        }
    }

    // ---- 过滤模型 ----
    ListModel { id: filteredModel }

    function filter() {
        filteredModel.clear()
        var q = searchInput.text.toLowerCase()
        // 先加固定项
        for (var i = 0; i < clipModel.count; i++) {
            var it = clipModel.get(i)
            if (it.pinned && (q.length === 0 ||
                it.text.toLowerCase().indexOf(q) >= 0))
                filteredModel.append(it)
        }
        // 再加非固定项
        for (i = 0; i < clipModel.count; i++) {
            it = clipModel.get(i)
            if (!it.pinned && (q.length === 0 ||
                it.text.toLowerCase().indexOf(q) >= 0))
                filteredModel.append(it)
        }
    }

    function reorder() {
        filter()
    }

    // ---- 公开接口 ----
    function appendEntry(text) {
        clipModel.append({ text: text, pinned: false })
        filter()
    }

    Component.onCompleted: {
        // 示例数据 (生产对接系统剪贴板监听)
        clipModel.append({ text: "剪贴板内容示例 — 第一行文本", pinned: false })
        clipModel.append({ text: "openos-dev@openos:~$ sudo apt update", pinned: true })
        clipModel.append({ text: "https://github.com/openos-dev/openos-core", pinned: false })
        clipModel.append({ text: "OPENOS 设计体系: NUI2 + OPENUI", pinned: false })
        clipModel.append({ text: "Qt Quick / QML 组件开发指南", pinned: false })
        filter()
    }
}
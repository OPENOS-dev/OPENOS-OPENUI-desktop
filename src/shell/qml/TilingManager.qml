import QtQuick 2.15

/* OPENOS 窗口布局面板 (点托盘弹出)
 * 提供6种窗口布局预设，点击即可应用
 */
Rectangle {
    id: tilingPanel
    visible: false
    width: 320
    height: 300
    radius: OpenUI.shapeLg
    color: Qt.rgba(OpenUI.surface6.r, OpenUI.surface6.g, OpenUI.surface6.b,
                   OpenUI.glassMenuAlpha)
    border.color: OpenUI.outlineVariant; border.width: 1

    property int currentPreset: -1

    // 打开动画
    opacity: 0; scale: 0.95
    onVisibleChanged: {
        if (visible) { opacity = 0; scale = 0.95; anim.restart() }
    }
    ParallelAnimation {
        id: anim
        NumberAnimation { target: tilingPanel; property: "opacity"; to: 1;
            duration: OpenUI.dur200; easing.type: Easing.OutCubic }
        NumberAnimation { target: tilingPanel; property: "scale"; to: 1;
            duration: OpenUI.dur200; easing.type: Easing.OutCubic }
    }

    Column {
        anchors.fill: parent
        anchors.margins: OpenUI.sp4
        spacing: OpenUI.sp3

        // 标题行
        Row {
            width: parent.width
            height: 26
            Text {
                text: "窗口布局"
                color: OpenUI.onSurface
                font.pixelSize: OpenUI.typeTitle
                font.bold: true
                anchors.verticalCenter: parent.verticalCenter
            }
            Item { width: parent.width - 80; height: 1 }
            Rectangle {
                width: 24; height: 24; radius: OpenUI.shapeXs
                color: closeHover.hovered ? Qt.rgba(OpenUI.error.r, OpenUI.error.g,
                                                     OpenUI.error.b, 0.3) : "transparent"
                Text {
                    anchors.centerIn: parent
                    text: "\u2715"
                    color: OpenUI.onSurface
                    font.pixelSize: OpenUI.typeLabelL
                }
                MouseArea {
                    id: closeHover
                    anchors.fill: parent
                    hoverEnabled: true
                    onClicked: tilingPanel.visible = false
                }
            }
        }

        // 布局预设网格
        Grid {
            columns: 3
            spacing: OpenUI.sp2
            width: parent.width

            Repeater {
                model: ListModel {
                    ListElement { name: "单窗口";   type: "single" }
                    ListElement { name: "左右分屏"; type: "leftRight" }
                    ListElement { name: "左宽右窄"; type: "leftWide" }
                    ListElement { name: "上二下一"; type: "topTwo" }
                    ListElement { name: "左二右二"; type: "fourQuad" }
                    ListElement { name: "三列";     type: "threeCol" }
                }

                Item {
                    width: (parent.width - parent.spacing * 2) / 3
                    height: 96

                    // 预览区域容器
                    Rectangle {
                        id: previewBox
                        anchors.top: parent.top
                        anchors.horizontalCenter: parent.horizontalCenter
                        width: 80
                        height: 56
                        radius: OpenUI.shapeXs
                        color: Qt.rgba(OpenUI.surfaceBright.r, OpenUI.surfaceBright.g,
                                       OpenUI.surfaceBright.b, 0.4)
                        border.color: tilingPanel.currentPreset === index
                                      ? OpenUI.primary : "transparent"
                        border.width: tilingPanel.currentPreset === index ? 2 : 0
                        clip: true

                        // 内部布局预览小方块
                        Item {
                            anchors.fill: parent
                            anchors.margins: 4

                            // 根据布局类型绘制不同分块
                            Loader {
                                anchors.fill: parent
                                sourceComponent: {
                                    switch (model.type) {
                                    case "single":    return singlePreview
                                    case "leftRight": return leftRightPreview
                                    case "leftWide":  return leftWidePreview
                                    case "topTwo":    return topTwoPreview
                                    case "fourQuad":  return fourQuadPreview
                                    case "threeCol":  return threeColPreview
                                    default:          return singlePreview
                                    }
                                }
                            }
                        }

                        // 鼠标悬停高亮
                        Rectangle {
                            anchors.fill: parent
                            radius: OpenUI.shapeXs
                            color: Qt.rgba(OpenUI.primary.r, OpenUI.primary.g,
                                           OpenUI.primary.b, presetHover.hovered ? OpenUI.hoverAlpha : 0)
                        }
                    }

                    // 预设名称
                    Text {
                        anchors.top: previewBox.bottom
                        anchors.topMargin: OpenUI.sp1
                        anchors.horizontalCenter: parent.horizontalCenter
                        text: model.name
                        color: tilingPanel.currentPreset === index
                               ? OpenUI.primary : OpenUI.onSurfaceVariant
                        font.pixelSize: OpenUI.typeLabelS
                    }

                    // 点击选择
                    MouseArea {
                        id: presetHover
                        anchors.fill: parent
                        hoverEnabled: true
                        onClicked: tilingPanel.currentPreset = index
                    }
                }
            }
        }

        // 底部操作按钮
        Rectangle {
            width: parent.width
            height: 34
            radius: OpenUI.shapeFull
            color: applyToggle.toggled
                   ? Qt.rgba(OpenUI.primary.r, OpenUI.primary.g,
                             OpenUI.primary.b, 0.2)
                   : Qt.rgba(OpenUI.surfaceBright.r, OpenUI.surfaceBright.g,
                             OpenUI.surfaceBright.b, 0.4)
            border.color: applyToggle.toggled ? OpenUI.primary : "transparent"
            border.width: 1

            property bool toggled: false

            Text {
                anchors.centerIn: parent
                text: applyToggle.toggled ? "已应用到当前工作区" : "应用到当前工作区"
                color: applyToggle.toggled ? OpenUI.primary : OpenUI.onSurfaceVariant
                font.pixelSize: OpenUI.typeLabelM
            }

            MouseArea {
                id: applyToggle
                anchors.fill: parent
                hoverEnabled: true
                onClicked: parent.toggled = !parent.toggled
            }
        }
    }

    // 点击外部关闭
    MouseArea {
        anchors.fill: parent.parent
        z: -1
        onClicked: tilingPanel.visible = false
    }

    // ---- 布局预览组件 ----

    Component {
        id: singlePreview
        Rectangle {
            anchors.fill: parent
            radius: 2
            color: OpenUI.primary
        }
    }

    Component {
        id: leftRightPreview
        Row {
            spacing: 2
            Rectangle { width: parent.width / 2 - 1; height: parent.height; radius: 2; color: OpenUI.primary }
            Rectangle { width: parent.width / 2 - 1; height: parent.height; radius: 2; color: OpenUI.primary }
        }
    }

    Component {
        id: leftWidePreview
        Row {
            spacing: 2
            Rectangle { width: parent.width * 0.6 - 1; height: parent.height; radius: 2; color: OpenUI.primary }
            Rectangle { width: parent.width * 0.4 - 1; height: parent.height; radius: 2; color: OpenUI.primary }
        }
    }

    Component {
        id: topTwoPreview
        Column {
            spacing: 2
            Row {
                spacing: 2
                width: parent.width; height: parent.height / 2 - 1
                Rectangle { width: parent.width / 2 - 1; height: parent.height; radius: 2; color: OpenUI.primary }
                Rectangle { width: parent.width / 2 - 1; height: parent.height; radius: 2; color: OpenUI.primary }
            }
            Rectangle { width: parent.width; height: parent.height / 2 - 1; radius: 2; color: OpenUI.primary }
        }
    }

    Component {
        id: fourQuadPreview
        Grid {
            columns: 2
            rows: 2
            spacing: 2
            Repeater {
                model: 4
                Rectangle { width: parent.width / 2 - 1; height: parent.height / 2 - 1; radius: 2; color: OpenUI.primary }
            }
        }
    }

    Component {
        id: threeColPreview
        Row {
            spacing: 2
            Rectangle { width: parent.width / 3 - 1; height: parent.height; radius: 2; color: OpenUI.primary }
            Rectangle { width: parent.width / 3 - 1; height: parent.height; radius: 2; color: OpenUI.primary }
            Rectangle { width: parent.width / 3 - 1; height: parent.height; radius: 2; color: OpenUI.primary }
        }
    }
}
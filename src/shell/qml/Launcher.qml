import QtQuick 2.15
import QtQuick.Controls 2.15 as QQC

/* OPENOS 应用启动器 (应用抽屉)
 * - 数据: shell.apps (由 ShellBackend.refreshApps() 从 vmapp 各虚拟化环境
 *   的 .desktop 解析)
 * - OPENUI 风格: 卡片 + 键盘优先导航 + 状态层叠
 * - 分组: 顶部"系统应用"(宿主), 下方按 vmapp 环境分组
 */
Rectangle {
    id: launcher
    visible: false
    width: 420
    height: Math.min(480, contentH + 40)
    radius: OpenUI.shapeLg   /* 大圆角浮窗 (24) */
    color: Qt.rgba(OpenUI.surface6.r, OpenUI.surface6.g, OpenUI.surface6.b,
                   OpenUI.glassMenuAlpha)   /* 更高透明度 (毛玻璃) */
    border.color: OpenUI.outlineVariant; border.width: 1

    property bool ready: false
    property double contentH: 0

    // 定位在面板 Menu 按钮正下方
    x: parent.width - width - 8
    y: parent.height + 4
    z: 20
    transformOrigin: Item.TopRight

    // 打开动画: 淡入 + 缩放 (OPENUI dur200 + OutCubic)
    opacity: 0
    scale: 0.96
    onVisibleChanged: {
        if (visible) {
            opacity = 0; scale = 0.96
            showAnim.restart()
            if (!ready) { shell.refreshApps(); ready = true; }
        }
    }

    // 显式打开动画 (比 Behavior 可靠)
    ParallelAnimation {
        id: showAnim
        NumberAnimation {
            target: launcher; property: "opacity"; to: 1
            duration: OpenUI.dur200; easing.type: Easing.OutCubic
        }
        NumberAnimation {
            target: launcher; property: "scale"; to: 1
            duration: OpenUI.dur200; easing.type: Easing.OutCubic
        }
    }

    Column {
        anchors.fill: parent
        anchors.margins: OpenUI.sp2
        spacing: 6

        // 标题
        Text {
            text: "应用程序"
            color: OpenUI.onSurface
            font.pixelSize: OpenUI.typeTitle
            font.bold: true
        }

        // 搜索框 (键盘优先)
        TextField {
            id: search
            width: parent.width; height: 26
            placeholderText: "搜索应用…"
            placeholderTextColor: OpenUI.onSurfaceVariant
            color: OpenUI.onSurface
            font.pixelSize: 13
            Rectangle {
                z: -1; anchors.fill: parent
                radius: OpenUI.shapeXs
                color: Qt.rgba(OpenUI.surfaceBright.r, OpenUI.surfaceBright.g,
                               OpenUI.surfaceBright.b, 0.5)
            }
            onTextChanged: appList.visible = true
        }

        // 应用列表 (按名称过滤)
        ListView {
            id: appList
            width: parent.width
            height: Math.min(380, Math.max(120, launcher.height - 90))
            clip: true
            model: shell.apps
            section.property: "vmapp"
            section.labelPositioning: ViewSection.InlineLabels
            section.delegate: Rectangle {
                width: appList.width; height: 22; color: "transparent"
                Text {
                    anchors.fill: parent
                    anchors.leftMargin: OpenUI.sp2
                    verticalAlignment: Text.AlignVCenter
                    text: section == "" ? "系统应用" : ("虚拟环境: " + section)
                    color: OpenUI.onSurfaceVariant
                    font.pixelSize: 11; font.bold: true
                }
            }
            delegate: Rectangle {
                id: item
                width: appList.width; height: 32; radius: OpenUI.shapeXs
                color: mouse.containsMouse || mouse2.pressed
                       ? Qt.rgba(OpenUI.onSurface.r, OpenUI.onSurface.g,
                                 OpenUI.onSurface.b,
                                 mouse2.pressed ? OpenUI.pressedAlpha
                                                : OpenUI.hoverAlpha)
                       : "transparent"
                Behavior on color { ColorAnimation { duration: OpenUI.dur100 } }

                // hover 左侧指示条 (更明显的悬停反馈)
                Rectangle {
                    x: 0; y: 6; width: 3; height: parent.height - 12
                    radius: 2
                    color: OpenUI.primary
                    opacity: mouse.containsMouse ? 1.0 : 0.0
                    Behavior on opacity { NumberAnimation { duration: OpenUI.dur100 } }
                }

                Row {
                    anchors.fill: parent
                    anchors.leftMargin: OpenUI.sp3 + 6
                    spacing: OpenUI.sp2
                    // 文字符号图标 (禁 Emoji) — hover 时转主色
                    Text {
                        width: 18; height: parent.height
                        verticalAlignment: Text.AlignVCenter
                        text: "\u2609"
                        color: mouse.containsMouse ? OpenUI.primary
                                                   : OpenUI.onSurfaceVariant
                        font.pixelSize: 14
                        Behavior on color { ColorAnimation { duration: OpenUI.dur100 } }
                    }
                    // 项内缩进 (更宽松, 普通用户更习惯)
                    Rectangle { width: 4; height: 1; color: "transparent" }
                    Text {
                        width: parent.width - 60
                        height: parent.height
                        verticalAlignment: Text.AlignVCenter
                        text: model.name
                        elide: Text.ElideRight
                        color: mouse.containsMouse ? OpenUI.onSurface
                                                   : OpenUI.onSurfaceVariant
                        font.pixelSize: 13
                        Behavior on color { ColorAnimation { duration: OpenUI.dur100 } }
                    }
                    // vmapp 环境徽标
                    Text {
                        width: 40; height: parent.height
                        verticalAlignment: Text.AlignVCenter
                        text: model.vmapp
                        color: OpenUI.onSurfaceVariant
                        font.pixelSize: 10
                        visible: model.vmapp !== ""
                    }
                }
                MouseArea {
                    id: mouse
                    anchors.fill: parent
                    hoverEnabled: true
                }
                MouseArea {
                    id: mouse2
                    anchors.fill: parent
                    onClicked: {
                        shell.launchInVmapp(model.vmapp, model.exec)
                        launcher.visible = false
                    }
                }
            }
        }
    }

    // 点击外部关闭
    MouseArea {
        anchors.fill: parent.parent
        z: -1
        onClicked: launcher.visible = false
    }
}

import QtQuick 2.15
import QtQuick.Window 2.15
import QtQml 2.15

/* OPENOS 屏幕录制工具
 * 选择录制区域、音频源、画质，开始/暂停/停止录制
 */
Window {
    id: screenRecorderWin
    visible: true
    width: 420
    height: 360
    flags: Qt.FramelessWindowHint | Qt.WindowStaysOnTopHint
    title: "屏幕录制"
    color: OpenUI.background

    // 窗口入场动画
    opacity: 0
    onVisibleChanged: {
        if (visible) {
            opacity = 0
            enterAnim.restart()
        }
    }
    NumberAnimation {
        id: enterAnim
        target: screenRecorderWin
        property: "opacity"
        to: 1
        duration: OpenUI.dur200
        easing.type: Easing.OutCubic
    }

    // 窗口拖动
    MouseArea {
        id: dragArea
        anchors.top: parent.top
        anchors.left: parent.left
        anchors.right: parent.right
        height: titleBar.height + OpenUI.sp2
        propagateComposedEvents: true
        onPressed: mouse.accepted
        onReleased: mouse.accepted
        onPositionChanged: {
            screenRecorderWin.x += delta.x
            screenRecorderWin.y += delta.y
        }
    }

    // 计时器
    property Timer timer: Timer {
        interval: 1000
        repeat: true
        onTriggered: elapsedSeconds++
    }

    Column {
        anchors.fill: parent
        anchors.margins: OpenUI.sp4
        anchors.topMargin: OpenUI.sp3
        spacing: OpenUI.sp3

        // 标题栏
        Row {
            id: titleBar
            width: parent.width
            spacing: OpenUI.sp2

            Text {
                text: "屏幕录制"
                color: OpenUI.onSurface
                font.pixelSize: OpenUI.typeTitle
                font.bold: true
                anchors.verticalCenter: parent.verticalCenter
            }

            Item { width: parent.width - 120; height: 1 }

            Rectangle {
                width: 28
                height: 28
                radius: OpenUI.shapeXs
                color: closeHover.hovered
                       ? Qt.rgba(OpenUI.error.r, OpenUI.error.g, OpenUI.error.b, OpenUI.hoverAlpha)
                       : "transparent"

                ThemedIcon {
                    anchors.centerIn: parent
                    name: "window-close"; ctx: "Actions"; size: 14; color: OpenUI.onSurface
                }

                MouseArea {
                    id: closeHover
                    anchors.fill: parent
                    hoverEnabled: true
                    onClicked: screenRecorderWin.visible = false
                }
            }
        }

        // 录制区域选择
        Column {
            spacing: OpenUI.sp1
            width: parent.width

            Text {
                text: "录制区域"
                color: OpenUI.onSurfaceVariant
                font.pixelSize: OpenUI.typeLabelM
            }

            Row {
                spacing: OpenUI.sp2
                width: parent.width

                Repeater {
                    model: ListModel {
                        ListElement { icon: "screenshot-fullscreen"; label: "全屏";   value: "full" }
                        ListElement { icon: "screenshot-region"; label: "区域";   value: "region" }
                        ListElement { icon: "screenshot-window"; label: "窗口";   value: "window" }
                    }

                    Rectangle {
                        width: (parent.width - OpenUI.sp2 * 2) / 3
                        height: 36
                        radius: OpenUI.shapeXs
                        color: areaSelected === model.value
                               ? Qt.rgba(OpenUI.primary.r, OpenUI.primary.g, OpenUI.primary.b, 0.2)
                               : Qt.rgba(OpenUI.surfaceBright.r, OpenUI.surfaceBright.g, OpenUI.surfaceBright.b, OpenUI.glassCardAlpha)
                        border.color: areaSelected === model.value ? OpenUI.primary : "transparent"
                        border.width: 1

                        Row {
                            anchors.centerIn: parent
                            spacing: OpenUI.sp2

                            ThemedIcon {
                                name: model.icon; ctx: "Actions"; size: 14
                                color: areaSelected === model.value ? OpenUI.primary : OpenUI.onSurfaceVariant
                            }

                            Text {
                                text: model.label
                                color: areaSelected === model.value ? OpenUI.primary : OpenUI.onSurfaceVariant
                                font.pixelSize: OpenUI.typeLabelL
                            }
                        }

                        MouseArea {
                            anchors.fill: parent
                            hoverEnabled: true
                            onClicked: areaSelected = model.value
                        }
                    }
                }
            }
        }

        // 音频源选择
        Column {
            spacing: OpenUI.sp1
            width: parent.width

            Text {
                text: "音频来源"
                color: OpenUI.onSurfaceVariant
                font.pixelSize: OpenUI.typeLabelM
            }

            Row {
                spacing: OpenUI.sp2
                width: parent.width

                Repeater {
                    model: ListModel {
                        ListElement { label: "系统音频"; value: "system" }
                        ListElement { label: "麦克风";  value: "mic" }
                        ListElement { label: "无";      value: "none" }
                    }

                    Rectangle {
                        width: (parent.width - OpenUI.sp2 * 2) / 3
                        height: 28
                        radius: OpenUI.shapeXs
                        color: audioSelected === model.value
                               ? Qt.rgba(OpenUI.primary.r, OpenUI.primary.g, OpenUI.primary.b, 0.2)
                               : Qt.rgba(OpenUI.surfaceBright.r, OpenUI.surfaceBright.g, OpenUI.surfaceBright.b, OpenUI.glassCardAlpha)

                        Text {
                            anchors.centerIn: parent
                            text: model.label
                            color: audioSelected === model.value ? OpenUI.primary : OpenUI.onSurfaceVariant
                            font.pixelSize: OpenUI.typeLabelM
                        }

                        MouseArea {
                            anchors.fill: parent
                            hoverEnabled: true
                            onClicked: audioSelected = model.value
                        }
                    }
                }
            }
        }

        // 画质选择
        Column {
            spacing: OpenUI.sp1
            width: parent.width

            Text {
                text: "画质"
                color: OpenUI.onSurfaceVariant
                font.pixelSize: OpenUI.typeLabelM
            }

            Row {
                spacing: OpenUI.sp2
                width: parent.width

                Repeater {
                    model: ListModel {
                        ListElement { label: "高清"; value: "high" }
                        ListElement { label: "标准"; value: "medium" }
                        ListElement { label: "流畅"; value: "low" }
                    }

                    Rectangle {
                        width: (parent.width - OpenUI.sp2 * 2) / 3
                        height: 32
                        radius: OpenUI.shapeXs
                        color: qualitySelected === model.value
                               ? Qt.rgba(OpenUI.primary.r, OpenUI.primary.g, OpenUI.primary.b, 0.2)
                               : Qt.rgba(OpenUI.surfaceBright.r, OpenUI.surfaceBright.g, OpenUI.surfaceBright.b, OpenUI.glassCardAlpha)

                        Text {
                            anchors.centerIn: parent
                            text: model.label
                            color: qualitySelected === model.value ? OpenUI.primary : OpenUI.onSurfaceVariant
                            font.pixelSize: OpenUI.typeLabelM
                        }

                        MouseArea {
                            anchors.fill: parent
                            hoverEnabled: true
                            onClicked: qualitySelected = model.value
                        }
                    }
                }
            }
        }

        // 录制按钮与状态指示器
        Item {
            width: parent.width
            height: 100

            // 录制状态指示器
            Row {
                visible: isRecording
                anchors.top: parent.top
                anchors.topMargin: OpenUI.sp2
                anchors.horizontalCenter: parent.horizontalCenter
                spacing: OpenUI.sp2

                Rectangle {
                    id: recordingDot
                    width: 10
                    height: 10
                    radius: 5
                    color: OpenUI.error

                    NumberAnimation on opacity {
                        from: 0.3
                        to: 1.0
                        duration: OpenUI.dur300 * 3
                        loops: Animation.Infinite
                        running: isRecording
                        easing.type: Easing.InOutQuad
                    }
                }

                Text {
                    text: "录制中... " + elapsedTimeStr
                    color: OpenUI.error
                    font.pixelSize: OpenUI.typeLabelL
                    anchors.verticalCenter: recordingDot.verticalCenter
                }
            }

            // 录制按钮
            Rectangle {
                id: recordButton
                width: 56
                height: 56
                radius: OpenUI.shapeFull
                anchors.bottom: parent.bottom
                anchors.horizontalCenter: parent.horizontalCenter
                color: isRecording
                       ? Qt.rgba(OpenUI.error.r, OpenUI.error.g, OpenUI.error.b, 0.7)
                       : OpenUI.error
                border.color: OpenUI.onError
                border.width: 2

                // 悬停放大效果
                scale: rbHover.hovered ? 1.08 : 1.0
                Behavior on scale { NumberAnimation { duration: OpenUI.dur100 } }

                ThemedIcon {
                    anchors.centerIn: parent
                    name: isRecording ? "media-playback-pause" : "media-record"
                    ctx: "Actions"; size: 22; color: OpenUI.onError
                }

                MouseArea {
                    id: rbHover
                    anchors.fill: parent
                    hoverEnabled: true
                    onClicked: toggleRecording()
                }
            }

            // 暂停按钮 (仅录制时显示)
            Rectangle {
                visible: isRecording
                width: 40
                height: 40
                radius: OpenUI.shapeFull
                anchors.bottom: parent.bottom
                anchors.bottomMargin: 8
                anchors.right: parent.right
                anchors.rightMargin: OpenUI.sp4
                color: Qt.rgba(OpenUI.surfaceBright.r, OpenUI.surfaceBright.g, OpenUI.surfaceBright.b, OpenUI.glassCardAlpha)

                ThemedIcon { anchors.centerIn: parent; name: "media-playback-pause"; ctx: "Actions"; size: 18; color: OpenUI.onSurface }

                MouseArea {
                    anchors.fill: parent
                    hoverEnabled: true
                    onClicked: togglePause()
                }
            }
        }

        // 保存位置
        Column {
            spacing: OpenUI.sp1
            width: parent.width

            Text {
                text: "保存位置"
                color: OpenUI.onSurfaceVariant
                font.pixelSize: OpenUI.typeLabelM
            }

            Rectangle {
                width: parent.width
                height: 32
                radius: OpenUI.shapeXs
                color: Qt.rgba(OpenUI.surfaceBright.r, OpenUI.surfaceBright.g, OpenUI.surfaceBright.b, OpenUI.glassCardAlpha)

                Row {
                    anchors.fill: parent
                    anchors.leftMargin: OpenUI.sp2
                    spacing: OpenUI.sp2

                    ThemedIcon { name: "open-menu"; ctx: "Actions"; size: 14; color: OpenUI.onSurfaceVariant
                                 anchors.verticalCenter: parent.verticalCenter }

                    Text {
                        id: savePathText
                        text: savePath
                        color: OpenUI.onSurface
                        font.pixelSize: OpenUI.typeBodyM
                        anchors.verticalCenter: parent.verticalCenter
                        elide: Text.ElideMiddle
                        width: parent.width - 32
                    }
                }
            }
        }
    }

    // === 状态与逻辑 ===

    // 录制区域选择
    property string areaSelected: "full"
    // 音频选择
    property string audioSelected: "system"
    // 画质选择
    property string qualitySelected: "high"
    // 是否正在录制
    property bool isRecording: false
    // 是否暂停
    property bool isPaused: false
    // 录制时长
    property int elapsedSeconds: 0

    // 格式化时长
    readonly property string elapsedTimeStr: {
        var mins = Math.floor(elapsedSeconds / 60)
        var secs = elapsedSeconds % 60
        return (mins < 10 ? "0" + mins : mins) + ":" + (secs < 10 ? "0" + secs : secs)
    }

    // 默认保存路径 (由 shell 环境提供)
    readonly property string savePath: {
        var home = Qt.platform.os === "osx" ? "/Users" : "/home"
        return home + "/" + (Qt.platform.os === "osx" ? "cangcang" : "user") + "/Videos/OPENOS录屏"
    }

    // 开始/停止录制
    function toggleRecording() {
        if (!isRecording) {
            startRecording()
        } else {
            stopRecording()
        }
    }

    // 暂停/恢复
    function togglePause() {
        isPaused = !isPaused
        if (isPaused) {
            timer.stop()
        } else {
            timer.start()
        }
    }

    function startRecording() {
        console.log("ScreenRecorder: start",
                    "area:", areaSelected,
                    "audio:", audioSelected,
                    "quality:", qualitySelected,
                    "path:", savePath)
        isRecording = true
        isPaused = false
        elapsedSeconds = 0
        timer.start()
        // 实际录制逻辑由 C++ 后端通过 shell 接口调用
        if (typeof shell !== "undefined" && shell.startScreenRecording) {
            shell.startScreenRecording(areaSelected, audioSelected, qualitySelected, savePath)
        }
    }

    function stopRecording() {
        console.log("ScreenRecorder: stop")
        isRecording = false
        isPaused = false
        timer.stop()
        // 通知后端停止录制
        if (typeof shell !== "undefined" && shell.stopScreenRecording) {
            shell.stopScreenRecording()
        }
    }
}

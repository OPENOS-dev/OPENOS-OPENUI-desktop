import QtQuick 2.15
import QtQuick.Window 2.15

/* OPENOS 锁屏 (增强版)
 * 大时钟 + 日期 + 用户头像 + 密码输入 + OAK 对接
 * 对接: 通过 openos-securityd 验证凭据
 */
Rectangle {
    id: lockScreen
    visible: false
    anchors.fill: parent.parent

    // 动画状态
    property bool unlocking: false
    property int failCount: 0

    // 背景层
    Rectangle {
        anchors.fill: parent
        color: Qt.rgba(0x09, 0x09, 0x09, 0.92)
    }

    // 解锁动画 (向外扩散消失)
    Rectangle {
        id: unlockOverlay
        anchors.fill: parent
        color: Qt.rgba(0x09, 0x09, 0x09, 1.0)
        opacity: 0
        visible: false

        NumberAnimation on opacity {
            id: unlockAnim
            from: 1.0; to: 0.0
            duration: 400
            easing.type: Easing.OutCubic
            onStopped: {
                unlockOverlay.visible = false
                lockScreen.visible = false
                lockScreen.unlocking = false
            }
        }
    }

    Column {
        anchors.centerIn: parent
        spacing: OpenUI.sp4

        // 用户头像 (文字符号)
        Rectangle {
            anchors.horizontalCenter: parent.horizontalCenter
            width: 80; height: 80; radius: 40
            color: Qt.rgba(OpenUI.primary.r, OpenUI.primary.g,
                          OpenUI.primary.b, 0.2)
            Text {
                anchors.centerIn: parent
                text: "\u263A"
                color: OpenUI.primary; font.pixelSize: 36
            }
        }

        // 用户名
        Text {
            anchors.horizontalCenter: parent.horizontalCenter
            text: "USER"
            color: OpenUI.onSurface
            font.pixelSize: OpenUI.typeTitleM
        }

        // 大时钟
        Text {
            id: bigClock
            anchors.horizontalCenter: parent.horizontalCenter
            text: Qt.formatTime(new Date(), "hh:mm")
            color: OpenUI.onSurface
            font.pixelSize: OpenUI.typeDisplayM * 2
            font.weight: Font.Light
            Timer {
                interval: 1000; running: true; repeat: true
                onTriggered: bigClock.text = Qt.formatTime(new Date(), "hh:mm")
            }
        }

        // 日期
        Text {
            anchors.horizontalCenter: parent.horizontalCenter
            text: Qt.formatDate(new Date(), "yyyy年M月d日 dddd")
            color: OpenUI.onSurfaceVariant
            font.pixelSize: OpenUI.typeBodyM
        }

        // 密码输入
        Rectangle {
            width: 260; height: 42; radius: OpenUI.shapeXs
            color: Qt.rgba(OpenUI.surfaceBright.r, OpenUI.surfaceBright.g,
                          OpenUI.surfaceBright.b, 0.4)
            border.color: passInput.activeFocus
                          ? OpenUI.primary : OpenUI.outlineVariant
            border.width: passInput.activeFocus ? 2 : 1
            Behavior on border.color { ColorAnimation { duration: OpenUI.dur100 } }

            TextInput {
                id: passInput
                anchors.fill: parent
                anchors.margins: OpenUI.sp2
                echoMode: TextInput.Password
                color: OpenUI.onSurface
                font.pixelSize: OpenUI.typeBodyM
                verticalAlignment: Text.AlignVCenter
                focus: lockScreen.visible
                onAccepted: tryUnlock()
            }
        }

        // 错误信息
        Text {
            id: errText
            anchors.horizontalCenter: parent.horizontalCenter
            text: ""; color: OpenUI.error
            font.pixelSize: OpenUI.typeLabelM
            opacity: 0
            Behavior on opacity { NumberAnimation { duration: OpenUI.dur150 } }
        }

        // 解锁按钮
        Rectangle {
            anchors.horizontalCenter: parent.horizontalCenter
            width: 120; height: 38; radius: OpenUI.shapeXs
            color: unlockBtn.hovered
                   ? Qt.rgba(OpenUI.primary.r, OpenUI.primary.g,
                            OpenUI.primary.b, 0.25)
                   : "transparent"
            border.color: OpenUI.primary; border.width: 1
            Text {
                anchors.centerIn: parent
                text: lockScreen.unlocking ? "解锁中..." : "解锁"
                color: OpenUI.primary; font.pixelSize: OpenUI.typeLabelL
            }
            MouseArea {
                id: unlockBtn
                anchors.fill: parent; hoverEnabled: true
                onClicked: tryUnlock()
            }
        }
    }

    // 底部操作区
    Row {
        anchors { bottom: parent.bottom; bottomMargin: 40; horizontalCenter: parent.horizontalCenter }
        spacing: OpenUI.sp4

        Rectangle {
            width: 120; height: 34; radius: OpenUI.shapeXs
            color: "transparent"
            Text {
                anchors.centerIn: parent
                text: "\u21BB 切换用户"
                color: OpenUI.onSurfaceVariant; font.pixelSize: 12
            }
            MouseArea {
                anchors.fill: parent; hoverEnabled: true
                onClicked: {
                    lockScreen.visible = false
                    loginScreen.visible = true
                }
            }
        }

        Rectangle {
            width: 120; height: 34; radius: OpenUI.shapeXs
            color: "transparent"
            Text {
                anchors.centerIn: parent
                text: "\u23FB 关机"
                color: OpenUI.onSurfaceVariant; font.pixelSize: 12
            }
            MouseArea {
                anchors.fill: parent; hoverEnabled: true
                onClicked: console.log("poweroff")  // 生产: systemctl poweroff
            }
        }
    }

    function tryUnlock() {
        if (lockScreen.unlocking) return
        var pass = passInput.text
        if (pass.length === 0) {
            showError("请输入密码")
            return
        }

        lockScreen.unlocking = true

        // 生产: 调用 openos-securityd 验证 (OAK 握手)
        // 原型: 任意非空密码可解锁
        var success = pass.length > 0

        if (success) {
            errText.opacity = 0
            passInput.text = ""
            lockScreen.failCount = 0
            // 解锁动画
            unlockOverlay.visible = true
            unlockOverlay.opacity = 1.0
            unlockAnim.restart()
        } else {
            lockScreen.failCount++
            showError("密码错误 (第" + lockScreen.failCount + "次)")
            lockScreen.unlocking = false
        }
    }

    function showError(msg) {
        errText.text = msg
        errText.opacity = 1.0
        // 3 秒后淡出
        errorTimer.restart()
    }

    Timer {
        id: errorTimer
        interval: 3000
        onTriggered: errText.opacity = 0
    }
}
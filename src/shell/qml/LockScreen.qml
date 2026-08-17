import QtQuick 2.15
import QtQuick.Window 2.15

/* OPENOS 锁屏
 * 大时钟 + 日期 + 密码输入 (解锁)
 * 对接: OAK 解锁凭据 (/proc/oak/authorize) — 生产经 securityd 验证
 */
Item {
    id: lockScreen
    visible: false
    anchors.fill: parent.parent

    Rectangle {
        anchors.fill: parent
        color: Qt.rgba(0x09, 0x09, 0x09, 0.92)   /* surface-0 高不透明 */
    }

    Column {
        anchors.centerIn: parent
        spacing: OpenUI.sp4

        // 大时钟 (Display)
        Text {
            id: bigClock
            anchors.horizontalCenter: parent.horizontalCenter
            text: Qt.formatTime(new Date(), "hh:mm")
            color: OpenUI.onSurface
            font.pixelSize: OpenUI.typeDisplayM * 2   /* 90px */
            font.weight: Font.Light
            Timer { interval: 1000; running: true; repeat: true
                onTriggered: bigClock.text = Qt.formatTime(new Date(), "hh:mm") }
        }
        Text {
            anchors.horizontalCenter: parent.horizontalCenter
            text: Qt.formatDate(new Date(), "yyyy年M月d日 dddd")
            color: OpenUI.onSurfaceVariant
            font.pixelSize: OpenUI.typeBodyM
        }

        // 密码输入
        Rectangle {
            width: 240; height: 40; radius: OpenUI.shapeXs
            color: Qt.rgba(OpenUI.surfaceBright.r, OpenUI.surfaceBright.g,
                           OpenUI.surfaceBright.b, 0.4)
            border.color: OpenUI.outlineVariant; border.width: 1
            TextInput {
                id: passInput
                anchors.fill: parent
                anchors.margins: OpenUI.sp2
                echoMode: TextInput.Password
                color: OpenUI.onSurface
                font.pixelSize: OpenUI.typeBodyM
                verticalAlignment: Text.AlignVCenter
                onAccepted: tryUnlock()
            }
        }
        Text {
            id: errText
            anchors.horizontalCenter: parent.horizontalCenter
            text: ""; color: OpenUI.error
            font.pixelSize: OpenUI.typeLabelM
        }
    }

    function tryUnlock() {
        if (passInput.text.length > 0) {
            // 生产: 调 openos-securityd 验证 (OAK 握手), 此处演示
            errText.text = ""
            lockScreen.visible = false
            passInput.text = ""
        } else {
            errText.text = "请输入密码"
        }
    }
}

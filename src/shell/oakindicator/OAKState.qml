import QtQuick 2.15

/* OAK 状态指示器组件 (离屏渲染到合成器, 由 oak_bridge 驱动)
 * - color 属性由 C++ OakSocketClient 经 openos-securityd socket 消息绑定
 * - 显示在屏幕角落, 表示 OAK 安全链路状态
 */
Item {
    id: oakIndicator
    objectName: "oakState"
    width: 96
    height: 24

    property color color: "#2196F3"   /* 默认 info 蓝 */
    property string text: "OAK"

    Rectangle {
        anchors.fill: parent
        radius: 12
        color: "transparent"
        border.color: oakIndicator.color
        border.width: 2

        Row {
            anchors.centerIn: parent
            spacing: 6
            Rectangle {                 /* 状态圆点 */
                width: 10; height: 10; radius: 5
                anchors.verticalCenter: parent.verticalCenter
                color: oakIndicator.color
                Behavior on color { ColorAnimation { duration: 150 } }
            }
            Text {
                text: oakIndicator.text
                color: oakIndicator.color
                font.pixelSize: 11
                verticalAlignment: Text.AlignVCenter
            }
        }
    }
}

import QtQuick 2.15
import QtQuick.Window 2.15

/* OPENOS 屏幕键盘 (OPENUI 浮窗)
 * 触摸屏虚拟键盘, 支持中英文切换
 */
Window {
    id: kbdWin
    visible: false
    width: 600; height: 200
    flags: Qt.FramelessWindowHint
    title: "屏幕键盘"
    color: "transparent"

    property bool shift: false
    property bool caps: false

    function sendKey(ch) {
        console.log("key:", ch)
    }

    // 键盘按键组件
    Component {
        id: keyButtonComponent
        Rectangle {
            property string label: ""
            property color btnColor: Qt.rgba(OpenUI.surfaceBright.r, OpenUI.surfaceBright.g, OpenUI.surfaceBright.b, 0.3)
            property color txtColor: OpenUI.onSurface
            signal clicked()
            width: (parent.width - 33) / 10
            height: 32
            radius: OpenUI.shapeXs
            color: keyHover.hovered ? Qt.rgba(OpenUI.primary.r, OpenUI.primary.g, OpenUI.primary.b, 0.2) : btnColor
            Text { anchors.centerIn: parent; text: label; color: txtColor; font.pixelSize: 13 }
            MouseArea { id: keyHover; anchors.fill: parent; hoverEnabled: true; onClicked: parent.clicked() }
        }
    }

    Rectangle {
        anchors.fill: parent; anchors.margins: 1
        radius: OpenUI.shapeLg
        color: Qt.rgba(OpenUI.surface6.r, OpenUI.surface6.g, OpenUI.surface6.b, OpenUI.glassMenuAlpha)
        border.color: OpenUI.outlineVariant; border.width: 1

        Column {
            anchors.fill: parent; anchors.margins: OpenUI.sp2; spacing: 3

            // 字母行 1
            Row { spacing: 3; width: parent.width
                Repeater {
                    model: (caps || shift) ? "QWERTYUIOP".split("") : "qwertyuiop".split("")
                    Loader {
                        sourceComponent: keyButtonComponent
                        onLoaded: { item.label = modelData; item.clicked.connect(function(){ sendKey(modelData) }) }
                    }
                }
            }
            // 字母行 2
            Row { spacing: 3; width: parent.width
                Item { width: 12; height: 1 }
                Repeater {
                    model: (caps || shift) ? "ASDFGHJKL".split("") : "asdfghjkl".split("")
                    Loader {
                        sourceComponent: keyButtonComponent
                        onLoaded: { item.label = modelData; item.clicked.connect(function(){ sendKey(modelData) }) }
                    }
                }
                Item { width: 12; height: 1 }
            }
            // 字母行 3 + 功能键
            Row { spacing: 3; width: parent.width
                Rectangle {
                    width: 40; height: 32; radius: OpenUI.shapeXs
                    color: shiftHover.hovered ? Qt.rgba(OpenUI.primary.r,OpenUI.primary.g,OpenUI.primary.b,0.3) : Qt.rgba(OpenUI.surfaceBright.r,OpenUI.surfaceBright.g,OpenUI.surfaceBright.b,0.3)
                    Text { anchors.centerIn: parent; text: "\u21E7"; color: shift ? OpenUI.primary : OpenUI.onSurface; font.pixelSize: 14 }
                    MouseArea { id: shiftHover; anchors.fill: parent; hoverEnabled: true; onClicked: { shift = !shift; caps = false } }
                }
                Repeater {
                    model: (caps || shift) ? "ZXCVBNM".split("") : "zxcvbnm".split("")
                    Loader {
                        sourceComponent: keyButtonComponent
                        onLoaded: { item.label = modelData; item.clicked.connect(function(){ sendKey(modelData) }) }
                    }
                }
                Rectangle {
                    width: 40; height: 32; radius: OpenUI.shapeXs
                    color: delHover.hovered ? Qt.rgba(OpenUI.error.r,OpenUI.error.g,OpenUI.error.b,0.3) : Qt.rgba(OpenUI.surfaceBright.r,OpenUI.surfaceBright.g,OpenUI.surfaceBright.b,0.3)
                    Text { anchors.centerIn: parent; text: "\u232B"; color: OpenUI.onSurface; font.pixelSize: 14 }
                    MouseArea { id: delHover; anchors.fill: parent; hoverEnabled: true; onClicked: sendKey("backspace") }
                }
            }
            // 空格行
            Row { spacing: 3; width: parent.width
                Rectangle {
                    width: 40; height: 32; radius: OpenUI.shapeXs
                    color: numHover.hovered ? Qt.rgba(OpenUI.onSurface.r,OpenUI.onSurface.g,OpenUI.onSurface.b,OpenUI.hoverAlpha) : Qt.rgba(OpenUI.surfaceBright.r,OpenUI.surfaceBright.g,OpenUI.surfaceBright.b,0.3)
                    Text { anchors.centerIn: parent; text: "123"; color: OpenUI.onSurface; font.pixelSize: 12 }
                    MouseArea { id: numHover; anchors.fill: parent; hoverEnabled: true; onClicked: {} }
                }
                Rectangle {
                    width: width - 86; height: 32; radius: OpenUI.shapeXs
                    color: Qt.rgba(OpenUI.surfaceBright.r,OpenUI.surfaceBright.g,OpenUI.surfaceBright.b,0.3)
                    Text { anchors.centerIn: parent; text: "空格"; color: OpenUI.onSurface; font.pixelSize: 12 }
                    MouseArea { anchors.fill: parent; onClicked: sendKey(" ") }
                }
                Rectangle {
                    width: 40; height: 32; radius: OpenUI.shapeXs
                    color: entHover.hovered ? Qt.rgba(OpenUI.primary.r,OpenUI.primary.g,OpenUI.primary.b,0.3) : Qt.rgba(OpenUI.surfaceBright.r,OpenUI.surfaceBright.g,OpenUI.surfaceBright.b,0.3)
                    Text { anchors.centerIn: parent; text: "\u23CE"; color: OpenUI.primary; font.pixelSize: 14 }
                    MouseArea { id: entHover; anchors.fill: parent; hoverEnabled: true; onClicked: sendKey("enter") }
                }
                Rectangle {
                    width: 40; height: 32; radius: OpenUI.shapeXs
                    color: hideHover.hovered ? Qt.rgba(OpenUI.error.r,OpenUI.error.g,OpenUI.error.b,0.3) : Qt.rgba(OpenUI.surfaceBright.r,OpenUI.surfaceBright.g,OpenUI.surfaceBright.b,0.3)
                    Text { anchors.centerIn: parent; text: "\u00D7"; color: OpenUI.error; font.pixelSize: 14 }
                    MouseArea { id: hideHover; anchors.fill: parent; hoverEnabled: true; onClicked: kbdWin.visible = false }
                }
            }
        }
    }
}
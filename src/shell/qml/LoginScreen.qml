/*
 * Copyright (C) 2026 OPENOS-dev
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the OPENOS-PROJECT-LICENSE (OPL) v1.2.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * OPL for more details.
 *
 * You should have received a copy of the OPL along with this program.
 * If not, see <https://github.com/OPENOS-dev/OPL>.
 */

import QtQuick 2.15

/* OPENOS 登录屏
 * 多用户选择 + 密码/OAK 验证
 * 首次启动时显示, 或从锁屏切换用户时显示
 */
Rectangle {
    id: loginScreen
    visible: false
    anchors.fill: parent.parent

    // 背景层
    Rectangle {
        anchors.fill: parent
        color: Qt.rgba(0x09, 0x09, 0x09, 0.95)
    }

    // 用户列表模型
    property var users: [
        { name: "USER", type: "本地" }
    ]

    property int selectedUser: 0
    property bool authenticating: false

    Column {
        anchors.centerIn: parent
        spacing: OpenUI.sp5

        // 品牌标识
        Text {
            anchors.horizontalCenter: parent.horizontalCenter
            text: "OPENOS"
            color: OpenUI.primary
            font.pixelSize: OpenUI.typeDisplayM
            font.weight: Font.Light
        }

        // 用户选择 (左右切换)
        Row {
            anchors.horizontalCenter: parent.horizontalCenter
            spacing: 30

            // 左箭头
            Rectangle {
                width: 36; height: 36; radius: 18
                anchors.verticalCenter: parent.verticalCenter
                color: leftHover.hovered
                       ? Qt.rgba(OpenUI.onSurface.r, OpenUI.onSurface.g,
                                OpenUI.onSurface.b, 0.1)
                       : "transparent"
                ThemedIcon {
                    anchors.centerIn: parent
                    name: "go-previous"; ctx: "Navigation"; size: 18; color: OpenUI.onSurfaceVariant
                }
                MouseArea {
                    id: leftHover; anchors.fill: parent; hoverEnabled: true
                    onClicked: {
                        selectedUser = (selectedUser - 1 + users.length) % users.length
                    }
                }
            }

            // 当前用户头像
            Rectangle {
                width: 96; height: 96; radius: 48
                color: Qt.rgba(OpenUI.primary.r, OpenUI.primary.g,
                              OpenUI.primary.b, 0.2)
                border.color: OpenUI.primary; border.width: 2
                ThemedIcon {
                    anchors.centerIn: parent
                    name: "user-online"; ctx: "Status"; size: 42; color: OpenUI.primary
                }
            }

            // 右箭头
            Rectangle {
                width: 36; height: 36; radius: 18
                anchors.verticalCenter: parent.verticalCenter
                color: rightHover.hovered
                       ? Qt.rgba(OpenUI.onSurface.r, OpenUI.onSurface.g,
                                OpenUI.onSurface.b, 0.1)
                       : "transparent"
                ThemedIcon {
                    anchors.centerIn: parent
                    name: "go-next"; ctx: "Navigation"; size: 18; color: OpenUI.onSurfaceVariant
                }
                MouseArea {
                    id: rightHover; anchors.fill: parent; hoverEnabled: true
                    onClicked: {
                        selectedUser = (selectedUser + 1) % users.length
                    }
                }
            }
        }

        // 用户名
        Text {
            anchors.horizontalCenter: parent.horizontalCenter
            text: users[selectedUser].name
            color: OpenUI.onSurface
            font.pixelSize: OpenUI.typeTitleM
        }

        // 用户类型标签
        Rectangle {
            anchors.horizontalCenter: parent.horizontalCenter
            width: userType.width + 16; height: 20; radius: OpenUI.shapeXs
            color: Qt.rgba(OpenUI.primary.r, OpenUI.primary.g,
                          OpenUI.primary.b, 0.15)
            Text {
                id: userType
                anchors.centerIn: parent
                text: users[selectedUser].type
                color: OpenUI.primary; font.pixelSize: 11
            }
        }

        // 密码输入
        Rectangle {
            anchors.horizontalCenter: parent.horizontalCenter
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
                focus: loginScreen.visible
                placeholderText: "密码 / OAK 密钥"
                placeholderTextColor: OpenUI.onSurfaceDisabled
                onAccepted: doLogin()
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

        // 登录按钮
        Rectangle {
            anchors.horizontalCenter: parent.horizontalCenter
            width: 160; height: 40; radius: OpenUI.shapeXs
            color: loginBtn.hovered
                   ? Qt.rgba(OpenUI.primary.r, OpenUI.primary.g,
                            OpenUI.primary.b, 0.25)
                   : OpenUI.primary
            Text {
                anchors.centerIn: parent
                text: authenticating ? "验证中..." : "登录"
                color: OpenUI.onPrimary
                font.pixelSize: OpenUI.typeLabelL; font.bold: true
            }
            MouseArea {
                id: loginBtn
                anchors.fill: parent; hoverEnabled: true
                onClicked: doLogin()
            }
        }

        // 其他选项
        Row {
            anchors.horizontalCenter: parent.horizontalCenter
            spacing: OpenUI.sp4

            Rectangle {
                width: 120; height: 30; radius: OpenUI.shapeXs
                color: "transparent"
                Text {
                    anchors.centerIn: parent
                    text: "OAK 密钥登录"
                    color: OpenUI.onSurfaceVariant; font.pixelSize: 11
                }
                MouseArea {
                    anchors.fill: parent; hoverEnabled: true
                    onClicked: {
                        // 生产: 切换到 OAK 密钥验证模式
                        console.log("OAK key login")
                    }
                }
            }

            Rectangle {
                width: 100; height: 30; radius: OpenUI.shapeXs
                color: "transparent"
                Text {
                    anchors.centerIn: parent
                    text: "紧急恢复"
                    color: OpenUI.onSurfaceVariant; font.pixelSize: 11
                }
                MouseArea {
                    anchors.fill: parent; hoverEnabled: true
                    onClicked: {
                        // 生产: 显示 OAK-Seal 恢复选项
                        console.log("emergency recovery")
                    }
                }
            }
        }
    }

    function doLogin() {
        if (authenticating) return
        var pass = passInput.text
        if (pass.length === 0) {
            showError("请输入密码")
            return
        }

        authenticating = true

        // 生产: 调用 openos-securityd 验证
        // 原型: 任意非空密码可登录
        var success = pass.length > 0

        if (success) {
            passInput.text = ""
            errText.opacity = 0
            loginScreen.visible = false
            authenticating = false
        } else {
            showError("密码错误")
            authenticating = false
        }
    }

    function showError(msg) {
        errText.text = msg
        errText.opacity = 1.0
        errorTimer.restart()
    }

    Timer {
        id: errorTimer
        interval: 3000
        onTriggered: errText.opacity = 0
    }
}
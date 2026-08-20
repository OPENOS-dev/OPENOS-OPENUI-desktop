import QtQuick 2.15

/* OPENOS 首次启动欢迎屏 (壳内版, 5 步)
 * 步骤: 欢迎 -> OAK 安全初始化 -> 创建用户 -> opt 包管理 -> 完成
 * 与 openos-welcome/Welcome.qml 保持视觉一致
 */
Rectangle {
    id: welcome
    visible: false
    anchors.fill: parent.parent
    color: OpenUI.background

    property int step: 0
    property bool oakReady: false
    property bool optReady: false
    property bool userCreated: false

    Column {
        anchors.centerIn: parent
        spacing: OpenUI.sp5
        width: 420

        Text { text: "OPENOS"; anchors.horizontalCenter: parent.horizontalCenter
               color: OpenUI.primary; font.pixelSize: OpenUI.typeDisplayM; font.weight: Font.Light }
        Text { text: "DEV2026.1"; anchors.horizontalCenter: parent.horizontalCenter
               color: OpenUI.onSurfaceVariant; font.pixelSize: OpenUI.typeLabelL }

        // 步骤指示器 (5 步)
        Row { anchors.horizontalCenter: parent.horizontalCenter; spacing: 6
            Repeater { model: 5
                Rectangle { width: 36; height: 4; radius: OpenUI.shapeFull
                    color: welcome.step >= index ? OpenUI.primary : OpenUI.outlineVariant
                    Behavior on color { ColorAnimation { duration: OpenUI.dur100 } } }
            }
        }

        // 内容区
        Rectangle {
            width: 420; height: 200; radius: OpenUI.shapeLg
            color: Qt.rgba(OpenUI.surface.r, OpenUI.surface.g, OpenUI.surface.b, 0.8)
            border.color: OpenUI.outlineVariant; border.width: 1
            clip: true

            Column { anchors.fill: parent; anchors.margins: OpenUI.sp5; spacing: OpenUI.sp3
                // 标题
                Text {
                    text: step === 0 ? "欢迎使用 OPENOS" :
                          step === 1 ? "OAK 安全初始化" :
                          step === 2 ? "创建用户账户" :
                          step === 3 ? "初始化包管理 (opt)" :
                          "全部就绪"
                    color: OpenUI.onSurface; font.pixelSize: OpenUI.typeTitle; font.bold: true
                }

                // 描述
                Text {
                    text: step === 0 ? "OPENOS 是一个注重安全和隔离的操作系统。\n我们将引导你完成初始设置。" :
                          step === 1 ? "OPENOS Security 将保护系统核心组件。\n系统将自动启用 OAK 加密模块。" :
                          step === 2 ? "设置你的本地账户和密码。\n此账户用于登录和解锁。" :
                          step === 3 ? "opt 将内置安装 apt 包管理器。\n你还可以安装其他包管理后端。" :
                          "OPENOS 已准备就绪。\n点击完成开始使用。"
                    color: OpenUI.onSurfaceVariant; font.pixelSize: OpenUI.typeBodyM
                    wrapMode: Text.WordWrap; width: parent.width
                    lineHeight: 1.5
                }

                // 状态指示
                Row {
                    spacing: OpenUI.sp2
                    visible: step > 0

                    Rectangle {
                        visible: step >= 1
                        width: oakStatus.width + 20; height: 24; radius: OpenUI.shapeXs
                        color: welcome.oakReady
                               ? Qt.rgba(OpenUI.statusSuccess.r, OpenUI.statusSuccess.g,
                                        OpenUI.statusSuccess.b, 0.15)
                               : Qt.rgba(OpenUI.statusWarning.r, OpenUI.statusWarning.g,
                                        OpenUI.statusWarning.b, 0.15)
                        Text {
                            id: oakStatus
                            anchors.centerIn: parent
                            text: welcome.oakReady ? "\u2713 OAK 安全已启用" : "\u25D2 OAK 安全初始化中..."
                            color: welcome.oakReady ? OpenUI.statusSuccess : OpenUI.statusWarning
                            font.pixelSize: 12
                        }
                    }

                    Rectangle {
                        visible: step >= 3
                        width: optStatus.width + 20; height: 24; radius: OpenUI.shapeXs
                        color: welcome.optReady
                               ? Qt.rgba(OpenUI.statusSuccess.r, OpenUI.statusSuccess.g,
                                        OpenUI.statusSuccess.b, 0.15)
                               : Qt.rgba(OpenUI.statusWarning.r, OpenUI.statusWarning.g,
                                        OpenUI.statusWarning.b, 0.15)
                        Text {
                            id: optStatus
                            anchors.centerIn: parent
                            text: welcome.optReady ? "\u2713 opt 已就绪" : "\u25D2 opt 初始化中..."
                            color: welcome.optReady ? OpenUI.statusSuccess : OpenUI.statusWarning
                            font.pixelSize: 12
                        }
                    }
                }

                // 步骤 2: 创建用户表单
                Column {
                    visible: step === 2
                    spacing: OpenUI.sp2
                    width: parent.width

                    Row { spacing: OpenUI.sp2; width: parent.width
                        Text { text: "用户名:"; width: 60; height: 32
                               verticalAlignment: Text.AlignVCenter
                               color: OpenUI.onSurface; font.pixelSize: 13 }
                        Rectangle { width: parent.width - 70; height: 32; radius: OpenUI.shapeXs
                            color: Qt.rgba(OpenUI.surfaceBright.r, OpenUI.surfaceBright.g,
                                          OpenUI.surfaceBright.b, 0.4)
                            TextInput { id: userNameInput; anchors.fill: parent; anchors.margins: 6
                                color: OpenUI.onSurface; font.pixelSize: 13
                                verticalAlignment: Text.AlignVCenter
                                text: "user" }
                        }
                    }
                    Row { spacing: OpenUI.sp2; width: parent.width
                        Text { text: "密码:"; width: 60; height: 32
                               verticalAlignment: Text.AlignVCenter
                               color: OpenUI.onSurface; font.pixelSize: 13 }
                        Rectangle { width: parent.width - 70; height: 32; radius: OpenUI.shapeXs
                            color: Qt.rgba(OpenUI.surfaceBright.r, OpenUI.surfaceBright.g,
                                          OpenUI.surfaceBright.b, 0.4)
                            TextInput { id: passInput; anchors.fill: parent; anchors.margins: 6
                                echoMode: TextInput.Password
                                color: OpenUI.onSurface; font.pixelSize: 13
                                verticalAlignment: Text.AlignVCenter } }
                    }
                    Row { spacing: OpenUI.sp2; width: parent.width
                        Text { text: "确认:"; width: 60; height: 32
                               verticalAlignment: Text.AlignVCenter
                               color: OpenUI.onSurface; font.pixelSize: 13 }
                        Rectangle { width: parent.width - 70; height: 32; radius: OpenUI.shapeXs
                            color: Qt.rgba(OpenUI.surfaceBright.r, OpenUI.surfaceBright.g,
                                          OpenUI.surfaceBright.b, 0.4)
                            TextInput { id: passConfirm; anchors.fill: parent; anchors.margins: 6
                                echoMode: TextInput.Password
                                color: OpenUI.onSurface; font.pixelSize: 13
                                verticalAlignment: Text.AlignVCenter } }
                    }
                }
            }
        }

        // 按钮
        Row { anchors.horizontalCenter: parent.horizontalCenter; spacing: OpenUI.sp3
            Rectangle { width: 130; height: 40; radius: OpenUI.shapeXs
                color: Qt.rgba(OpenUI.primary.r, OpenUI.primary.g, OpenUI.primary.b, 0.2)
                Text { anchors.centerIn: parent; text: step > 0 ? "上一步" : "跳过"
                       color: OpenUI.primary; font.pixelSize: OpenUI.typeLabelL }
                MouseArea { anchors.fill: parent; hoverEnabled: true
                    onClicked: step > 0 ? step-- : welcome.visible = false }
            }
            Rectangle { width: 130; height: 40; radius: OpenUI.shapeXs
                color: OpenUI.primary
                opacity: (step === 2 && !userCreated) ? 0.5 : 1.0
                Text { anchors.centerIn: parent; text: step < 4 ? "下一步" : "完成"
                       color: OpenUI.onPrimary; font.pixelSize: OpenUI.typeLabelL }
                MouseArea { anchors.fill: parent; hoverEnabled: true
                    onClicked: {
                        if (step === 2 && !userCreated) {
                            if (passInput.text.length > 0 &&
                                passInput.text === passConfirm.text) {
                                welcome.userCreated = true
                                console.log("create user:", userNameInput.text)
                            }
                        }
                        if (step < 4) {
                            step++
                            if (step === 1) oakTimer.start()
                            if (step === 3) optTimer.start()
                        } else {
                            welcome.visible = false
                        }
                    }
                }
            }
        }
    }

    Timer { id: oakTimer; interval: 1500; onTriggered: { welcome.oakReady = true; console.log("OAK initialized") } }
    Timer { id: optTimer; interval: 2000; onTriggered: { welcome.optReady = true; console.log("opt initialized") } }
}
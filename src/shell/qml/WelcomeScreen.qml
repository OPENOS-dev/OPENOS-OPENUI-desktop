import QtQuick 2.15

/* OPENOS 首次启动欢迎屏
 * 展示 OPENOS 特色 (OAK 安全 / 隔离 / 桌面), 引导初始化
 * 生产: 对接 OAK 初始化 + opt 首次安装 + 创建用户
 */
Rectangle {
    id: welcome
    visible: true
    anchors.fill: parent.parent
    color: OpenUI.background

    property int step: 0

    Column {
        anchors.centerIn: parent
        spacing: OpenUI.sp5
        width: 420

        Text { text: "OPENOS"; anchors.horizontalCenter: parent.horizontalCenter
               color: OpenUI.primary; font.pixelSize: OpenUI.typeDisplayM; font.weight: Font.Light }
        Text { text: "DEV2026.1"; anchors.horizontalCenter: parent.horizontalCenter
               color: OpenUI.onSurfaceVariant; font.pixelSize: OpenUI.typeLabelL }

        // 步骤指示
        Row { anchors.horizontalCenter: parent.horizontalCenter; spacing: 6
            Repeater { model: 3
                Rectangle { width: 40; height: 4; radius: OpenUI.shapeFull
                    color: welcome.step >= index ? OpenUI.primary : OpenUI.outlineVariant }
            }
        }

        // 内容区
        Rectangle {
            width: 420; height: 160; radius: OpenUI.shapeLg
            color: Qt.rgba(OpenUI.surface.r, OpenUI.surface.g, OpenUI.surface.b, 0.8)
            Column { anchors.fill: parent; anchors.margins: OpenUI.sp5; spacing: OpenUI.sp3
                Text {
                    text: step === 0 ? "OAK 安全初始化" :
                          step === 1 ? "初始化包管理 (opt)" : "完成设置"
                    color: OpenUI.onSurface; font.pixelSize: OpenUI.typeTitle; font.bold: true
                }
                Text {
                    text: step === 0 ? "系统将启用 OPENOS Security, 保护核心子安全主体" :
                          step === 1 ? "opt 将内置安装 apt, 并可安装其他包管理后端" :
                          "你可以开始使用 OPENOS 了"
                    color: OpenUI.onSurfaceVariant; font.pixelSize: OpenUI.typeBodyM
                    wrapMode: Text.WordWrap; width: parent.width
                }
                Text { text: step === 1 ? "(进入隔离视图 /vmapp/opt 安装)" : ""
                       color: OpenUI.onSurfaceDisabled; font.pixelSize: OpenUI.typeLabelS }
            }
        }

        // 按钮
        Row { anchors.horizontalCenter: parent.horizontalCenter; spacing: OpenUI.sp3
            Rectangle { width: 130; height: 40; radius: OpenUI.shapeXs
                color: Qt.rgba(OpenUI.primary.r, OpenUI.primary.g, OpenUI.primary.b, 0.2)
                Text { anchors.centerIn: parent; text: welcome.step > 0 ? "上一步" : "跳过"
                       color: OpenUI.primary; font.pixelSize: OpenUI.typeLabelL }
                MouseArea { anchors.fill: parent; hoverEnabled: true
                    onClicked: welcome.step > 0 ? welcome.step-- : welcome.visible = false }
            }
            Rectangle { width: 130; height: 40; radius: OpenUI.shapeXs
                color: OpenUI.primary
                Text { anchors.centerIn: parent; text: welcome.step < 2 ? "下一步" : "完成"
                       color: OpenUI.onPrimary; font.pixelSize: OpenUI.typeLabelL }
                MouseArea { anchors.fill: parent; hoverEnabled: true
                    onClicked: welcome.step < 2 ? welcome.step++ : welcome.visible = false }
            }
        }
    }
}

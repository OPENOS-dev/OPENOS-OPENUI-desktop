import QtQuick 2.15

/* 设置中心页面集合 (AppearancePage/SecurityPage/PackagesPage/SystemPage/NetworkPage)
 * 供 SettingsCenter.qml 引用
 */

// ---- 外观 ----
Item {
    id: appearancePage
    Column { spacing: OpenUI.sp3; anchors.fill: parent
        Text { text: "外观"; color: OpenUI.onSurface; font.pixelSize: OpenUI.typeHeadlineM; font.bold: true }
        Text { text: "强调色"; color: OpenUI.onSurfaceVariant; font.pixelSize: OpenUI.typeBodyM }
        Row { spacing: OpenUI.sp2
            Repeater {
                model: ["#00BCD4", "#9FC85F", "#F44336", "#6EB3C0"]
                Rectangle {
                    width: 28; height: 28; radius: OpenUI.shapeFull
                    color: modelData
                    border.width: 2; border.color: OpenUI.onSurface
                    MouseArea { anchors.fill: parent; hoverEnabled: true; onClicked: {} }
                }
            }
        }
        Text { text: "圆角大小"; color: OpenUI.onSurfaceVariant; font.pixelSize: OpenUI.typeBodyM }
        Slider { from: 4; to: 32; value: OpenUI.shapeSm; width: parent.width - 40
            onMoved: console.log("corner", value) }
        Text { text: "玻璃透明度"; color: OpenUI.onSurfaceVariant; font.pixelSize: OpenUI.typeBodyM }
        Slider { from: 0.3; to: 0.95; value: OpenUI.glassPanelAlpha; width: parent.width - 40
            onMoved: console.log("glass", value) }
    }
}

// ---- 安全 (OAK) ----
Item {
    id: securityPage
    Column { spacing: OpenUI.sp3; anchors.fill: parent
        Text { text: "OPENOS Security"; color: OpenUI.onSurface; font.pixelSize: OpenUI.typeHeadlineM; font.bold: true }
        Text { text: "OAK 安全体系状态 (经 /proc/oak)"; color: OpenUI.onSurfaceVariant; font.pixelSize: OpenUI.typeBodyM }

        // 子安全主体状态
        Rectangle {
            width: parent.width; height: 90; radius: OpenUI.shapeSm
            color: Qt.rgba(OpenUI.surfaceBright.r, OpenUI.surfaceBright.g,
                           OpenUI.surfaceBright.b, 0.4)
            Column { anchors.fill: parent; anchors.margins: OpenUI.sp3; spacing: 4
                Text { text: "内置子安全主体"; color: OpenUI.onSurface; font.pixelSize: OpenUI.typeLabelL; font.bold: true }
                Repeater {
                    model: ListModel {
                        ListElement { name: "System";     state: "已保护" }
                        ListElement { name: "OPT";        state: "已保护" }
                        ListElement { name: "Application"; state: "已保护" }
                    }
                    Row { spacing: OpenUI.sp3
                        Text { width: 100; text: model.name; color: OpenUI.onSurface; font.pixelSize: OpenUI.typeLabelM }
                        Text { text: model.state; color: OpenUI.primary; font.pixelSize: OpenUI.typeLabelM }
                    }
                }
            }
        }

        // 白名单
        Rectangle {
            width: parent.width; height: 70; radius: OpenUI.shapeSm
            color: Qt.rgba(OpenUI.surfaceBright.r, OpenUI.surfaceBright.g,
                           OpenUI.surfaceBright.b, 0.4)
            Column { anchors.fill: parent; anchors.margins: OpenUI.sp3; spacing: 4
                Text { text: "OAK 认证白名单"; color: OpenUI.onSurface; font.pixelSize: OpenUI.typeLabelL; font.bold: true }
                Text { text: "无第三方进程 (生产经 /proc/oak/whitelist 动态更新)";
                       color: OpenUI.onSurfaceVariant; font.pixelSize: OpenUI.typeLabelM }
            }
        }

        Text { text: "生产: 读取 /proc/oak/subjects 实时状态 + 经 openos-settingsd 管理";
               color: OpenUI.onSurfaceDisabled; font.pixelSize: OpenUI.typeLabelS }
    }
}

// ---- 软件包 (opt) ----
Item {
    id: packagesPage
    Column { spacing: OpenUI.sp3; anchors.fill: parent
        Text { text: "软件包管理 (opt)"; color: OpenUI.onSurface; font.pixelSize: OpenUI.typeHeadlineM; font.bold: true }
        Text { text: "已安装后端: apt (内置)"; color: OpenUI.onSurfaceVariant; font.pixelSize: OpenUI.typeBodyM }
        Rectangle {
            width: parent.width; height: 36; radius: OpenUI.shapeXs
            color: Qt.rgba(OpenUI.primary.r, OpenUI.primary.g, OpenUI.primary.b, 0.2)
            Text { anchors.centerIn: parent; text: "检查更新"
                   color: OpenUI.primary; font.pixelSize: OpenUI.typeLabelL; font.bold: true }
            MouseArea { anchors.fill: parent; hoverEnabled: true
                onClicked: console.log("opt: check updates") }
        }
        Text { text: "生产: 调用 opt 命令 (opt apt-get update 等)";
               color: OpenUI.onSurfaceDisabled; font.pixelSize: OpenUI.typeLabelS }
    }
}

// ---- 系统 ----
Item {
    id: systemPage
    Column { spacing: OpenUI.sp3; anchors.fill: parent
        Text { text: "系统"; color: OpenUI.onSurface; font.pixelSize: OpenUI.typeHeadlineM; font.bold: true }
        Repeater {
            model: ListModel {
                ListElement { label: "系统信息"; detail: "OPENOS DEV2026.1" }
                ListElement { label: "内核版本"; detail: "linux-6.12.103" }
                ListElement { label: "主机名";   detail: "openos" }
            }
            Row { width: parent.width; spacing: OpenUI.sp4
                Text { width: 100; text: model.label; color: OpenUI.onSurface; font.pixelSize: OpenUI.typeBodyM }
                Text { text: model.detail; color: OpenUI.onSurfaceVariant; font.pixelSize: OpenUI.typeBodyM }
            }
        }
    }
}

// ---- 网络 ----
Item {
    id: networkPage
    Column { spacing: OpenUI.sp3; anchors.fill: parent
        Text { text: "网络"; color: OpenUI.onSurface; font.pixelSize: OpenUI.typeHeadlineM; font.bold: true }
        Rectangle {
            width: parent.width; height: 44; radius: OpenUI.shapeXs
            color: Qt.rgba(OpenUI.surfaceBright.r, OpenUI.surfaceBright.g,
                           OpenUI.surfaceBright.b, 0.4)
            Row { anchors.fill: parent; anchors.leftMargin: OpenUI.sp3; spacing: OpenUI.sp2
                Text { width: 24; height: parent.height; verticalAlignment: Text.AlignVCenter
                       text: "\u263C"; color: OpenUI.primary }
                Text { height: parent.height; verticalAlignment: Text.AlignVCenter
                       text: "Wi-Fi: 已连接"; color: OpenUI.onSurface; font.pixelSize: OpenUI.typeBodyM }
            }
            MouseArea { anchors.fill: parent; hoverEnabled: true; onClicked: {} }
        }
    }
}

// ---- 软件隔离 (vmapp) ----
Item {
    id: vmappPage
    Column { spacing: OpenUI.sp3; anchors.fill: parent
        Text { text: "软件隔离"; color: OpenUI.onSurface; font.pixelSize: OpenUI.typeHeadlineM; font.bold: true }
        Text { text: "每个软件独立文件系统视图 (/vmapp/<name>)"
               color: OpenUI.onSurfaceVariant; font.pixelSize: OpenUI.typeBodyM }
        Rectangle {
            width: parent.width; height: 200; radius: OpenUI.shapeSm
            color: Qt.rgba(OpenUI.surfaceBright.r, OpenUI.surfaceBright.g,
                           OpenUI.surfaceBright.b, 0.3)
            ListView {
                anchors.fill: parent; anchors.margins: OpenUI.sp2; clip: true
                model: ListModel {
                    ListElement { name: "opt";      size: "128 MB" }
                    ListElement { name: "firefox";  size: "450 MB" }
                    ListElement { name: "code";     size: "320 MB" }
                }
                delegate: Rectangle {
                    width: parent.width; height: 40; radius: OpenUI.shapeXs
                    color: hover.hovered
                           ? Qt.rgba(OpenUI.onSurface.r, OpenUI.onSurface.g,
                                     OpenUI.onSurface.b, OpenUI.hoverAlpha)
                           : "transparent"
                    Row { anchors.fill: parent; anchors.leftMargin: OpenUI.sp3
                        Text { width: 140; height: parent.height; verticalAlignment: Text.AlignVCenter
                               text: model.name; color: OpenUI.onSurface; font.pixelSize: OpenUI.typeBodyM }
                        Text { height: parent.height; verticalAlignment: Text.AlignVCenter
                               text: model.size; color: OpenUI.onSurfaceVariant
                               font.pixelSize: OpenUI.typeLabelM }
                    }
                    MouseArea { id: hover; anchors.fill: parent; hoverEnabled: true }
                }
            }
        }
    }
}

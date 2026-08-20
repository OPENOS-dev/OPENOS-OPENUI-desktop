import QtQuick 2.15

/* OPENOS 运行对话框 (Alt+F2)
 * 输入命令回车执行
 */
Rectangle {
    id: runDlg
    visible: false
    width: 460
    height: 90
    radius: OpenUI.shapeLg
    color: Qt.rgba(OpenUI.surface6.r, OpenUI.surface6.g, OpenUI.surface6.b,
                   OpenUI.glassMenuAlpha)
    border.color: OpenUI.outlineVariant; border.width: 1
    anchors.centerIn: parent

    opacity: 0; scale: 0.95
    onVisibleChanged: {
        if (visible) { opacity = 0; scale = 0.95; anim.restart(); input.forceActiveFocus() }
    }
    ParallelAnimation {
        id: anim
        NumberAnimation { target: runDlg; property: "opacity"; to: 1;
            duration: OpenUI.dur150; easing.type: Easing.OutCubic }
        NumberAnimation { target: runDlg; property: "scale"; to: 1;
            duration: OpenUI.dur150; easing.type: Easing.OutCubic }
    }

    Column {
        anchors.fill: parent; anchors.margins: OpenUI.sp3; spacing: OpenUI.sp2
        Text { text: "运行"; color: OpenUI.onSurfaceVariant;
               font.pixelSize: OpenUI.typeLabelM }
        Row { width: parent.width; spacing: OpenUI.sp2
            ThemedIcon { name: "arrow-right"; ctx: "Navigation"; size: 16; color: OpenUI.primary; width: 20 }
            TextField {
                id: input
                width: parent.width - 20; height: 32
                placeholderText: "输入命令…"
                color: OpenUI.onSurface; font.pixelSize: OpenUI.typeBodyM
                Rectangle { z: -1; anchors.fill: parent; radius: OpenUI.shapeXs
                    color: Qt.rgba(OpenUI.surfaceBright.r, OpenUI.surfaceBright.g,
                                   OpenUI.surfaceBright.b, 0.5) }
                Keys.onReturnPressed: {
                    console.log("run:", input.text)
                    shell.launchApp(input.text)
                    runDlg.visible = false; input.text = ""
                }
                Keys.onEscapePressed: { runDlg.visible = false; input.text = "" }
            }
        }
    }
}

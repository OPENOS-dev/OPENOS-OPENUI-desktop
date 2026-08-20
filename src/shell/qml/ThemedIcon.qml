import QtQuick 2.15

/* 统一图标组件 — 优先 freedesktop SVG, 缺失回退到 Unicode 字符 (NUI)
 *
 * 用法:
 *   ThemedIcon { name: "audio-volume-high"; ctx: "Panel"; size: 16; color: OpenUI.primary }
 *   ThemedIcon { name: model.icon; ctx: "Apps"; size: 44 }   // 应用图标用原色
 *   ThemedIcon { fallback: "\u23FB"; name: "system-power"; ctx: "Actions" }
 *
 * 行为:
 *   - 解析 OpenUI.icon.url() 得到 provider URL
 *   - URL 为空 (主题里没该图标) -> 显示 fallback Unicode 字符
 *   - color 透明 -> 保留 SVG 原色 (彩色 logo); 否则按 color 着色
 */
Item {
    id: root

    property string name: ""
    property string ctx: ""
    property int size: 24
    property color color: "transparent"
    property string fallback: ""

    implicitWidth: size
    implicitHeight: size

    readonly property bool _hasColor: color != Qt.rgba(0, 0, 0, 0)
    readonly property url resolved: name.length
        ? OpenUI.icon.url(name, size, ctx, _hasColor ? color.toString() : "")
        : ""

    Image {
        id: img
        anchors.fill: parent
        source: root.resolved
        fillMode: Image.PreserveAspectFit
        visible: status === Image.Ready && root.resolved != ""
        asynchronous: true
        cache: true
    }

    Text {
        anchors.centerIn: parent
        text: root.fallback
        color: root._hasColor ? root.color : OpenUI.onSurfaceVariant
        font.pixelSize: Math.round(root.size * 0.85)
        visible: root.fallback.length > 0 && (!img.visible)
    }
}

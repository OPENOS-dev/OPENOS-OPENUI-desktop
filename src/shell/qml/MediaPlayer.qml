import QtQuick 2.15; import QtQuick.Window 2.15
Window { id: win; visible: false; width: 420; height: 340; minimumWidth: 320; minimumHeight: 240; flags: Qt.FramelessWindowHint; title: "媒体播放器"; color: "transparent"
  property bool playing: false; property int progress: 40; property int totalTime: 180; property int dragX: 0; property int dragY: 0; property bool dragging: false
  property var playlist: ["曲目 1 - 开场", "曲目 2 - 主题", "曲目 3 - 尾声", "曲目 4 - 安可"]; property int currentTrack: 0
  Rectangle { anchors.top: parent.top; anchors.right: parent.right; anchors.margins: 6; z: 10; width: 22; height: 22; radius: OpenUI.shapeFull
    color: ch.hovered ? Qt.rgba(OpenUI.error.r,OpenUI.error.g,OpenUI.error.b,0.3) : "transparent"
    ThemedIcon { anchors.centerIn: parent; name: "window-close"; ctx: "Actions"; size: 14; color: ch.hovered ? OpenUI.error : OpenUI.onSurfaceVariant }
    MouseArea { id: ch; anchors.fill: parent; hoverEnabled: true; onClicked: win.visible = false } }
  Rectangle { anchors.fill: parent; anchors.margins: 1; radius: OpenUI.shapeLg
    color: Qt.rgba(OpenUI.neutral10.r,OpenUI.neutral10.g,OpenUI.neutral10.b,0.95); border.color: OpenUI.outlineVariant; border.width: 1; clip: true
    Column { anchors.fill: parent; anchors.margins: OpenUI.sp3; spacing: OpenUI.sp3
      Rectangle { width: parent.width; height: 24; color: "transparent"
        Text { anchors.left: parent.left; anchors.verticalCenter: parent.verticalCenter; text: "媒体播放器"; color: OpenUI.onSurface; font.pixelSize: OpenUI.typeLabelL }
        MouseArea { anchors.fill: parent; onPressed: { dragX = mouse.x; dragY = mouse.y; dragging = true }; onMouseXChanged: { if (dragging) { win.x += mouse.x - dragX; win.y += mouse.y - dragY } }; onReleased: dragging = false } }
      Rectangle { width: 100; height: 100; radius: OpenUI.shapeMd; anchors.horizontalCenter: parent.horizontalCenter; color: Qt.rgba(OpenUI.primary.r,OpenUI.primary.g,OpenUI.primary.b,0.2)
        ThemedIcon { anchors.centerIn: parent; name: "multimedia-player"; ctx: "Apps"; size: 48; color: OpenUI.primary } }
      Text { text: playlist[currentTrack]; anchors.horizontalCenter: parent.horizontalCenter; color: OpenUI.onSurface; font.pixelSize: OpenUI.typeBodyM }
      Rectangle { width: parent.width; height: 4; radius: 2; color: Qt.rgba(OpenUI.surfaceBright.r,OpenUI.surfaceBright.g,OpenUI.surfaceBright.b,0.3)
        Rectangle { width: progress / totalTime * parent.width; height: 4; radius: 2; color: OpenUI.primary } }
      Row { anchors.horizontalCenter: parent.horizontalCenter; spacing: OpenUI.sp4
        Rectangle { width: 32; height: 32; radius: 16; color: ph.hovered ? Qt.rgba(OpenUI.onSurface.r,OpenUI.onSurface.g,OpenUI.onSurface.b,0.15) : "transparent"
          ThemedIcon { anchors.centerIn: parent; name: "media-skip-backward"; ctx: "Actions"; size: 16; color: OpenUI.onSurface }; MouseArea { id: ph; anchors.fill: parent; hoverEnabled: true; onClicked: currentTrack = Math.max(0, currentTrack - 1) } }
        Rectangle { width: 40; height: 40; radius: 20; color: ph2.hovered ? Qt.rgba(OpenUI.primary.r,OpenUI.primary.g,OpenUI.primary.b,0.3) : Qt.rgba(OpenUI.primary.r,OpenUI.primary.g,OpenUI.primary.b,0.2)
          ThemedIcon { anchors.centerIn: parent; name: playing ? "media-playback-pause" : "media-playback-start"; ctx: "Actions"; size: 18; color: OpenUI.primary }; MouseArea { id: ph2; anchors.fill: parent; hoverEnabled: true; onClicked: playing = !playing } }
        Rectangle { width: 32; height: 32; radius: 16; color: ph3.hovered ? Qt.rgba(OpenUI.onSurface.r,OpenUI.onSurface.g,OpenUI.onSurface.b,0.15) : "transparent"
          ThemedIcon { anchors.centerIn: parent; name: "media-skip-forward"; ctx: "Actions"; size: 16; color: OpenUI.onSurface }; MouseArea { id: ph3; anchors.fill: parent; hoverEnabled: true; onClicked: currentTrack = Math.min(playlist.length - 1, currentTrack + 1) } } } } } }
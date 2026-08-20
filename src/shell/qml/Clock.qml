import QtQuick 2.15; import QtQuick.Window 2.15
Window { id: win; visible: false; width: 320; height: 360; minimumWidth: 280; minimumHeight: 300; flags: Qt.FramelessWindowHint; title: "时钟"; color: "transparent"
  property int tab: 0; property int dragX: 0; property int dragY: 0; property bool dragging: false
  property var alarms: [{time:"07:00", label:"起床", enabled:true},{time:"12:00", label:"午休", enabled:false},{time:"22:00", label:"睡觉", enabled:true}]
  Rectangle { anchors.top: parent.top; anchors.right: parent.right; anchors.margins: 6; z: 10; width: 22; height: 22; radius: OpenUI.shapeFull
    color: ch.hovered ? Qt.rgba(OpenUI.error.r,OpenUI.error.g,OpenUI.error.b,0.3) : "transparent"
    Text { anchors.centerIn: parent; text: "\u00D7"; color: ch.hovered ? OpenUI.error : OpenUI.onSurfaceVariant; font.pixelSize: 14 }
    MouseArea { id: ch; anchors.fill: parent; hoverEnabled: true; onClicked: win.visible = false } }
  Rectangle { anchors.fill: parent; anchors.margins: 1; radius: OpenUI.shapeLg
    color: Qt.rgba(OpenUI.neutral10.r,OpenUI.neutral10.g,OpenUI.neutral10.b,0.95); border.color: OpenUI.outlineVariant; border.width: 1; clip: true
    Column { anchors.fill: parent; anchors.margins: OpenUI.sp3; spacing: OpenUI.sp2
      Rectangle { width: parent.width; height: 24; color: "transparent"
        MouseArea { anchors.fill: parent; onPressed: { dragX = mouse.x; dragY = mouse.y; dragging = true }; onMouseXChanged: { if (dragging) { win.x += mouse.x - dragX; win.y += mouse.y - dragY } }; onReleased: dragging = false } }
      Text { id: td; text: Qt.formatTime(new Date(),"HH:mm:ss"); anchors.horizontalCenter: parent.horizontalCenter; color: OpenUI.onSurface; font.pixelSize: 42; font.weight: Font.Light
        Timer { interval: 1000; running: true; repeat: true; onTriggered: td.text = Qt.formatTime(new Date(),"HH:mm:ss") } }
      Text { text: Qt.formatDate(new Date(),"yyyy年M月d日 dddd"); anchors.horizontalCenter: parent.horizontalCenter; color: OpenUI.onSurfaceVariant; font.pixelSize: OpenUI.typeLabelM }
      Row { anchors.horizontalCenter: parent.horizontalCenter; spacing: OpenUI.sp2
        Repeater { model: ["时钟","闹钟","计时器"]
          Rectangle { width: 56; height: 26; radius: 13; color: tab === index ? Qt.rgba(OpenUI.primary.r,OpenUI.primary.g,OpenUI.primary.b,0.2) : "transparent"
            Text { anchors.centerIn: parent; text: modelData; color: tab === index ? OpenUI.primary : OpenUI.onSurfaceVariant; font.pixelSize: 11 }
            MouseArea { anchors.fill: parent; onClicked: tab = index } } } }
      Column { visible: tab === 1; width: parent.width; spacing: OpenUI.sp1
        Repeater { model: alarms
          Rectangle { width: parent.width; height: 40; radius: OpenUI.shapeXs; color: Qt.rgba(OpenUI.surfaceBright.r,OpenUI.surfaceBright.g,OpenUI.surfaceBright.b,0.1)
            Row { anchors.fill: parent; anchors.margins: OpenUI.sp2; spacing: OpenUI.sp2
              Column { spacing: 2
                Text { text: modelData.time; color: OpenUI.onSurface; font.pixelSize: 16; font.weight: Font.Light }
                Text { text: modelData.label; color: OpenUI.onSurfaceVariant; font.pixelSize: 10 } }
              Item { width: parent.width - 120; height: 1 }
              Rectangle { width: 32; height: 18; radius: 9; anchors.verticalCenter: parent.verticalCenter; color: modelData.enabled ? Qt.rgba(OpenUI.primary.r,OpenUI.primary.g,OpenUI.primary.b,0.5) : Qt.rgba(OpenUI.surfaceBright.r,OpenUI.surfaceBright.g,OpenUI.surfaceBright.b,0.3)
                Rectangle { width: 14; height: 14; radius: 7; x: modelData.enabled ? 16 : 2; y: 2; color: modelData.enabled ? OpenUI.primary : OpenUI.onSurfaceVariant } } } } } }
      Column { visible: tab === 2; width: parent.width; spacing: OpenUI.sp2; anchors.horizontalCenter: parent.horizontalCenter
        Text { text: "00:05:00"; anchors.horizontalCenter: parent.horizontalCenter; color: OpenUI.onSurface; font.pixelSize: 32; font.weight: Font.Light }
        Rectangle { width: 100; height: 32; radius: 16; anchors.horizontalCenter: parent.horizontalCenter; color: Qt.rgba(OpenUI.primary.r,OpenUI.primary.g,OpenUI.primary.b,0.2)
          Text { anchors.centerIn: parent; text: "开始"; color: OpenUI.primary; font.pixelSize: 12 }
          MouseArea { anchors.fill: parent; hoverEnabled: true } } } } } }
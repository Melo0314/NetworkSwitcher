import QtQuick 2.12
import QtQuick.Controls 2.12
import QtQuick.Layouts 1.12
import "."

Button {
    id: control
    font.family: "Microsoft YaHei"

    property string title: ""
    property string iconSource: ""
    property bool active: false
    property color baseColor: "#1b2230"
    property color hoverColor: "#273142"
    property color pressedColor: "#313d52"
    property color activeColor: "#24364C"
    property color activeHoverColor: "#2D4661"

    background: Rectangle {
        id: buttonBackground
        radius: 8
        color: control.down
               ? (control.active ? control.activeHoverColor : control.pressedColor)
               : (control.hovered ? (control.active ? control.activeHoverColor : control.hoverColor)
                                  : (control.active ? control.activeColor : control.baseColor))
        border.width: 1
        border.color: control.active ? "#5BC0EB" : "#273142"
        opacity: control.enabled ? 1.0 : 0.45

        Behavior on color {
            ColorAnimation {
                duration: 90
            }
        }

        Behavior on border.color {
            ColorAnimation {
                duration: 120
            }
        }
    }

    contentItem: Item {
        anchors.fill: parent

        Column {
            anchors.centerIn: parent
            width: parent.width - 24
            spacing: 12

            ThemedIcon {
                anchors.horizontalCenter: parent.horizontalCenter
                iconSize: 64
                source: control.iconSource
                color: control.active ? "#B8DBFF" : "#E6ECF5"
            }

            Text {
                width: parent.width
                text: control.title
                color: "#F1F5FB"
                font.family: "Microsoft YaHei"
                font.pixelSize: 18
                font.bold: true
                horizontalAlignment: Text.AlignHCenter
            }
        }
    }
}

import QtQuick 2.12
import QtQuick.Controls 2.12
import QtQuick.Layouts 1.12
import "."

Button {
    id: control
    font.family: "Microsoft YaHei"

    property string title: ""
    property string subtitle: ""
    property string iconSource: ""
    property color accentColor: "#5BC0EB"
    property bool active: false
    property bool destructive: false
    property bool compact: false

    implicitWidth: 196
    implicitHeight: compact ? 30 : 42
    leftPadding: compact ? 9 : 12
    rightPadding: compact ? 9 : 12
    topPadding: compact ? 5 : 10
    bottomPadding: compact ? 5 : 10

    background: Rectangle {
        radius: 8
        color: control.down
               ? (control.active ? "#243A54" : "#243044")
               : (control.hovered ? (control.active ? "#22384F" : "#1E2A3A")
                                  : (control.active ? "#1B3043" : "transparent"))
        border.width: 1
        border.color: control.active ? control.accentColor : (control.hovered ? "#314055" : "transparent")
        opacity: control.enabled ? 1.0 : 0.45

        Behavior on color {
            ColorAnimation { duration: 100 }
        }

        Behavior on border.color {
            ColorAnimation { duration: 120 }
        }
    }

    contentItem: RowLayout {
        spacing: 10

        Rectangle {
            Layout.preferredWidth: compact ? 18 : 26
            Layout.preferredHeight: compact ? 18 : 26
            radius: compact ? 5 : 6
            color: control.active ? "#243D59" : "#182230"
            border.width: 1
            border.color: control.active ? control.accentColor : "#2A3649"

            ThemedIcon {
                anchors.centerIn: parent
                iconSize: control.compact ? 12 : 16
                source: control.iconSource
                color: control.destructive ? "#F4A8BC" : (control.active ? "#DDF0FF" : "#D7E2F1")
            }
        }

        ColumnLayout {
            Layout.fillWidth: true
            spacing: control.subtitle.length > 0 ? 2 : 0

            Text {
                Layout.fillWidth: true
                text: control.title
                color: control.destructive ? "#F6BDD0" : "#EEF4FC"
                font.family: control.font.family
                font.pixelSize: control.compact ? 12 : 14
                font.bold: true
                elide: Text.ElideRight
            }

            Text {
                Layout.fillWidth: true
                visible: control.subtitle.length > 0
                text: control.subtitle
                color: "#8FA3C2"
                font.family: control.font.family
                font.pixelSize: 11
                elide: Text.ElideRight
            }
        }
    }
}

import QtQuick 2.12
import QtQuick.Controls 2.12
import QtQuick.Layouts 1.12
import "."

Button {
    id: control
    font.family: "Microsoft YaHei"

    property string iconSource: ""
    property color backgroundColor: "#1F2937"
    property color borderColor: "#344155"
    property color hoverColor: "#273346"
    property color iconColor: "#D8DEE9"
    property color textColor: "#E9EEF6"

    implicitWidth: text.length > 0 ? 122 : 36
    implicitHeight: 36

    background: Rectangle {
        radius: 8
        color: control.hovered ? control.hoverColor : control.backgroundColor
        border.width: 1
        border.color: control.borderColor
    }

    contentItem: RowLayout {
        spacing: 8

        Item {
            Layout.preferredWidth: 18
            Layout.preferredHeight: 18
            visible: control.iconSource.length > 0

            ThemedIcon {
                anchors.centerIn: parent
                iconSize: 18
                source: control.iconSource
                color: control.iconColor
                visible: control.iconSource.length > 0
            }
        }

        Text {
            visible: control.text.length > 0
            text: control.text
            color: control.textColor
            font.family: control.font.family
            font.pixelSize: 14
            font.bold: true
            verticalAlignment: Text.AlignVCenter
        }
    }
}

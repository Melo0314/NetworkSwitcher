import QtQuick 2.12
import QtGraphicalEffects 1.12

Item {
    id: root

    property string source: ""
    property color color: "#D8DEE9"
    property int iconSize: 18

    implicitWidth: iconSize
    implicitHeight: iconSize
    width: iconSize
    height: iconSize

    Image {
        id: iconImage
        anchors.fill: parent
        source: root.source
        fillMode: Image.PreserveAspectFit
        smooth: true
        sourceSize.width: root.iconSize
        sourceSize.height: root.iconSize
        visible: false
    }

    ColorOverlay {
        anchors.fill: iconImage
        source: iconImage
        color: root.color
        visible: root.source.length > 0
    }
}

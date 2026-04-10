import QtGraphicalEffects 1.12
import QtQuick 2.12
import QtQuick.Controls 2.12
import QtQuick.Layouts 1.12
import QtQuick.Window 2.12
import "components"

ApplicationWindow {
    id: root
    visible: false
    width: 820
    height: appController.logExpanded ? 576 : 340
    minimumWidth: 780
    minimumHeight: appController.logExpanded ? 576 : 340
    color: "transparent"
    title: "NetworkSwitcher"
    flags: Qt.FramelessWindowHint | Qt.Window
    font.family: "Microsoft YaHei"

    function showStatusToast(message) {
        if (!message || !message.length) {
            return
        }

        toastText.text = message
        toastAnimation.stop()
        toastLayer.opacity = 0.0
        toastLayer.verticalOffset = 20
        toastAnimation.restart()
    }

    Item {
        id: frameLayer
        anchors.fill: parent
        anchors.margins: 28

        DropShadow {
            anchors.fill: centralCard
            source: centralCard
            horizontalOffset: 0
            verticalOffset: 6
            radius: 14
            samples: 25
            color: "#5A000000"
        }

        Rectangle {
            id: centralCard
            anchors.fill: parent
            radius: 8
            color: "#141821"
            border.width: 1
            border.color: "#20293A"

            MouseArea {
                anchors.fill: parent
                acceptedButtons: Qt.LeftButton
                onPressed: appController.startWindowDrag()
            }

            ColumnLayout {
                anchors.fill: parent
                anchors.margins: 16
                spacing: 16

                RowLayout {
                    Layout.fillWidth: true
                    spacing: 8

                    IconTextButton {
                        Layout.preferredWidth: 36
                        Layout.preferredHeight: 36
                        iconSource: "qrc:/icons/settings.svg"
                        backgroundColor: "#1b2230"
                        borderColor: "#2B3445"
                        hoverColor: "#243044"
                        iconColor: "#DCE6F5"
                        onClicked: {
                            appController.openSettings()
                            settingsWindow.present()
                        }
                    }

                    IconTextButton {
                        Layout.preferredWidth: 36
                        Layout.preferredHeight: 36
                        iconSource: "qrc:/icons/logs.svg"
                        backgroundColor: appController.logExpanded ? "#243044" : "#1b2230"
                        borderColor: "#2B3445"
                        hoverColor: "#243044"
                        iconColor: "#DCE6F5"
                        onClicked: appController.toggleLogPanel()
                    }

                    Item {
                        Layout.fillWidth: true
                    }

                    IconTextButton {
                        Layout.preferredWidth: 36
                        Layout.preferredHeight: 36
                        iconSource: "qrc:/icons/x.svg"
                        backgroundColor: "#1b2230"
                        borderColor: "#2B3445"
                        hoverColor: "#243044"
                        iconColor: "#F38BA8"
                        onClicked: appController.hideMainWindow()
                    }
                }

                Item {
                    id: modeSection
                    Layout.fillWidth: true
                    Layout.preferredHeight: 200
                    property int modeSpacing: 12
                    property int modeButtonWidth: Math.floor((width - modeSpacing * 2) / 3)

                    Row {
                        id: modeRow
                        anchors.horizontalCenter: parent.horizontalCenter
                        anchors.verticalCenter: parent.verticalCenter
                        width: modeSection.width
                        height: modeSection.height
                        spacing: modeSection.modeSpacing

                        ModeButton {
                            width: modeSection.modeButtonWidth
                            height: modeSection.height
                            title: "本地局域网"
                            iconSource: "qrc:/icons/home.svg"
                            active: appController.currentMode === title
                            enabled: !appController.busy
                            onClicked: appController.requestModeChange(title)
                        }

                        ModeButton {
                            width: modeSection.modeButtonWidth
                            height: modeSection.height
                            title: "远程局域网"
                            iconSource: "qrc:/icons/world.svg"
                            active: appController.currentMode === title
                            enabled: !appController.busy
                            onClicked: appController.requestModeChange(title)
                        }

                        ModeButton {
                            width: modeSection.modeButtonWidth
                            height: modeSection.height
                            title: "无规则"
                            iconSource: "qrc:/icons/ban.svg"
                            active: appController.currentMode === title
                            enabled: !appController.busy
                            onClicked: appController.requestModeChange(title)
                        }
                    }
                }

                Rectangle {
                    Layout.fillWidth: true
                    Layout.preferredHeight: appController.logExpanded ? 220 : 0
                    visible: appController.logExpanded
                    radius: 8
                    color: "#10151E"
                    border.width: 1
                    border.color: "#273142"

                    ListView {
                        id: logList
                        anchors.fill: parent
                        anchors.margins: 1
                        clip: true
                        boundsBehavior: Flickable.StopAtBounds
                        model: appController.logsModel
                        spacing: 6
                        leftMargin: 12
                        rightMargin: 12
                        topMargin: 12
                        bottomMargin: 12

                        delegate: Text {
                            width: logList.width - logList.leftMargin - logList.rightMargin
                            text: model.text
                            color: "#DCE6F5"
                            font.family: "Microsoft YaHei"
                            font.pixelSize: 12
                            wrapMode: Text.WrapAnywhere
                            textFormat: Text.PlainText
                        }

                        onCountChanged: {
                            Qt.callLater(function() {
                                if (count > 0) {
                                    positionViewAtEnd()
                                }
                            })
                        }

                        ScrollBar.vertical: ScrollBar {
                            policy: ScrollBar.AsNeeded
                            active: true
                        }
                    }
                }
            }
        }

    }

    Item {
        id: toastLayer
        z: 1000
        anchors.centerIn: frameLayer
        anchors.verticalCenterOffset: verticalOffset
        property real verticalOffset: 20
        property real horizontalPadding: 18
        property real verticalPadding: 14
        property real maxWidth: Math.max(1, root.width * 0.6)
        property real maxTextWidth: Math.max(1, maxWidth - horizontalPadding * 2)
        property real contentWidth: Math.min(maxTextWidth, toastText.implicitWidth)
        width: Math.min(maxWidth, contentWidth + horizontalPadding * 2)
        height: toastText.paintedHeight + verticalPadding * 2
        opacity: 0.0
        visible: opacity > 0.0 || toastAnimation.running
        enabled: false

        ShaderEffectSource {
            id: toastSource
            anchors.fill: parent
            sourceItem: centralCard
            sourceRect: Qt.rect(
                            Math.max(0, toastLayer.x - frameLayer.x),
                            Math.max(0, toastLayer.y - frameLayer.y),
                            toastLayer.width,
                            toastLayer.height)
            live: true
            visible: false
        }

        DropShadow {
            anchors.fill: toastBackground
            source: toastBackground
            horizontalOffset: 0
            verticalOffset: 4
            radius: 10
            samples: 21
            color: "#46000000"
        }

        FastBlur {
            anchors.fill: toastBackground
            source: toastSource
            radius: 24
            transparentBorder: true
        }

        Rectangle {
            id: toastBackground
            anchors.fill: parent
            radius: 14
            color: "#CC111826"
            border.width: 1
            border.color: "#41516A"
        }

        Text {
            id: toastText
            anchors.centerIn: parent
            width: Math.max(1, toastLayer.contentWidth)
            horizontalAlignment: Text.AlignHCenter
            verticalAlignment: Text.AlignVCenter
            wrapMode: Text.Wrap
            elide: Text.ElideNone
            text: ""
            color: "#EAF1FB"
            font.family: "Microsoft YaHei"
            font.pixelSize: 14
            font.bold: true
        }
    }

    Connections {
        target: appController
        onCurrentModeChanged: {
            if (!modeSwitchAnimation.running) {
                modeSwitchAnimation.restart()
            }
        }

        onStatusTextChanged: {
            if (!appController.statusText.length) {
                return
            }
            root.showStatusToast(appController.statusText)
        }
    }

    SequentialAnimation {
        id: toastAnimation
        running: false

        ScriptAction {
            script: {
                toastLayer.opacity = 0.0
                toastLayer.verticalOffset = 20
            }
        }

        ParallelAnimation {
            NumberAnimation {
                target: toastLayer
                property: "opacity"
                from: 0.0
                to: 1.0
                duration: 180
                easing.type: Easing.OutCubic
            }

            NumberAnimation {
                target: toastLayer
                property: "verticalOffset"
                from: 20
                to: 0
                duration: 220
                easing.type: Easing.OutCubic
            }
        }

        PauseAnimation {
            duration: 2000
        }

        ParallelAnimation {
            NumberAnimation {
                target: toastLayer
                property: "opacity"
                from: 1.0
                to: 0.0
                duration: 220
                easing.type: Easing.InCubic
            }

            NumberAnimation {
                target: toastLayer
                property: "verticalOffset"
                from: 0
                to: -12
                duration: 220
                easing.type: Easing.InCubic
            }
        }
    }

    SequentialAnimation {
        id: modeSwitchAnimation
        running: false

        NumberAnimation {
            target: modeSection
            property: "opacity"
            from: 1.0
            to: 0.35
            duration: 90
            easing.type: Easing.InOutQuad
        }

        NumberAnimation {
            target: modeSection
            property: "opacity"
            from: 0.35
            to: 1.0
            duration: 160
            easing.type: Easing.InOutQuad
        }
    }

    SettingsPopup {
        id: settingsWindow
        parentWindow: root
    }

    TrayMenuWindow {
        id: trayMenuWindow

        Component.onCompleted: appController.attachTrayMenuWindow(trayMenuWindow)
    }
}

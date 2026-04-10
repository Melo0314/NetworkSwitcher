import QtGraphicalEffects 1.12
import QtQuick 2.12
import QtQuick.Controls 2.12
import QtQuick.Layouts 1.12
import QtQuick.Window 2.12
import "components"

ApplicationWindow {
    id: trayMenuWindow
    visible: false
    width: 220
    height: 224
    minimumWidth: width
    maximumWidth: width
    minimumHeight: height
    maximumHeight: height
    color: "transparent"
    title: "NetworkSwitcher Tray Menu"
    flags: Qt.Tool | Qt.FramelessWindowHint | Qt.WindowStaysOnTopHint
    font.family: "Microsoft YaHei"

    onActiveChanged: {
        if (!active && visible) {
            hide()
        }
    }

    Shortcut {
        sequence: "Esc"
        onActivated: trayMenuWindow.hide()
    }

    Item {
        anchors.fill: parent
        anchors.margins: 10

        DropShadow {
            anchors.fill: menuCard
            source: menuCard
            horizontalOffset: 0
            verticalOffset: 8
            radius: 16
            samples: 29
            color: "#56000000"
        }

        Rectangle {
            id: menuCard
            anchors.fill: parent
            radius: 8
            color: "#171D29"
            border.width: 1
            border.color: "#2D384D"

            ColumnLayout {
                anchors.fill: parent
                anchors.margins: 10
                spacing: 4

                TrayMenuItem {
                    Layout.fillWidth: true
                    title: "打开主界面"
                    iconSource: "qrc:/icons/eye.svg"
                    compact: true
                    onClicked: {
                        trayMenuWindow.hide()
                        appController.showMainWindow()
                    }
                }

                Rectangle {
                    Layout.fillWidth: true
                    Layout.preferredHeight: 1
                    color: "#243142"
                }

                TrayMenuItem {
                    Layout.fillWidth: true
                    title: "本地局域网"
                    iconSource: "qrc:/icons/home.svg"
                    accentColor: "#5BC0EB"
                    active: appController.currentMode === title
                    compact: true
                    enabled: !appController.busy
                    onClicked: {
                        trayMenuWindow.hide()
                        appController.requestModeChange(title)
                    }
                }

                TrayMenuItem {
                    Layout.fillWidth: true
                    title: "远程局域网"
                    iconSource: "qrc:/icons/world.svg"
                    accentColor: "#7BD88F"
                    active: appController.currentMode === title
                    compact: true
                    enabled: !appController.busy
                    onClicked: {
                        trayMenuWindow.hide()
                        appController.requestModeChange(title)
                    }
                }

                TrayMenuItem {
                    Layout.fillWidth: true
                    title: "无规则"
                    iconSource: "qrc:/icons/ban.svg"
                    accentColor: "#F5A97F"
                    active: appController.currentMode === title
                    compact: true
                    enabled: !appController.busy
                    onClicked: {
                        trayMenuWindow.hide()
                        appController.requestModeChange(title)
                    }
                }

                Item {
                    Layout.fillHeight: true
                }

                Rectangle {
                    Layout.fillWidth: true
                    Layout.preferredHeight: 1
                    color: "#243142"
                }

                TrayMenuItem {
                    Layout.fillWidth: true
                    title: "退出程序"
                    iconSource: "qrc:/icons/x.svg"
                    accentColor: "#F38BA8"
                    destructive: true
                    compact: true
                    onClicked: {
                        trayMenuWindow.hide()
                        appController.closeApplication()
                    }
                }
            }
        }
    }
}

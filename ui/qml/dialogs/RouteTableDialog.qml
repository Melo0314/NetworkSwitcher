import QtQuick 2.12
import QtQuick.Controls 2.12
import QtQuick.Layouts 1.12
import "../components"

Popup {
    id: dialog
    parent: Overlay.overlay
    font.family: "Microsoft YaHei"
    width: 980
    height: 620
    modal: true
    focus: true
    padding: 0
    closePolicy: Popup.CloseOnEscape

    onOpened: settingsViewModel.refreshRoutes()

    background: Rectangle {
        radius: 8
        color: "#171D29"
        border.width: 1
        border.color: "#2D384D"
    }

    ColumnLayout {
        anchors.fill: parent
        anchors.margins: 20
        spacing: 16

        RowLayout {
            Layout.fillWidth: true

            Text {
                text: "本地路由表"
                color: "#F1F5FB"
                font.pixelSize: 20
                font.bold: true
                font.family: popup.font.family
            }

            Item {
                Layout.fillWidth: true
            }

            IconTextButton {
                iconSource: "qrc:/icons/x.svg"
                Layout.preferredWidth: 36
                Layout.preferredHeight: 36
                backgroundColor: "#301A23"
                borderColor: "#5C3341"
                hoverColor: "#472230"
                iconColor: "#F38BA8"
                onClicked: dialog.close()
            }
        }

        Rectangle {
            Layout.fillWidth: true
            Layout.fillHeight: true
            radius: 8
            color: "#111722"
            border.width: 1
            border.color: "#273142"

            ColumnLayout {
                anchors.fill: parent
                anchors.margins: 12
                spacing: 10

                Rectangle {
                    Layout.fillWidth: true
                    Layout.preferredHeight: 40
                    radius: 8
                    color: "#1C2432"

                    RowLayout {
                        anchors.fill: parent
                        anchors.margins: 10
                        spacing: 10

                        Repeater {
                            model: [
                                { title: "目标前缀", width: 220 },
                                { title: "下一跳", width: 140 },
                                { title: "接口", width: 160 },
                                { title: "跃点", width: 70 },
                                { title: "协议", width: 80 },
                                { title: "状态", width: 80 },
                                { title: "策略存储", width: 120 }
                            ]

                            delegate: Text {
                                Layout.preferredWidth: modelData.width
                                Layout.fillHeight: true
                                verticalAlignment: Text.AlignVCenter
                                text: modelData.title
                                color: "#8FA3C2"
                                font.pixelSize: 12
                                font.bold: true
                                font.family: popup.font.family
                                elide: Text.ElideRight
                            }
                        }
                    }
                }

                ListView {
                    id: routeList
                    Layout.fillWidth: true
                    Layout.fillHeight: true
                    clip: true
                    spacing: 8
                    model: settingsViewModel.routeTableModel

                    delegate: Rectangle {
                        width: routeList.width
                        height: 54
                        radius: 8
                        color: index % 2 === 0 ? "#161D29" : "#131925"

                        RowLayout {
                            anchors.fill: parent
                            anchors.margins: 10
                            spacing: 10

                            Text { Layout.preferredWidth: 220; text: destinationPrefix; color: "#E6ECF5"; font.pixelSize: 12; font.family: popup.font.family; elide: Text.ElideRight }
                            Text { Layout.preferredWidth: 140; text: nextHop; color: "#C9D5E9"; font.pixelSize: 12; font.family: popup.font.family; elide: Text.ElideRight }
                            Text { Layout.preferredWidth: 160; text: interfaceAlias; color: "#C9D5E9"; font.pixelSize: 12; font.family: popup.font.family; elide: Text.ElideRight }
                            Text { Layout.preferredWidth: 70; text: routeMetric; color: "#C9D5E9"; font.pixelSize: 12; font.family: popup.font.family; elide: Text.ElideRight }
                            Text { Layout.preferredWidth: 80; text: protocol; color: "#C9D5E9"; font.pixelSize: 12; font.family: popup.font.family; elide: Text.ElideRight }
                            Text { Layout.preferredWidth: 80; text: state; color: "#C9D5E9"; font.pixelSize: 12; font.family: popup.font.family; elide: Text.ElideRight }
                            Text { Layout.preferredWidth: 120; text: policyStore; color: "#8FA3C2"; font.pixelSize: 12; font.family: popup.font.family; elide: Text.ElideRight }
                        }
                    }
                }
            }
        }

        RowLayout {
            Layout.fillWidth: true

            IconTextButton {
                text: "刷新"
                iconSource: "qrc:/icons/route.svg"
                Layout.preferredHeight: 36
                onClicked: settingsViewModel.refreshRoutes()
            }

            Item {
                Layout.fillWidth: true
            }

            IconTextButton {
                text: "清空路由"
                iconSource: "qrc:/icons/trash.svg"
                Layout.preferredHeight: 36
                backgroundColor: "#311C20"
                borderColor: "#5A2C34"
                hoverColor: "#46252C"
                iconColor: "#F38BA8"
                onClicked: clearConfirmDialog.open()
            }
        }
    }

    Popup {
        id: clearConfirmDialog
        parent: Overlay.overlay
        width: 420
        height: 190
        modal: true
        focus: true
        closePolicy: Popup.CloseOnEscape
        x: (parent.width - width) / 2
        y: (parent.height - height) / 2
        padding: 0

        background: Rectangle {
            radius: 8
            color: "#171D29"
            border.width: 1
            border.color: "#2D384D"
        }

        ColumnLayout {
            anchors.fill: parent
            anchors.margins: 18
            spacing: 14

            Text {
                text: "确认清空路由"
                color: "#F1F5FB"
                font.pixelSize: 18
                font.bold: true
                font.family: popup.font.family
            }

            Text {
                Layout.fillWidth: true
                wrapMode: Text.WordWrap
                text: "这会执行系统级的 route -f，清空本机 IPv4 路由。该操作风险较高，是否继续？"
                color: "#D7E0EE"
                font.pixelSize: 13
                font.family: popup.font.family
            }

            RowLayout {
                Layout.fillWidth: true

                Item {
                    Layout.fillWidth: true
                }

                IconTextButton {
                    text: "取消"
                    iconSource: "qrc:/icons/x.svg"
                    onClicked: clearConfirmDialog.close()
                }

                IconTextButton {
                    text: "继续"
                    iconSource: "qrc:/icons/trash.svg"
                    backgroundColor: "#311C20"
                    borderColor: "#5A2C34"
                    hoverColor: "#46252C"
                    iconColor: "#F38BA8"
                    onClicked: {
                        clearConfirmDialog.close()
                        settingsViewModel.clearAllRoutes()
                    }
                }
            }
        }
    }
}

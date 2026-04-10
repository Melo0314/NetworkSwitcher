import QtQuick 2.12
import QtQuick.Controls 2.12
import QtQuick.Layouts 1.12
import QtQuick.Window 2.12
import "components"
import "dialogs"

ApplicationWindow {
    id: popup
    visible: false
    width: 1120
    height: 760
    minimumWidth: width
    maximumWidth: width
    minimumHeight: height
    maximumHeight: height
    modality: Qt.ApplicationModal
    flags: Qt.Dialog | Qt.WindowTitleHint | Qt.WindowCloseButtonHint | Qt.CustomizeWindowHint
    title: "设置"
    color: "#0B1017"
    font.family: "Microsoft YaHei"

    property bool bypassCancelOnClose: false
    property var parentWindow: null
    readonly property int pageMargin: 28
    readonly property int bodySideMargin: 18
    readonly property int bodyTopMargin: 14
    readonly property int contentMaxWidth: 960
    readonly property color lineColor: "#243142"
    readonly property color titleColor: "#EEF4FC"
    readonly property color bodyColor: "#CAD7E8"
    readonly property color hintColor: "#8FA3C2"
    readonly property color fieldBackground: "#101722"
    readonly property color fieldBorderColor: "#324055"
    readonly property color buttonBackground: "#182230"
    readonly property color buttonHoverBackground: "#223246"
    readonly property color buttonPressedBackground: "#2A3C53"
    readonly property color primaryButtonBackground: "#284261"
    readonly property color primaryButtonHoverBackground: "#32537A"
    readonly property color primaryButtonPressedBackground: "#3A628F"

    function centerOnScreen() {
        x = (Screen.width - width) / 2
        y = (Screen.height - height) / 2
    }

    function present() {
        settingsViewModel.beginEdit()
        centerOnScreen()
        show()
        raise()
        requestActivate()
    }

    function dismiss(saveChanges) {
        if (saveChanges) {
            settingsViewModel.save()
        } else {
            settingsViewModel.cancel()
        }
        bypassCancelOnClose = true
        close()
    }

    onClosing: function(close) {
        if (!bypassCancelOnClose) {
            settingsViewModel.cancel()
        }
        bypassCancelOnClose = false
    }

    function normalizeTimeoutValue(rawValue) {
        var numericText = String(rawValue).replace(/,/g, "").trim()
        var parsedValue = parseInt(numericText, 10)
        if (isNaN(parsedValue)) {
            parsedValue = settingsViewModel.operationTimeoutMs
        }
        return Math.max(5000, Math.min(180000, parsedValue))
    }

    function formatTimeoutValue(value) {
        return Number(value).toLocaleString(Qt.locale(), "f", 0)
    }

    Shortcut {
        sequence: "Esc"
        onActivated: popup.dismiss(false)
    }

    Rectangle {
        anchors.fill: parent
        color: popup.color

        ColumnLayout {
            anchors.fill: parent
            spacing: popup.bodyTopMargin

            Item {
                Layout.fillWidth: true
                Layout.preferredHeight: headerColumn.implicitHeight + popup.pageMargin

                ColumnLayout {
                    id: headerColumn
                    x: popup.pageMargin
                    y: popup.pageMargin
                    width: parent.width - popup.pageMargin * 2
                    spacing: 10

                    RowLayout {
                        Layout.fillWidth: true

                        ColumnLayout {
                            Layout.fillWidth: true
                            spacing: 8

                            Text {
                                text: "设置"
                                color: popup.titleColor
                                font.family: popup.font.family
                                font.pixelSize: 30
                                font.bold: true
                            }
                        }

                        Item { Layout.fillWidth: true }
                    }

                    Rectangle {
                        Layout.fillWidth: true
                        Layout.preferredHeight: 1
                        color: popup.lineColor
                    }
                }
            }

            ScrollView {
                id: bodyScroll
                Layout.fillWidth: true
                Layout.fillHeight: true
                clip: true
                ScrollBar.horizontal.policy: ScrollBar.AlwaysOff

                Item {
                    width: bodyScroll.availableWidth
                    implicitHeight: contentColumn.implicitHeight + popup.bodyTopMargin + popup.pageMargin

                    ColumnLayout {
                        id: contentColumn
                        x: Math.max(popup.bodySideMargin, (parent.width - width) / 2)
                        y: 0 //popup.bodyTopMargin
                        width: Math.min(parent.width - popup.bodySideMargin * 2, popup.contentMaxWidth)
                        spacing: 28

                        ColumnLayout {
                            Layout.fillWidth: true
                            spacing: 12

                            Text {
                                text: "本地局域网"
                                color: popup.titleColor
                                font.family: popup.font.family
                                font.pixelSize: 23
                                font.bold: true
                            }

                            Rectangle { Layout.fillWidth: true; Layout.preferredHeight: 1; color: popup.lineColor }

                            Text {
                                Layout.fillWidth: true
                                wrapMode: Text.WordWrap
                                text: "选择用于本地模式的网卡，并配置切换到本地局域网模式时要写入的目标网段。CIDR 每行一条。"
                                color: popup.hintColor
                                font.family: popup.font.family
                                font.pixelSize: 15
                            }

                            Text { text: "网卡"; color: popup.bodyColor; font.family: popup.font.family; font.pixelSize: 15; font.bold: true }

                            RowLayout {
                                Layout.fillWidth: true
                                spacing: 10

                                ComboBox {
                                    id: adapterCombo
                                    Layout.fillWidth: true
                                    Layout.preferredHeight: 40
                                    font.family: popup.font.family
                                    model: settingsViewModel.adapterModel
                                    textRole: "summary"
                                    currentIndex: settingsViewModel.selectedAdapterIndex
                                    onActivated: settingsViewModel.selectedAdapterIndex = currentIndex

                                    contentItem: Text {
                                        leftPadding: 12
                                        rightPadding: 36
                                        verticalAlignment: Text.AlignVCenter
                                        text: adapterCombo.currentIndex >= 0 ? adapterCombo.displayText : "请选择本地局域网网卡"
                                        color: adapterCombo.currentIndex >= 0 ? popup.titleColor : popup.hintColor
                                        font.family: popup.font.family
                                        font.pixelSize: 15
                                        elide: Text.ElideRight
                                    }

                                    background: Rectangle {
                                        color: popup.fieldBackground
                                        border.width: 1
                                        border.color: popup.fieldBorderColor
                                    }

                                    delegate: ItemDelegate {
                                        width: adapterCombo.width
                                        implicitHeight: 40
                                        highlighted: adapterCombo.highlightedIndex === index
                                        font.family: popup.font.family

                                        background: Rectangle {
                                            color: highlighted ? "#1B2838" : "transparent"
                                        }

                                        contentItem: Text {
                                            text: summary
                                            color: popup.titleColor
                                            font.family: popup.font.family
                                            font.pixelSize: 15
                                            elide: Text.ElideRight
                                            verticalAlignment: Text.AlignVCenter
                                        }
                                    }

                                    popup: Popup {
                                        y: adapterCombo.height + 4
                                        width: adapterCombo.width
                                        implicitHeight: Math.min(adapterListView.contentHeight + 8, 320)
                                        padding: 4

                                        contentItem: ListView {
                                            id: adapterListView
                                            clip: true
                                            model: adapterCombo.popup.visible ? adapterCombo.delegateModel : null
                                            currentIndex: adapterCombo.highlightedIndex
                                            implicitHeight: contentHeight
                                        }

                                        background: Rectangle {
                                            color: popup.fieldBackground
                                            border.width: 1
                                            border.color: popup.fieldBorderColor
                                        }
                                    }
                                }

                                Button {
                                    id: refreshAdaptersButton
                                    Layout.preferredHeight: 40
                                    text: "刷新网卡"
                                    font.family: popup.font.family
                                    onClicked: settingsViewModel.refreshAdapters()
                                    background: Rectangle {
                                        color: refreshAdaptersButton.down ? popup.buttonPressedBackground
                                                                          : (refreshAdaptersButton.hovered ? popup.buttonHoverBackground : popup.buttonBackground)
                                        border.width: 1
                                        border.color: popup.fieldBorderColor
                                    }
                                    contentItem: Text {
                                        text: refreshAdaptersButton.text
                                        color: popup.titleColor
                                        font.family: popup.font.family
                                        font.pixelSize: 15
                                        font.bold: true
                                        horizontalAlignment: Text.AlignHCenter
                                        verticalAlignment: Text.AlignVCenter
                                    }
                                }
                            }

                            Text { text: "目标路由 CIDR"; color: popup.bodyColor; font.family: popup.font.family; font.pixelSize: 15; font.bold: true }

                            TextArea {
                                Layout.fillWidth: true
                                Layout.preferredHeight: 220
                                text: settingsViewModel.routeCidrsText
                                color: popup.titleColor
                                font.family: popup.font.family
                                font.pixelSize: 15
                                wrapMode: TextEdit.NoWrap
                                placeholderText: "例如：\n192.168.0.0/16\n10.10.0.0/24"
                                onTextChanged: settingsViewModel.routeCidrsText = text
                                background: Rectangle {
                                    color: popup.fieldBackground
                                    border.width: 1
                                    border.color: popup.fieldBorderColor
                                }
                            }
                        }

                        ColumnLayout {
                            Layout.fillWidth: true
                            spacing: 12

                            Text {
                                text: "OpenVPN"
                                color: popup.titleColor
                                font.family: popup.font.family
                                font.pixelSize: 23
                                font.bold: true
                            }

                            Rectangle { Layout.fillWidth: true; Layout.preferredHeight: 1; color: popup.lineColor }

                            Text {
                                Layout.fillWidth: true
                                wrapMode: Text.WordWrap
                                text: "配置远程局域网模式所需的 OpenVPN 客户端、配置文件、认证信息和附加参数。"
                                color: popup.hintColor
                                font.family: popup.font.family
                                font.pixelSize: 15
                            }

                            Text { text: "可执行文件路径"; color: popup.bodyColor; font.family: popup.font.family; font.pixelSize: 15; font.bold: true }

                            TextField {
                                Layout.fillWidth: true
                                Layout.preferredHeight: 40
                                placeholderText: "例如 C:/Program Files/OpenVPN/bin/openvpn.exe"
                                text: settingsViewModel.openVpnExePath
                                onTextChanged: settingsViewModel.openVpnExePath = text
                                color: popup.titleColor
                                font.family: popup.font.family
                                background: Rectangle {
                                    color: popup.fieldBackground
                                    border.width: 1
                                    border.color: popup.fieldBorderColor
                                }
                            }

                            Text { text: "配置文件路径"; color: popup.bodyColor; font.family: popup.font.family; font.pixelSize: 15; font.bold: true }

                            TextField {
                                Layout.fillWidth: true
                                Layout.preferredHeight: 40
                                placeholderText: ".ovpn 配置路径"
                                text: settingsViewModel.openVpnConfigPath
                                onTextChanged: settingsViewModel.openVpnConfigPath = text
                                color: popup.titleColor
                                font.family: popup.font.family
                                background: Rectangle {
                                    color: popup.fieldBackground
                                    border.width: 1
                                    border.color: popup.fieldBorderColor
                                }
                            }

                            Text { text: "用户名"; color: popup.bodyColor; font.family: popup.font.family; font.pixelSize: 15; font.bold: true }

                            TextField {
                                Layout.fillWidth: true
                                Layout.preferredHeight: 40
                                placeholderText: "OpenVPN 用户名，可留空"
                                text: settingsViewModel.openVpnUsername
                                onTextChanged: settingsViewModel.openVpnUsername = text
                                color: popup.titleColor
                                font.family: popup.font.family
                                background: Rectangle {
                                    color: popup.fieldBackground
                                    border.width: 1
                                    border.color: popup.fieldBorderColor
                                }
                            }

                            Text { text: "密码"; color: popup.bodyColor; font.family: popup.font.family; font.pixelSize: 15; font.bold: true }

                            TextField {
                                id: openVpnPasswordField
                                Layout.fillWidth: true
                                Layout.preferredHeight: 40
                                property bool passwordVisible: false
                                placeholderText: "OpenVPN 密码，可留空"
                                text: settingsViewModel.openVpnPassword
                                onTextChanged: settingsViewModel.openVpnPassword = text
                                echoMode: passwordVisible ? TextInput.Normal : TextInput.Password
                                color: popup.titleColor
                                font.family: popup.font.family
                                rightPadding: 40
                                background: Rectangle {
                                    color: popup.fieldBackground
                                    border.width: 1
                                    border.color: popup.fieldBorderColor
                                }

                                Item {
                                    id: passwordToggleButton
                                    width: 30
                                    height: 30
                                    anchors.right: parent.right
                                    anchors.rightMargin: 5
                                    anchors.verticalCenter: parent.verticalCenter

                                    Rectangle {
                                        anchors.fill: parent
                                        radius: 15
                                        color: passwordToggleArea.pressed ? "#243044"
                                              : (passwordToggleArea.containsMouse ? "#1E2838" : "transparent")
                                        border.width: passwordToggleArea.containsMouse ? 1 : 0
                                        border.color: popup.fieldBorderColor
                                    }

                                    ThemedIcon {
                                        anchors.centerIn: parent
                                        source: openVpnPasswordField.passwordVisible ? "qrc:/icons/eye-off.svg" : "qrc:/icons/eye.svg"
                                        iconSize: 18
                                        color: passwordToggleArea.containsMouse ? "#F3F8FF" : popup.hintColor
                                    }

                                    MouseArea {
                                        id: passwordToggleArea
                                        anchors.fill: parent
                                        hoverEnabled: true
                                        cursorShape: Qt.PointingHandCursor
                                        onClicked: openVpnPasswordField.passwordVisible = !openVpnPasswordField.passwordVisible
                                    }
                                }
                            }

                            Text { text: "额外启动参数"; color: popup.bodyColor; font.family: popup.font.family; font.pixelSize: 15; font.bold: true }

                            TextField {
                                Layout.fillWidth: true
                                Layout.preferredHeight: 40
                                placeholderText: "OpenVPN 额外参数，可留空"
                                text: settingsViewModel.openVpnExtraArgs
                                onTextChanged: settingsViewModel.openVpnExtraArgs = text
                                color: popup.titleColor
                                font.family: popup.font.family
                                background: Rectangle {
                                    color: popup.fieldBackground
                                    border.width: 1
                                    border.color: popup.fieldBorderColor
                                }
                            }
                        }

                        ColumnLayout {
                            Layout.fillWidth: true
                            spacing: 12

                            Text {
                                text: "系统行为"
                                color: popup.titleColor
                                font.family: popup.font.family
                                font.pixelSize: 23
                                font.bold: true
                            }

                            Rectangle { Layout.fillWidth: true; Layout.preferredHeight: 1; color: popup.lineColor }

                            CheckBox {
                                id: autoStartCheckBox
                                padding: 0
                                topPadding: 2
                                bottomPadding: 2
                                spacing: 10
                                text: "开机自启"
                                font.family: popup.font.family
                                checked: settingsViewModel.autoStart
                                onToggled: settingsViewModel.autoStart = checked
                                implicitHeight: Math.max(indicator.implicitHeight, contentItem.implicitHeight) + topPadding + bottomPadding
                                indicator: Rectangle {
                                    x: 0
                                    y: Math.round((autoStartCheckBox.height - height) / 2)
                                    implicitWidth: 18
                                    implicitHeight: 18
                                    color: autoStartCheckBox.checked ? "#2B4768" : popup.fieldBackground
                                    border.width: 1
                                    border.color: autoStartCheckBox.checked ? "#7EB5F1" : popup.fieldBorderColor
                                    Rectangle {
                                        anchors.centerIn: parent
                                        width: 8
                                        height: 8
                                        color: "#F3F8FF"
                                        visible: autoStartCheckBox.checked
                                    }
                                }
                                contentItem: Text {
                                    leftPadding: autoStartCheckBox.indicator.width + autoStartCheckBox.spacing
                                    text: autoStartCheckBox.text
                                    color: popup.bodyColor
                                    font.family: popup.font.family
                                    font.pixelSize: 15
                                    verticalAlignment: Text.AlignVCenter
                                }
                            }

                            CheckBox {
                                id: startMinimizedCheckBox
                                padding: 0
                                topPadding: 2
                                bottomPadding: 2
                                spacing: 10
                                text: "启动时最小化"
                                font.family: popup.font.family
                                checked: settingsViewModel.startMinimized
                                onToggled: settingsViewModel.startMinimized = checked
                                implicitHeight: Math.max(indicator.implicitHeight, contentItem.implicitHeight) + topPadding + bottomPadding
                                indicator: Rectangle {
                                    x: 0
                                    y: Math.round((startMinimizedCheckBox.height - height) / 2)
                                    implicitWidth: 18
                                    implicitHeight: 18
                                    color: startMinimizedCheckBox.checked ? "#2B4768" : popup.fieldBackground
                                    border.width: 1
                                    border.color: startMinimizedCheckBox.checked ? "#7EB5F1" : popup.fieldBorderColor
                                    Rectangle {
                                        anchors.centerIn: parent
                                        width: 8
                                        height: 8
                                        color: "#F3F8FF"
                                        visible: startMinimizedCheckBox.checked
                                    }
                                }
                                contentItem: Text {
                                    leftPadding: startMinimizedCheckBox.indicator.width + startMinimizedCheckBox.spacing
                                    text: startMinimizedCheckBox.text
                                    color: popup.bodyColor
                                    font.family: popup.font.family
                                    font.pixelSize: 15
                                    verticalAlignment: Text.AlignVCenter
                                }
                            }

                            CheckBox {
                                id: restoreLastModeCheckBox
                                padding: 0
                                topPadding: 2
                                bottomPadding: 2
                                spacing: 10
                                text: "启动后自动恢复上次模式"
                                font.family: popup.font.family
                                checked: settingsViewModel.restoreLastMode
                                onToggled: settingsViewModel.restoreLastMode = checked
                                implicitHeight: Math.max(indicator.implicitHeight, contentItem.implicitHeight) + topPadding + bottomPadding
                                indicator: Rectangle {
                                    x: 0
                                    y: Math.round((restoreLastModeCheckBox.height - height) / 2)
                                    implicitWidth: 18
                                    implicitHeight: 18
                                    color: restoreLastModeCheckBox.checked ? "#2B4768" : popup.fieldBackground
                                    border.width: 1
                                    border.color: restoreLastModeCheckBox.checked ? "#7EB5F1" : popup.fieldBorderColor
                                    Rectangle {
                                        anchors.centerIn: parent
                                        width: 8
                                        height: 8
                                        color: "#F3F8FF"
                                        visible: restoreLastModeCheckBox.checked
                                    }
                                }
                                contentItem: Text {
                                    leftPadding: restoreLastModeCheckBox.indicator.width + restoreLastModeCheckBox.spacing
                                    text: restoreLastModeCheckBox.text
                                    color: popup.bodyColor
                                    font.family: popup.font.family
                                    font.pixelSize: 15
                                    verticalAlignment: Text.AlignVCenter
                                }
                            }

                            RowLayout {
                                Layout.fillWidth: true
                                spacing: 12

                                ColumnLayout {
                                    Layout.fillWidth: true
                                    spacing: 4
                                    Text { text: "操作超时"; color: popup.bodyColor; font.family: popup.font.family; font.pixelSize: 15; font.bold: true }
                                    Text {
                                        Layout.fillWidth: true
                                        wrapMode: Text.WordWrap
                                        text: "用于网卡切换、VPN 启停和路由写入的超时时间，单位毫秒。"
                                        color: popup.hintColor
                                        font.family: popup.font.family
                                        font.pixelSize: 14
                                    }
                                }

                                Rectangle {
                                    Layout.preferredWidth: 170
                                    Layout.preferredHeight: 40
                                    color: popup.fieldBackground
                                    border.width: 1
                                    border.color: popup.fieldBorderColor

                                    RowLayout {
                                        anchors.fill: parent
                                        spacing: 0

                                        Button {
                                            id: timeoutDecreaseButton
                                            Layout.preferredWidth: 38
                                            Layout.fillHeight: true
                                            text: "-"
                                            font.family: popup.font.family
                                            onClicked: {
                                                var nextValue = popup.normalizeTimeoutValue(settingsViewModel.operationTimeoutMs - 1000)
                                                settingsViewModel.operationTimeoutMs = nextValue
                                                timeoutField.text = popup.formatTimeoutValue(nextValue)
                                            }
                                            background: Rectangle {
                                                color: timeoutDecreaseButton.down ? popup.buttonPressedBackground
                                                    : (timeoutDecreaseButton.hovered ? popup.buttonHoverBackground : popup.buttonBackground)
                                                border.width: 0
                                            }
                                            contentItem: Text {
                                                text: timeoutDecreaseButton.text
                                                color: popup.titleColor
                                                font.family: popup.font.family
                                                font.pixelSize: 22
                                                horizontalAlignment: Text.AlignHCenter
                                                verticalAlignment: Text.AlignVCenter
                                            }
                                        }

                                        Rectangle {
                                            Layout.preferredWidth: 1
                                            Layout.fillHeight: true
                                            color: popup.fieldBorderColor
                                        }

                                        TextField {
                                            id: timeoutField
                                            Layout.fillWidth: true
                                            Layout.fillHeight: true
                                            text: popup.formatTimeoutValue(settingsViewModel.operationTimeoutMs)
                                            color: popup.titleColor
                                            font.family: popup.font.family
                                            font.pixelSize: 15
                                            font.bold: true
                                            horizontalAlignment: Text.AlignHCenter
                                            verticalAlignment: Text.AlignVCenter
                                            selectByMouse: true
                                            inputMethodHints: Qt.ImhDigitsOnly
                                            background: Rectangle {
                                                color: "transparent"
                                                border.width: 0
                                            }
                                            onEditingFinished: {
                                                var nextValue = popup.normalizeTimeoutValue(text)
                                                settingsViewModel.operationTimeoutMs = nextValue
                                                text = popup.formatTimeoutValue(nextValue)
                                            }
                                        }

                                        Rectangle {
                                            Layout.preferredWidth: 1
                                            Layout.fillHeight: true
                                            color: popup.fieldBorderColor
                                        }

                                        Button {
                                            id: timeoutIncreaseButton
                                            Layout.preferredWidth: 38
                                            Layout.fillHeight: true
                                            text: "+"
                                            font.family: popup.font.family
                                            onClicked: {
                                                var nextValue = popup.normalizeTimeoutValue(settingsViewModel.operationTimeoutMs + 1000)
                                                settingsViewModel.operationTimeoutMs = nextValue
                                                timeoutField.text = popup.formatTimeoutValue(nextValue)
                                            }
                                            background: Rectangle {
                                                color: timeoutIncreaseButton.down ? popup.buttonPressedBackground
                                                    : (timeoutIncreaseButton.hovered ? popup.buttonHoverBackground : popup.buttonBackground)
                                                border.width: 0
                                            }
                                            contentItem: Text {
                                                text: timeoutIncreaseButton.text
                                                color: popup.titleColor
                                                font.family: popup.font.family
                                                font.pixelSize: 22
                                                horizontalAlignment: Text.AlignHCenter
                                                verticalAlignment: Text.AlignVCenter
                                            }
                                        }
                                    }
                                }
                            }
                        }

                        ColumnLayout {
                            Layout.fillWidth: true
                            spacing: 12

                            Text {
                                text: "快捷工具"
                                color: popup.titleColor
                                font.family: popup.font.family
                                font.pixelSize: 23
                                font.bold: true
                            }

                            Rectangle { Layout.fillWidth: true; Layout.preferredHeight: 1; color: popup.lineColor }

                            RowLayout {
                                Layout.fillWidth: true
                                spacing: 10

                                Button {
                                    id: routeTableButton
                                    Layout.preferredHeight: 40
                                    text: "查看本地路由表"
                                    font.family: popup.font.family
                                    onClicked: routeTableDialog.open()
                                    background: Rectangle {
                                        color: routeTableButton.down ? popup.buttonPressedBackground
                                                                     : (routeTableButton.hovered ? popup.buttonHoverBackground : popup.buttonBackground)
                                        border.width: 1
                                        border.color: popup.fieldBorderColor
                                    }
                                    contentItem: Text {
                                        text: routeTableButton.text
                                        color: popup.titleColor
                                        font.family: popup.font.family
                                        font.pixelSize: 15
                                        font.bold: true
                                        horizontalAlignment: Text.AlignHCenter
                                        verticalAlignment: Text.AlignVCenter
                                    }
                                }

                                Button {
                                    id: hostsButton
                                    Layout.preferredHeight: 40
                                    text: "定位 Hosts 文件"
                                    font.family: popup.font.family
                                    onClicked: settingsViewModel.openHostsLocation()
                                    background: Rectangle {
                                        color: hostsButton.down ? popup.buttonPressedBackground
                                                                : (hostsButton.hovered ? popup.buttonHoverBackground : popup.buttonBackground)
                                        border.width: 1
                                        border.color: popup.fieldBorderColor
                                    }
                                    contentItem: Text {
                                        text: hostsButton.text
                                        color: popup.titleColor
                                        font.family: popup.font.family
                                        font.pixelSize: 15
                                        font.bold: true
                                        horizontalAlignment: Text.AlignHCenter
                                        verticalAlignment: Text.AlignVCenter
                                    }
                                }
                            }
                        }
                    }
                }
            }

            Item {
                Layout.fillWidth: true
                Layout.preferredHeight: footerColumn.implicitHeight + popup.pageMargin

                ColumnLayout {
                    id: footerColumn
                    x: popup.pageMargin
                    y: 0
                    width: parent.width - popup.pageMargin * 2
                    spacing: 10

                    Rectangle {
                        Layout.fillWidth: true
                        Layout.preferredHeight: 1
                        color: popup.lineColor
                    }

                    RowLayout {
                        id: footerRow
                        x: popup.pageMargin
                        y: 12
                        width: parent.width - popup.pageMargin * 2
                        spacing: 10

                        Rectangle {
                            color: settingsViewModel.hasUnsavedChanges ? "#1B3047" : "#15212F"
//                            border.width: 1
//                            border.color: settingsViewModel.hasUnsavedChanges ? "#6EA8E6" : "#445168"
                            implicitWidth: footerStatusLabel.implicitWidth + 24
                            implicitHeight: footerStatusLabel.implicitHeight + 12

                            Text {
                                id: footerStatusLabel
                                anchors.centerIn: parent
                                leftPadding: 4
                                rightPadding: 4
                                topPadding: 4
                                bottomPadding: 4
                                text: settingsViewModel.hasUnsavedChanges ? "未保存修改" : "当前配置已同步"
                                color: settingsViewModel.hasUnsavedChanges ? "#C7E3FF" : popup.bodyColor
                                font.family: popup.font.family
                                font.pixelSize: 14
                                font.bold: true
                            }
                        }

                        Item { Layout.fillWidth: true }

                        Button {
                            id: cancelButton
                            text: "取消"
                            font.family: popup.font.family
                            leftPadding: 16
                            rightPadding: 16
                            topPadding: 10
                            bottomPadding: 10
                            onClicked: popup.dismiss(false)
                            background: Rectangle {
                                color: cancelButton.down ? popup.buttonPressedBackground
                                                         : (cancelButton.hovered ? popup.buttonHoverBackground : popup.buttonBackground)
                                border.width: 1
                                border.color: popup.fieldBorderColor
                            }
                            contentItem: Row {
                                spacing: 8
                                anchors.centerIn: parent

                                ThemedIcon {
                                    source: "qrc:/icons/x.svg"
                                    iconSize: 16
                                    color: popup.titleColor
                                }

                                Text {
                                    text: cancelButton.text
                                    color: popup.titleColor
                                    font.family: popup.font.family
                                    font.pixelSize: 15
                                    font.bold: true
                                    verticalAlignment: Text.AlignVCenter
                                }
                            }
                        }

                        Button {
                            id: saveButton
                            text: "保存"
                            font.family: popup.font.family
                            leftPadding: 16
                            rightPadding: 16
                            topPadding: 10
                            bottomPadding: 10
                            onClicked: popup.dismiss(true)
                            background: Rectangle {
                                color: saveButton.down ? popup.primaryButtonPressedBackground
                                                       : (saveButton.hovered ? popup.primaryButtonHoverBackground : popup.primaryButtonBackground)
                                border.width: 1
                                border.color: "#5E8FCA"
                            }
                            contentItem: Row {
                                spacing: 8
                                anchors.centerIn: parent

                                ThemedIcon {
                                    source: "qrc:/icons/device-floppy.svg"
                                    iconSize: 16
                                    color: "#F7FBFF"
                                }

                                Text {
                                    text: saveButton.text
                                    color: "#F7FBFF"
                                    font.family: popup.font.family
                                    font.pixelSize: 15
                                    font.bold: true
                                    verticalAlignment: Text.AlignVCenter
                                }
                            }
                        }
                    }
                }
            }
        }
    }

    RouteTableDialog {
        id: routeTableDialog
        parent: Overlay.overlay
        x: (parent.width - width) / 2
        y: (parent.height - height) / 2
    }
}

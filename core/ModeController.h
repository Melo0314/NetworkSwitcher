#pragma once

#include <QObject>

class NetworkService;
class OpenVpnService;
class SettingsRepository;

class ModeController : public QObject
{
    Q_OBJECT

public:
    explicit ModeController(
        SettingsRepository *settingsRepository,
        NetworkService *networkService,
        OpenVpnService *openVpnService,
        QObject *parent = nullptr);

    QString currentMode() const;
    QString statusText() const;
    bool busy() const;

    bool switchToMode(const QString &mode);

signals:
    void currentModeChanged();
    void statusTextChanged();
    void busyChanged();

private:
    void handleOpenVpnConnectedChanged();
    void handleOpenVpnRunningChanged();
    void handleOpenVpnStatusTextChanged();
    QStringList parseCidrs(const QString &routeText) const;
    void persistLastMode(const QString &mode);
    bool switchToLocalMode();
    bool switchToRemoteMode();
    bool switchToNativeMode();
    void setCurrentMode(const QString &mode);
    void setStatusText(const QString &statusText);
    void setBusy(bool busy);

    SettingsRepository *m_settingsRepository;
    NetworkService *m_networkService;
    OpenVpnService *m_openVpnService;
    QString m_currentMode;
    QString m_statusText;
    bool m_busy;
    bool m_waitingForRemoteConnection;
};

#pragma once

#include <QObject>
#include <QProcess>

class OpenVpnService : public QObject
{
    Q_OBJECT

public:
    explicit OpenVpnService(QObject *parent = nullptr);

    bool startManaged(
        const QString &exePath,
        const QString &configPath,
        const QString &username,
        const QString &password,
        const QString &extraArgs,
        int timeoutMs);
    bool stopManaged(int timeoutMs);

    bool isRunning() const;
    bool isConnected() const;
    QString statusText() const;

signals:
    void runningChanged();
    void connectedChanged();
    void statusTextChanged();

private slots:
    void handleStarted();
    void handleFinished(int exitCode, QProcess::ExitStatus exitStatus);
    void handleStandardOutput();
    void handleStandardError();

private:
    QStringList splitArguments(const QString &arguments) const;
    bool configContainsManagementDirective(const QString &configPath) const;
    bool configRequiresExternalCredentials(const QString &configPath) const;
    bool prepareAuthFile(const QString &username, const QString &password);
    void cleanupAuthFile();
    quint16 reserveManagementPort() const;
    void setConnected(bool connected);
    void setStatusText(const QString &statusText);

    QProcess m_process;
    bool m_connected;
    bool m_managementEnabled;
    quint16 m_managementPort;
    QString m_authFilePath;
    QString m_statusText;
};

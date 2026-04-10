#pragma once

#include <QObject>
#include <QLocalServer>
#include <QSharedMemory>

class SingleInstanceService : public QObject
{
    Q_OBJECT

public:
    explicit SingleInstanceService(const QString &sharedMemoryKey, const QString &serverName, QObject *parent = nullptr);
    ~SingleInstanceService() override;

    bool initialize();
    bool notifyPrimaryInstance(int timeoutMs = 1500);
    bool consumePendingActivationRequest();

signals:
    void activationRequested();

private slots:
    void handleNewConnection();

private:
    bool startLocalServer();

    QString m_serverName;
    QSharedMemory m_sharedMemory;
    QLocalServer m_localServer;
    bool m_isPrimaryInstance;
    bool m_hasPendingActivationRequest;
};

#include "SingleInstanceService.h"

#include <QLocalSocket>

namespace {

const char kActivateMessage[] = "activate";

} // namespace

SingleInstanceService::SingleInstanceService(const QString &sharedMemoryKey, const QString &serverName, QObject *parent)
    : QObject(parent)
    , m_serverName(serverName)
    , m_sharedMemory(sharedMemoryKey)
    , m_isPrimaryInstance(false)
    , m_hasPendingActivationRequest(false)
{
    connect(&m_localServer, &QLocalServer::newConnection, this, &SingleInstanceService::handleNewConnection);
}

SingleInstanceService::~SingleInstanceService()
{
    if (m_localServer.isListening()) {
        m_localServer.close();
        QLocalServer::removeServer(m_serverName);
    }

    if (m_sharedMemory.isAttached()) {
        m_sharedMemory.detach();
    }
}

bool SingleInstanceService::initialize()
{
    if (m_isPrimaryInstance) {
        return true;
    }

    if (!m_sharedMemory.create(1)) {
        return false;
    }

    m_isPrimaryInstance = true;
    startLocalServer();
    return true;
}

bool SingleInstanceService::notifyPrimaryInstance(int timeoutMs)
{
    QLocalSocket socket;
    socket.connectToServer(m_serverName, QIODevice::WriteOnly);
    if (!socket.waitForConnected(timeoutMs)) {
        return false;
    }

    socket.write(kActivateMessage);
    if (!socket.waitForBytesWritten(timeoutMs)) {
        return false;
    }

    socket.flush();
    socket.disconnectFromServer();
    return true;
}

bool SingleInstanceService::consumePendingActivationRequest()
{
    const bool hasPendingRequest = m_hasPendingActivationRequest;
    m_hasPendingActivationRequest = false;
    return hasPendingRequest;
}

void SingleInstanceService::handleNewConnection()
{
    while (m_localServer.hasPendingConnections()) {
        QLocalSocket *socket = m_localServer.nextPendingConnection();
        if (!socket) {
            continue;
        }

        const auto processMessage = [this, socket]() {
            const QByteArray payload = socket->readAll().trimmed();
            if (payload == kActivateMessage) {
                m_hasPendingActivationRequest = true;
                emit activationRequested();
            }
        };

        connect(socket, &QLocalSocket::readyRead, this, [socket, processMessage]() {
            processMessage();
            socket->disconnectFromServer();
        });
        connect(socket, &QLocalSocket::disconnected, socket, &QObject::deleteLater);
        connect(socket, qOverload<QLocalSocket::LocalSocketError>(&QLocalSocket::error), socket, &QObject::deleteLater);

        if (socket->bytesAvailable() > 0) {
            processMessage();
            socket->disconnectFromServer();
        }
    }
}

bool SingleInstanceService::startLocalServer()
{
    if (m_localServer.listen(m_serverName)) {
        return true;
    }

    QLocalServer::removeServer(m_serverName);
    return m_localServer.listen(m_serverName);
}

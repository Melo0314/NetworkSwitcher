#include "OpenVpnService.h"

#include "LoggerService.h"

#include <QFile>
#include <QFileInfo>
#include <QRegularExpression>
#include <QTcpServer>
#include <QTcpSocket>
#include <QTemporaryFile>

namespace {

DECL_LOGGER(logger, "OpenVPN");

}

OpenVpnService::OpenVpnService(QObject *parent)
    : QObject(parent)
    , m_connected(false)
    , m_managementEnabled(false)
    , m_managementPort(0)
    , m_statusText(QStringLiteral("OpenVPN 未启动。"))
{
    m_process.setProcessChannelMode(QProcess::SeparateChannels);

    connect(&m_process, &QProcess::started, this, &OpenVpnService::handleStarted);
    connect(&m_process, QOverload<int, QProcess::ExitStatus>::of(&QProcess::finished), this, &OpenVpnService::handleFinished);
    connect(&m_process, &QProcess::readyReadStandardOutput, this, &OpenVpnService::handleStandardOutput);
    connect(&m_process, &QProcess::readyReadStandardError, this, &OpenVpnService::handleStandardError);
}

bool OpenVpnService::startManaged(
    const QString &exePath,
    const QString &configPath,
    const QString &username,
    const QString &password,
    const QString &extraArgs,
    int timeoutMs)
{
    if (isRunning()) {
        logger.info(QStringLiteral("OpenVPN 已在运行，无需重复启动。"));
        return true;
    }

    if (!QFileInfo::exists(exePath) || !QFileInfo(exePath).isFile()) {
        setStatusText(QStringLiteral("OpenVPN 可执行文件不存在。"));
        logger.error(m_statusText);
        return false;
    }

    if (!QFileInfo::exists(configPath) || !QFileInfo(configPath).isFile()) {
        setStatusText(QStringLiteral("OpenVPN 配置文件不存在。"));
        logger.error(m_statusText);
        return false;
    }

    if (configContainsManagementDirective(configPath)) {
        setStatusText(QStringLiteral("当前 v1 不支持接管带 management 指令的 OpenVPN 配置。"));
        logger.error(m_statusText);
        return false;
    }

    const bool hasUsername = !username.isEmpty();
    const bool hasPassword = !password.isEmpty();
    if (hasUsername != hasPassword) {
        setStatusText(QStringLiteral("OpenVPN 用户名和密码必须同时填写。"));
        logger.error(m_statusText);
        return false;
    }

    if (!hasUsername && configRequiresExternalCredentials(configPath)) {
        setStatusText(QStringLiteral("当前 OpenVPN 配置要求用户名密码认证，请先在设置中填写用户名和密码。"));
        logger.error(m_statusText);
        return false;
    }

    if (!prepareAuthFile(username, password)) {
        return false;
    }

    m_managementPort = reserveManagementPort();
    if (m_managementPort == 0) {
        cleanupAuthFile();
        setStatusText(QStringLiteral("无法为 OpenVPN 管理接口分配端口。"));
        logger.error(m_statusText);
        return false;
    }

    QStringList arguments;
    arguments << QStringLiteral("--config") << configPath;
    if (!m_authFilePath.isEmpty()) {
        arguments << QStringLiteral("--auth-user-pass") << m_authFilePath;
    }
    arguments << QStringLiteral("--auth-retry") << QStringLiteral("none");
    arguments << splitArguments(extraArgs);
    arguments << QStringLiteral("--management") << QStringLiteral("127.0.0.1") << QString::number(m_managementPort);

    m_managementEnabled = true;
    setConnected(false);
    setStatusText(QStringLiteral("OpenVPN 正在启动。"));

    logger.info(QStringLiteral("准备启动 OpenVPN。"));
    m_process.start(exePath, arguments);

    if (!m_process.waitForStarted(timeoutMs)) {
        m_managementEnabled = false;
        cleanupAuthFile();
        setStatusText(QStringLiteral("OpenVPN 启动失败。"));
        logger.error(m_statusText);
        return false;
    }

    return true;
}

bool OpenVpnService::stopManaged(int timeoutMs)
{
    if (!isRunning()) {
        return true;
    }

    if (!m_managementEnabled || m_managementPort == 0) {
        setStatusText(QStringLiteral("当前 OpenVPN 进程不可控，无法优雅停止。"));
        logger.error(m_statusText);
        return false;
    }

    QTcpSocket socket;
    socket.connectToHost(QHostAddress::LocalHost, m_managementPort);
    if (!socket.waitForConnected(qMin(timeoutMs, 5000))) {
        setStatusText(QStringLiteral("无法连接 OpenVPN 管理接口。"));
        logger.error(m_statusText);
        return false;
    }

    socket.waitForReadyRead(500);
    socket.write("signal SIGTERM\n");
    socket.write("quit\n");
    socket.flush();
    socket.waitForBytesWritten(1000);
    socket.disconnectFromHost();

    if (!m_process.waitForFinished(timeoutMs)) {
        setStatusText(QStringLiteral("OpenVPN 未在超时时间内正常退出。"));
        logger.error(m_statusText);
        return false;
    }

    return true;
}

bool OpenVpnService::isRunning() const
{
    return m_process.state() != QProcess::NotRunning;
}

bool OpenVpnService::isConnected() const
{
    return m_connected;
}

QString OpenVpnService::statusText() const
{
    return m_statusText;
}

void OpenVpnService::handleStarted()
{
    setStatusText(QStringLiteral("OpenVPN 已启动，等待隧道建立。"));
    emit runningChanged();
    logger.info(QStringLiteral("OpenVPN 进程已启动。"));
}

void OpenVpnService::handleFinished(int exitCode, QProcess::ExitStatus exitStatus)
{
    Q_UNUSED(exitStatus);

    const QString finalStatus = m_statusText.contains(QStringLiteral("认证失败"))
        ? m_statusText
        : QStringLiteral("OpenVPN 已退出，退出码 %1。").arg(exitCode);

    m_managementEnabled = false;
    m_managementPort = 0;
    setConnected(false);
    cleanupAuthFile();
    setStatusText(finalStatus);
    emit runningChanged();
    logger.info(m_statusText);
}

void OpenVpnService::handleStandardOutput()
{
    const QString output = QString::fromLocal8Bit(m_process.readAllStandardOutput()).trimmed();
    if (output.isEmpty()) {
        return;
    }

    logger.output(output);
    if (output.contains(QStringLiteral("AUTH_FAILED"), Qt::CaseInsensitive)) {
        setStatusText(QStringLiteral("OpenVPN 认证失败。"));
        if (isRunning()) {
            m_process.kill();
        }
        return;
    }

    if (output.contains(QStringLiteral("Initialization Sequence Completed"), Qt::CaseInsensitive)) {
        setConnected(true);
        setStatusText(QStringLiteral("OpenVPN 隧道已建立。"));
    }
}

void OpenVpnService::handleStandardError()
{
    const QString output = QString::fromLocal8Bit(m_process.readAllStandardError()).trimmed();
    if (output.isEmpty()) {
        return;
    }

    logger.output(output);
    if (output.contains(QStringLiteral("AUTH_FAILED"), Qt::CaseInsensitive)) {
        setStatusText(QStringLiteral("OpenVPN 认证失败。"));
        if (isRunning()) {
            m_process.kill();
        }
    }
}

QStringList OpenVpnService::splitArguments(const QString &arguments) const
{
    return arguments.split(QRegularExpression(QStringLiteral("\\s+")), QString::SkipEmptyParts);
}

bool OpenVpnService::configContainsManagementDirective(const QString &configPath) const
{
    QFile file(configPath);
    if (!file.open(QIODevice::ReadOnly | QIODevice::Text)) {
        return false;
    }

    while (!file.atEnd()) {
        const QString line = QString::fromUtf8(file.readLine()).trimmed();
        if (line.isEmpty() || line.startsWith(QLatin1Char('#')) || line.startsWith(QLatin1Char(';'))) {
            continue;
        }

        if (line.startsWith(QStringLiteral("management "), Qt::CaseInsensitive)) {
            return true;
        }
    }

    return false;
}

bool OpenVpnService::configRequiresExternalCredentials(const QString &configPath) const
{
    QFile file(configPath);
    if (!file.open(QIODevice::ReadOnly | QIODevice::Text)) {
        return false;
    }

    const QRegularExpression directiveRegex(QStringLiteral("^auth-user-pass(?:\\s+(\\S+))?$"), QRegularExpression::CaseInsensitiveOption);
    while (!file.atEnd()) {
        const QString line = QString::fromUtf8(file.readLine()).trimmed();
        if (line.isEmpty() || line.startsWith(QLatin1Char('#')) || line.startsWith(QLatin1Char(';'))) {
            continue;
        }

        const QRegularExpressionMatch match = directiveRegex.match(line);
        if (!match.hasMatch()) {
            continue;
        }

        const QString argument = match.captured(1).trimmed();
        return argument.isEmpty() || argument.compare(QStringLiteral("stdin"), Qt::CaseInsensitive) == 0;
    }

    return false;
}

bool OpenVpnService::prepareAuthFile(const QString &username, const QString &password)
{
    cleanupAuthFile();

    if (username.isEmpty() && password.isEmpty()) {
        return true;
    }

    QTemporaryFile authFile;
    authFile.setAutoRemove(false);
    if (!authFile.open()) {
        setStatusText(QStringLiteral("无法创建 OpenVPN 认证文件。"));
        logger.error(m_statusText);
        return false;
    }

    authFile.setPermissions(QFileDevice::ReadOwner | QFileDevice::WriteOwner);
    authFile.write(username.toUtf8());
    authFile.write("\n");
    authFile.write(password.toUtf8());
    authFile.write("\n");
    authFile.close();

    m_authFilePath = authFile.fileName();
    return true;
}

void OpenVpnService::cleanupAuthFile()
{
    if (m_authFilePath.isEmpty()) {
        return;
    }

    QFile::remove(m_authFilePath);
    m_authFilePath.clear();
}

quint16 OpenVpnService::reserveManagementPort() const
{
    QTcpServer server;
    if (!server.listen(QHostAddress::LocalHost, 0)) {
        return 0;
    }

    const quint16 port = server.serverPort();
    server.close();
    return port;
}

void OpenVpnService::setConnected(bool connected)
{
    if (m_connected == connected) {
        return;
    }

    m_connected = connected;
    emit connectedChanged();
}

void OpenVpnService::setStatusText(const QString &statusText)
{
    if (m_statusText == statusText) {
        return;
    }

    m_statusText = statusText;
    emit statusTextChanged();
}

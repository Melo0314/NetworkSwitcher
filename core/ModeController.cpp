#include "ModeController.h"

#include "AppSettings.h"
#include "NetworkTypes.h"
#include "SettingsRepository.h"
#include "../services/LoggerService.h"
#include "../services/NetworkService.h"
#include "../services/OpenVpnService.h"

#include <QRegularExpression>

namespace {

DECL_LOGGER(logger, "Mode");

}

ModeController::ModeController(
    SettingsRepository *settingsRepository,
    NetworkService *networkService,
    OpenVpnService *openVpnService,
    QObject *parent)
    : QObject(parent)
    , m_settingsRepository(settingsRepository)
    , m_networkService(networkService)
    , m_openVpnService(openVpnService)
    , m_currentMode(QStringLiteral("无规则"))
    , m_statusText(QStringLiteral("就绪。"))
    , m_busy(false)
    , m_waitingForRemoteConnection(false)
{
    connect(m_openVpnService, &OpenVpnService::connectedChanged, this, [this]() {
        handleOpenVpnConnectedChanged();
    });
    connect(m_openVpnService, &OpenVpnService::runningChanged, this, [this]() {
        handleOpenVpnRunningChanged();
    });
    connect(m_openVpnService, &OpenVpnService::statusTextChanged, this, [this]() {
        handleOpenVpnStatusTextChanged();
    });
}

QString ModeController::currentMode() const
{
    return m_currentMode;
}

QString ModeController::statusText() const
{
    return m_statusText;
}

bool ModeController::busy() const
{
    return m_busy;
}

bool ModeController::switchToMode(const QString &mode)
{
    if (m_busy) {
        logger.warn(QStringLiteral("已有模式切换正在执行。"));
        return false;
    }

    setBusy(true);
    setStatusText(QStringLiteral("正在切换到 %1。").arg(mode));
    logger.info(m_statusText);

    bool success = false;
    if (mode == QStringLiteral("本地局域网")) {
        success = switchToLocalMode();
    } else if (mode == QStringLiteral("远程局域网")) {
        success = switchToRemoteMode();
    } else if (mode == QStringLiteral("无规则")) {
        success = switchToNativeMode();
    } else {
        setStatusText(QStringLiteral("未知模式：%1").arg(mode));
        logger.error(m_statusText);
    }

    if (!success || !m_waitingForRemoteConnection) {
        setBusy(false);
    }
    return success;
}

QStringList ModeController::parseCidrs(const QString &routeText) const
{
    QStringList cidrs;
    const QStringList lines = routeText.split(QRegularExpression(QStringLiteral("[\r\n]+")), QString::SkipEmptyParts);
    for (const QString &line : lines) {
        const QString trimmed = line.trimmed();
        if (!trimmed.isEmpty()) {
            cidrs.append(trimmed);
        }
    }

    return cidrs;
}

void ModeController::persistLastMode(const QString &mode)
{
    AppSettings settings = m_settingsRepository->load();
    if (settings.lastMode == mode) {
        return;
    }

    settings.lastMode = mode;
    m_settingsRepository->save(settings);
}

bool ModeController::switchToLocalMode()
{
    const AppSettings settings = m_settingsRepository->load();
    const QStringList cidrs = parseCidrs(settings.routeCidrsText);
    const int timeoutMs = settings.operationTimeoutMs > 0 ? settings.operationTimeoutMs : 30000;

    if (settings.localLanAdapterId.trimmed().isEmpty()) {
        setStatusText(QStringLiteral("未配置本地局域网网卡。"));
        logger.error(m_statusText);
        return false;
    }

    if (!m_openVpnService->stopManaged(timeoutMs)) {
        setStatusText(QStringLiteral("关闭 OpenVPN 失败，请查看日志。"));
        return false;
    }

    if (!m_networkService->enableAdapter(settings.localLanAdapterId)) {
        setStatusText(QStringLiteral("启用本地局域网网卡失败。"));
        return false;
    }

    m_networkService->removeRoutes(cidrs);

    const AdapterInfo adapter = m_networkService->findAdapterById(settings.localLanAdapterId);
    if (adapter.id.trimmed().isEmpty()) {
        setStatusText(QStringLiteral("未找到本地局域网网卡。"));
        return false;
    }

    if (!cidrs.isEmpty() && !m_networkService->addRoutes(cidrs, adapter)) {
        setStatusText(QStringLiteral("写入本地局域网路由失败。"));
        return false;
    }

    setCurrentMode(QStringLiteral("本地局域网"));
    persistLastMode(QStringLiteral("本地局域网"));
    setStatusText(QStringLiteral("已切换到本地局域网。"));
    logger.info(m_statusText);
    return true;
}

bool ModeController::switchToRemoteMode()
{
    const AppSettings settings = m_settingsRepository->load();
    const QStringList cidrs = parseCidrs(settings.routeCidrsText);
    const int timeoutMs = settings.operationTimeoutMs > 0 ? settings.operationTimeoutMs : 30000;

    if (settings.openVpnExePath.trimmed().isEmpty() || settings.openVpnConfigPath.trimmed().isEmpty()) {
        setStatusText(QStringLiteral("未配置 OpenVPN 路径或配置文件。"));
        logger.error(m_statusText);
        return false;
    }

    m_networkService->removeRoutes(cidrs);

    if (!settings.localLanAdapterId.trimmed().isEmpty() && !m_networkService->disableAdapter(settings.localLanAdapterId)) {
        setStatusText(QStringLiteral("禁用本地局域网网卡失败。"));
        return false;
    }

    m_waitingForRemoteConnection = true;
    if (!m_openVpnService->startManaged(
            settings.openVpnExePath,
            settings.openVpnConfigPath,
            settings.openVpnUsername,
            settings.openVpnPassword,
            settings.openVpnExtraArgs,
            timeoutMs)) {
        m_waitingForRemoteConnection = false;
        setStatusText(m_openVpnService->statusText());
        return false;
    }

    setStatusText(QStringLiteral("正在切换到远程局域网，等待 OpenVPN 建立连接。"));
    logger.info(m_statusText);
    return true;
}

bool ModeController::switchToNativeMode()
{
    const AppSettings settings = m_settingsRepository->load();
    const QStringList cidrs = parseCidrs(settings.routeCidrsText);
    const int timeoutMs = settings.operationTimeoutMs > 0 ? settings.operationTimeoutMs : 30000;

    if (!m_openVpnService->stopManaged(timeoutMs)) {
        setStatusText(QStringLiteral("关闭 OpenVPN 失败，请查看日志。"));
        return false;
    }

    m_networkService->removeRoutes(cidrs);

    if (!settings.localLanAdapterId.trimmed().isEmpty() && !m_networkService->enableAdapter(settings.localLanAdapterId)) {
        setStatusText(QStringLiteral("恢复本地局域网网卡失败。"));
        return false;
    }

    setCurrentMode(QStringLiteral("无规则"));
    persistLastMode(QStringLiteral("无规则"));
    setStatusText(QStringLiteral("已切换到无规则。"));
    logger.info(m_statusText);
    return true;
}

void ModeController::setCurrentMode(const QString &mode)
{
    if (m_currentMode == mode) {
        return;
    }

    m_currentMode = mode;
    emit currentModeChanged();
}

void ModeController::handleOpenVpnConnectedChanged()
{
    if (!m_waitingForRemoteConnection || !m_openVpnService->isConnected()) {
        return;
    }

    m_waitingForRemoteConnection = false;
    setCurrentMode(QStringLiteral("远程局域网"));
    persistLastMode(QStringLiteral("远程局域网"));
    setStatusText(QStringLiteral("已切换到远程局域网。"));
    logger.info(m_statusText);
    setBusy(false);
}

void ModeController::handleOpenVpnRunningChanged()
{
    if (!m_waitingForRemoteConnection || m_openVpnService->isRunning()) {
        return;
    }

    m_waitingForRemoteConnection = false;
    setStatusText(QStringLiteral("切换到远程局域网失败：%1").arg(m_openVpnService->statusText()));
    logger.error(m_statusText);
    setBusy(false);
}

void ModeController::handleOpenVpnStatusTextChanged()
{
    if (!m_waitingForRemoteConnection || m_openVpnService->isConnected()) {
        return;
    }

    setStatusText(m_openVpnService->statusText());
}

void ModeController::setStatusText(const QString &statusText)
{
    if (m_statusText == statusText) {
        return;
    }

    m_statusText = statusText;
    emit statusTextChanged();
}

void ModeController::setBusy(bool busy)
{
    if (m_busy == busy) {
        return;
    }

    m_busy = busy;
    emit busyChanged();
}

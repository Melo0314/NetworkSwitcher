#include "SettingsViewModel.h"

#include "../core/SettingsRepository.h"
#include "../models/AdapterListModel.h"
#include "../models/RouteTableModel.h"
#include "../services/LoggerService.h"
#include "../services/NetworkService.h"
#include "../services/StartupService.h"

#include <QCoreApplication>

namespace {

DECL_LOGGER(logger, "Settings");

}

SettingsViewModel::SettingsViewModel(
    SettingsRepository *settingsRepository,
    NetworkService *networkService,
    StartupService *startupService,
    AdapterListModel *adapterModel,
    RouteTableModel *routeTableModel,
    QObject *parent)
    : QObject(parent)
    , m_settingsRepository(settingsRepository)
    , m_networkService(networkService)
    , m_startupService(startupService)
    , m_adapterModel(adapterModel)
    , m_routeTableModel(routeTableModel)
    , m_selectedAdapterIndex(-1)
{
    m_persistedSettings = m_settingsRepository->load();
    m_workingSettings = m_persistedSettings;
    setLastActionMessage(QStringLiteral("设置已加载。"));
}

QObject *SettingsViewModel::adapterModel() const
{
    return m_adapterModel;
}

QObject *SettingsViewModel::routeTableModel() const
{
    return m_routeTableModel;
}

int SettingsViewModel::selectedAdapterIndex() const
{
    return m_selectedAdapterIndex;
}

QString SettingsViewModel::routeCidrsText() const
{
    return m_workingSettings.routeCidrsText;
}

QString SettingsViewModel::openVpnExePath() const
{
    return m_workingSettings.openVpnExePath;
}

QString SettingsViewModel::openVpnConfigPath() const
{
    return m_workingSettings.openVpnConfigPath;
}

QString SettingsViewModel::openVpnUsername() const
{
    return m_workingSettings.openVpnUsername;
}

QString SettingsViewModel::openVpnPassword() const
{
    return m_workingSettings.openVpnPassword;
}

QString SettingsViewModel::openVpnExtraArgs() const
{
    return m_workingSettings.openVpnExtraArgs;
}

bool SettingsViewModel::autoStart() const
{
    return m_workingSettings.autoStart;
}

bool SettingsViewModel::startMinimized() const
{
    return m_workingSettings.startMinimized;
}

bool SettingsViewModel::restoreLastMode() const
{
    return m_workingSettings.restoreLastMode;
}

int SettingsViewModel::operationTimeoutMs() const
{
    return m_workingSettings.operationTimeoutMs;
}

bool SettingsViewModel::hasUnsavedChanges() const
{
    return m_workingSettings != m_persistedSettings;
}

QString SettingsViewModel::lastActionMessage() const
{
    return m_lastActionMessage;
}

void SettingsViewModel::beginEdit()
{
    m_persistedSettings = m_settingsRepository->load();
    m_persistedSettings.autoStart = m_startupService->isAutoStartEnabled();
    m_workingSettings = m_persistedSettings;
    refreshAdapters();
    refreshRoutes();
    updateSelectionFromAdapterId();
    emitAllSettingSignals();
    setLastActionMessage(QStringLiteral("已载入最新设置，当前修改仅在保存后生效。"));
}

void SettingsViewModel::cancel()
{
    const bool previousDirty = hasUnsavedChanges();
    m_workingSettings = m_persistedSettings;
    updateSelectionFromAdapterId();
    emitAllSettingSignals();
    emitDirtyIfNeeded(previousDirty);
    setLastActionMessage(QStringLiteral("已取消未保存修改。"));
}

void SettingsViewModel::save()
{
    const bool previousDirty = hasUnsavedChanges();
    updateAdapterIdFromSelection();

    if (!m_startupService->syncAutoStart(
            m_workingSettings.autoStart,
            QCoreApplication::applicationFilePath(),
            m_workingSettings.startMinimized)) {
        setLastActionMessage(QStringLiteral("开机自启同步失败，请查看日志。"));
        logger.error(QStringLiteral("开机自启同步失败。"));
        return;
    }

    m_settingsRepository->save(m_workingSettings);
    m_persistedSettings = m_workingSettings;
    emitDirtyIfNeeded(previousDirty);
    setLastActionMessage(QStringLiteral("设置已保存。"));
    logger.info(QStringLiteral("设置已保存。"));
}

void SettingsViewModel::refreshAdapters()
{
    m_adapterModel->setAdapters(m_networkService->queryAdapters());
    updateSelectionFromAdapterId();
    emit selectedAdapterIndexChanged();
    setLastActionMessage(QStringLiteral("网卡列表已刷新。"));
}

void SettingsViewModel::refreshRoutes()
{
    m_routeTableModel->setRoutes(m_networkService->queryRoutes());
    setLastActionMessage(QStringLiteral("路由表已刷新。"));
}

bool SettingsViewModel::clearAllRoutes()
{
    const bool success = m_networkService->clearAllRoutes();
    refreshRoutes();
    setLastActionMessage(success
        ? QStringLiteral("已执行清空全部路由。")
        : QStringLiteral("清空全部路由失败，请查看日志。"));
    return success;
}

bool SettingsViewModel::openHostsLocation()
{
    const bool success = m_networkService->openHostsInExplorer();
    setLastActionMessage(success
        ? QStringLiteral("已定位 Hosts 文件。")
        : QStringLiteral("定位 Hosts 文件失败，请查看日志。"));
    return success;
}

void SettingsViewModel::setSelectedAdapterIndex(int index)
{
    if (m_selectedAdapterIndex == index) {
        return;
    }

    const bool previousDirty = hasUnsavedChanges();
    m_selectedAdapterIndex = index;
    updateAdapterIdFromSelection();
    emit selectedAdapterIndexChanged();
    emitDirtyIfNeeded(previousDirty);
}

void SettingsViewModel::setRouteCidrsText(const QString &text)
{
    if (m_workingSettings.routeCidrsText == text) {
        return;
    }

    const bool previousDirty = hasUnsavedChanges();
    m_workingSettings.routeCidrsText = text;
    emit routeCidrsTextChanged();
    emitDirtyIfNeeded(previousDirty);
}

void SettingsViewModel::setOpenVpnExePath(const QString &path)
{
    if (m_workingSettings.openVpnExePath == path) {
        return;
    }

    const bool previousDirty = hasUnsavedChanges();
    m_workingSettings.openVpnExePath = path;
    emit openVpnExePathChanged();
    emitDirtyIfNeeded(previousDirty);
}

void SettingsViewModel::setOpenVpnConfigPath(const QString &path)
{
    if (m_workingSettings.openVpnConfigPath == path) {
        return;
    }

    const bool previousDirty = hasUnsavedChanges();
    m_workingSettings.openVpnConfigPath = path;
    emit openVpnConfigPathChanged();
    emitDirtyIfNeeded(previousDirty);
}

void SettingsViewModel::setOpenVpnUsername(const QString &username)
{
    if (m_workingSettings.openVpnUsername == username) {
        return;
    }

    const bool previousDirty = hasUnsavedChanges();
    m_workingSettings.openVpnUsername = username;
    emit openVpnUsernameChanged();
    emitDirtyIfNeeded(previousDirty);
}

void SettingsViewModel::setOpenVpnPassword(const QString &password)
{
    if (m_workingSettings.openVpnPassword == password) {
        return;
    }

    const bool previousDirty = hasUnsavedChanges();
    m_workingSettings.openVpnPassword = password;
    emit openVpnPasswordChanged();
    emitDirtyIfNeeded(previousDirty);
}

void SettingsViewModel::setOpenVpnExtraArgs(const QString &args)
{
    if (m_workingSettings.openVpnExtraArgs == args) {
        return;
    }

    const bool previousDirty = hasUnsavedChanges();
    m_workingSettings.openVpnExtraArgs = args;
    emit openVpnExtraArgsChanged();
    emitDirtyIfNeeded(previousDirty);
}

void SettingsViewModel::setAutoStart(bool enabled)
{
    if (m_workingSettings.autoStart == enabled) {
        return;
    }

    const bool previousDirty = hasUnsavedChanges();
    m_workingSettings.autoStart = enabled;
    emit autoStartChanged();
    emitDirtyIfNeeded(previousDirty);
}

void SettingsViewModel::setStartMinimized(bool enabled)
{
    if (m_workingSettings.startMinimized == enabled) {
        return;
    }

    const bool previousDirty = hasUnsavedChanges();
    m_workingSettings.startMinimized = enabled;
    emit startMinimizedChanged();
    emitDirtyIfNeeded(previousDirty);
}

void SettingsViewModel::setRestoreLastMode(bool enabled)
{
    if (m_workingSettings.restoreLastMode == enabled) {
        return;
    }

    const bool previousDirty = hasUnsavedChanges();
    m_workingSettings.restoreLastMode = enabled;
    emit restoreLastModeChanged();
    emitDirtyIfNeeded(previousDirty);
}

void SettingsViewModel::setOperationTimeoutMs(int timeoutMs)
{
    if (m_workingSettings.operationTimeoutMs == timeoutMs) {
        return;
    }

    const bool previousDirty = hasUnsavedChanges();
    m_workingSettings.operationTimeoutMs = timeoutMs;
    emit operationTimeoutMsChanged();
    emitDirtyIfNeeded(previousDirty);
}

void SettingsViewModel::updateSelectionFromAdapterId()
{
    m_selectedAdapterIndex = m_adapterModel->indexOfAdapterId(m_workingSettings.localLanAdapterId);
    if (m_selectedAdapterIndex >= 0) {
        updateAdapterIdFromSelection();
    }
}

void SettingsViewModel::updateAdapterIdFromSelection()
{
    m_workingSettings.localLanAdapterId = m_adapterModel->adapterIdAt(m_selectedAdapterIndex);
}

void SettingsViewModel::emitAllSettingSignals()
{
    emit selectedAdapterIndexChanged();
    emit routeCidrsTextChanged();
    emit openVpnExePathChanged();
    emit openVpnConfigPathChanged();
    emit openVpnUsernameChanged();
    emit openVpnPasswordChanged();
    emit openVpnExtraArgsChanged();
    emit autoStartChanged();
    emit startMinimizedChanged();
    emit restoreLastModeChanged();
    emit operationTimeoutMsChanged();
    emit hasUnsavedChangesChanged();
}

void SettingsViewModel::setLastActionMessage(const QString &message)
{
    if (m_lastActionMessage == message) {
        return;
    }

    m_lastActionMessage = message;
    emit lastActionMessageChanged();
}

void SettingsViewModel::emitDirtyIfNeeded(bool previousDirty)
{
    if (previousDirty != hasUnsavedChanges()) {
        emit hasUnsavedChangesChanged();
    }
}

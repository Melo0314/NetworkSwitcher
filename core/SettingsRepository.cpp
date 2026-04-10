#include "SettingsRepository.h"

#include <QSettings>

namespace {

const char kGroup[] = "settings";

} // namespace

AppSettings SettingsRepository::load() const
{
    QSettings settings;
    AppSettings appSettings;

    settings.beginGroup(QString::fromLatin1(kGroup));
    appSettings.localLanAdapterId = settings.value(QStringLiteral("localLanAdapterId")).toString();
    appSettings.routeCidrsText = settings.value(QStringLiteral("routeCidrsText")).toString();
    appSettings.openVpnExePath = settings.value(QStringLiteral("openVpnExePath")).toString();
    appSettings.openVpnConfigPath = settings.value(QStringLiteral("openVpnConfigPath")).toString();
    appSettings.openVpnUsername = settings.value(QStringLiteral("openVpnUsername")).toString();
    appSettings.openVpnPassword = settings.value(QStringLiteral("openVpnPassword")).toString();
    appSettings.openVpnExtraArgs = settings.value(QStringLiteral("openVpnExtraArgs")).toString();
    appSettings.autoStart = settings.value(QStringLiteral("autoStart"), false).toBool();
    appSettings.startMinimized = settings.value(QStringLiteral("startMinimized"), false).toBool();
    appSettings.restoreLastMode = settings.value(QStringLiteral("restoreLastMode"), true).toBool();
    appSettings.lastMode = settings.value(QStringLiteral("lastMode")).toString();
    appSettings.operationTimeoutMs = settings.value(QStringLiteral("operationTimeoutMs"), 30000).toInt();
    settings.endGroup();

    return appSettings;
}

void SettingsRepository::save(const AppSettings &appSettings) const
{
    QSettings settings;

    settings.beginGroup(QString::fromLatin1(kGroup));
    settings.setValue(QStringLiteral("localLanAdapterId"), appSettings.localLanAdapterId);
    settings.setValue(QStringLiteral("routeCidrsText"), appSettings.routeCidrsText);
    settings.setValue(QStringLiteral("openVpnExePath"), appSettings.openVpnExePath);
    settings.setValue(QStringLiteral("openVpnConfigPath"), appSettings.openVpnConfigPath);
    settings.setValue(QStringLiteral("openVpnUsername"), appSettings.openVpnUsername);
    settings.setValue(QStringLiteral("openVpnPassword"), appSettings.openVpnPassword);
    settings.setValue(QStringLiteral("openVpnExtraArgs"), appSettings.openVpnExtraArgs);
    settings.setValue(QStringLiteral("autoStart"), appSettings.autoStart);
    settings.setValue(QStringLiteral("startMinimized"), appSettings.startMinimized);
    settings.setValue(QStringLiteral("restoreLastMode"), appSettings.restoreLastMode);
    settings.setValue(QStringLiteral("lastMode"), appSettings.lastMode);
    settings.setValue(QStringLiteral("operationTimeoutMs"), appSettings.operationTimeoutMs);
    settings.endGroup();
    settings.sync();
}

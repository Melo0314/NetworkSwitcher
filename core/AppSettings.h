#pragma once

#include <QString>

struct AppSettings {
    QString localLanAdapterId;
    QString routeCidrsText;
    QString openVpnExePath;
    QString openVpnConfigPath;
    QString openVpnUsername;
    QString openVpnPassword;
    QString openVpnExtraArgs;
    bool autoStart = false;
    bool startMinimized = false;
    bool restoreLastMode = true;
    QString lastMode;
    int operationTimeoutMs = 5000;
};

inline bool operator==(const AppSettings &lhs, const AppSettings &rhs)
{
    return lhs.localLanAdapterId == rhs.localLanAdapterId
           && lhs.routeCidrsText == rhs.routeCidrsText
           && lhs.openVpnExePath == rhs.openVpnExePath
           && lhs.openVpnConfigPath == rhs.openVpnConfigPath
           && lhs.openVpnUsername == rhs.openVpnUsername
           && lhs.openVpnPassword == rhs.openVpnPassword
           && lhs.openVpnExtraArgs == rhs.openVpnExtraArgs
           && lhs.autoStart == rhs.autoStart
           && lhs.startMinimized == rhs.startMinimized
           && lhs.restoreLastMode == rhs.restoreLastMode
           && lhs.lastMode == rhs.lastMode
           && lhs.operationTimeoutMs == rhs.operationTimeoutMs;
}

inline bool operator!=(const AppSettings &lhs, const AppSettings &rhs)
{
    return !(lhs == rhs);
}

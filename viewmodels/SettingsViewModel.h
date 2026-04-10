#pragma once

#include "../core/AppSettings.h"

#include <QObject>

class AdapterListModel;
class NetworkService;
class RouteTableModel;
class SettingsRepository;
class StartupService;

class SettingsViewModel : public QObject
{
    Q_OBJECT
    Q_PROPERTY(QObject *adapterModel READ adapterModel CONSTANT)
    Q_PROPERTY(QObject *routeTableModel READ routeTableModel CONSTANT)
    Q_PROPERTY(int selectedAdapterIndex READ selectedAdapterIndex WRITE setSelectedAdapterIndex NOTIFY selectedAdapterIndexChanged)
    Q_PROPERTY(QString routeCidrsText READ routeCidrsText WRITE setRouteCidrsText NOTIFY routeCidrsTextChanged)
    Q_PROPERTY(QString openVpnExePath READ openVpnExePath WRITE setOpenVpnExePath NOTIFY openVpnExePathChanged)
    Q_PROPERTY(QString openVpnConfigPath READ openVpnConfigPath WRITE setOpenVpnConfigPath NOTIFY openVpnConfigPathChanged)
    Q_PROPERTY(QString openVpnUsername READ openVpnUsername WRITE setOpenVpnUsername NOTIFY openVpnUsernameChanged)
    Q_PROPERTY(QString openVpnPassword READ openVpnPassword WRITE setOpenVpnPassword NOTIFY openVpnPasswordChanged)
    Q_PROPERTY(QString openVpnExtraArgs READ openVpnExtraArgs WRITE setOpenVpnExtraArgs NOTIFY openVpnExtraArgsChanged)
    Q_PROPERTY(bool autoStart READ autoStart WRITE setAutoStart NOTIFY autoStartChanged)
    Q_PROPERTY(bool startMinimized READ startMinimized WRITE setStartMinimized NOTIFY startMinimizedChanged)
    Q_PROPERTY(bool restoreLastMode READ restoreLastMode WRITE setRestoreLastMode NOTIFY restoreLastModeChanged)
    Q_PROPERTY(int operationTimeoutMs READ operationTimeoutMs WRITE setOperationTimeoutMs NOTIFY operationTimeoutMsChanged)
    Q_PROPERTY(bool hasUnsavedChanges READ hasUnsavedChanges NOTIFY hasUnsavedChangesChanged)
    Q_PROPERTY(QString lastActionMessage READ lastActionMessage NOTIFY lastActionMessageChanged)

public:
    explicit SettingsViewModel(
        SettingsRepository *settingsRepository,
        NetworkService *networkService,
        StartupService *startupService,
        AdapterListModel *adapterModel,
        RouteTableModel *routeTableModel,
        QObject *parent = nullptr);

    QObject *adapterModel() const;
    QObject *routeTableModel() const;

    int selectedAdapterIndex() const;
    QString routeCidrsText() const;
    QString openVpnExePath() const;
    QString openVpnConfigPath() const;
    QString openVpnUsername() const;
    QString openVpnPassword() const;
    QString openVpnExtraArgs() const;
    bool autoStart() const;
    bool startMinimized() const;
    bool restoreLastMode() const;
    int operationTimeoutMs() const;
    bool hasUnsavedChanges() const;
    QString lastActionMessage() const;

    Q_INVOKABLE void beginEdit();
    Q_INVOKABLE void cancel();
    Q_INVOKABLE void save();
    Q_INVOKABLE void refreshAdapters();
    Q_INVOKABLE void refreshRoutes();
    Q_INVOKABLE bool clearAllRoutes();
    Q_INVOKABLE bool openHostsLocation();

public slots:
    void setSelectedAdapterIndex(int index);
    void setRouteCidrsText(const QString &text);
    void setOpenVpnExePath(const QString &path);
    void setOpenVpnConfigPath(const QString &path);
    void setOpenVpnUsername(const QString &username);
    void setOpenVpnPassword(const QString &password);
    void setOpenVpnExtraArgs(const QString &args);
    void setAutoStart(bool enabled);
    void setStartMinimized(bool enabled);
    void setRestoreLastMode(bool enabled);
    void setOperationTimeoutMs(int timeoutMs);

signals:
    void selectedAdapterIndexChanged();
    void routeCidrsTextChanged();
    void openVpnExePathChanged();
    void openVpnConfigPathChanged();
    void openVpnUsernameChanged();
    void openVpnPasswordChanged();
    void openVpnExtraArgsChanged();
    void autoStartChanged();
    void startMinimizedChanged();
    void restoreLastModeChanged();
    void operationTimeoutMsChanged();
    void hasUnsavedChangesChanged();
    void lastActionMessageChanged();

private:
    void updateSelectionFromAdapterId();
    void updateAdapterIdFromSelection();
    void emitAllSettingSignals();
    void setLastActionMessage(const QString &message);
    void emitDirtyIfNeeded(bool previousDirty);

    SettingsRepository *m_settingsRepository;
    NetworkService *m_networkService;
    StartupService *m_startupService;
    AdapterListModel *m_adapterModel;
    RouteTableModel *m_routeTableModel;
    AppSettings m_persistedSettings;
    AppSettings m_workingSettings;
    int m_selectedAdapterIndex;
    QString m_lastActionMessage;
};

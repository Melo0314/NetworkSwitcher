#pragma once

#include <QObject>

class ProcessService;

class StartupService : public QObject
{
    Q_OBJECT

public:
    explicit StartupService(ProcessService *processService, QObject *parent = nullptr);

    bool isAutoStartEnabled() const;
    bool syncAutoStart(bool enabled, const QString &exePath, bool startMinimized);

private:
    QString currentUserName() const;

    ProcessService *m_processService;
};

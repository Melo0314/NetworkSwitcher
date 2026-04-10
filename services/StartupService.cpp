#include "StartupService.h"

#include "LoggerService.h"
#include "ProcessService.h"

#include <QCoreApplication>
#include <QDir>
#include <QProcessEnvironment>

namespace {

const char kTaskName[] = "NetworkSwitcher";

QString quotedTaskName()
{
    return QStringLiteral("\\%1").arg(QString::fromLatin1(kTaskName));
}

DECL_LOGGER(logger, "Startup");

} // namespace

StartupService::StartupService(ProcessService *processService, QObject *parent)
    : QObject(parent)
    , m_processService(processService)
{
}

bool StartupService::isAutoStartEnabled() const
{
    const CommandResult result = m_processService->runProcess(
        QStringLiteral("schtasks.exe"),
        { QStringLiteral("/Query"), QStringLiteral("/TN"), quotedTaskName() },
        QStringLiteral("Startup"),
        10000);

    return result.success;
}

bool StartupService::syncAutoStart(bool enabled, const QString &exePath, bool startMinimized)
{
    if (!enabled) {
        const CommandResult result = m_processService->runProcess(
            QStringLiteral("schtasks.exe"),
            { QStringLiteral("/Delete"), QStringLiteral("/TN"), quotedTaskName(), QStringLiteral("/F") },
            QStringLiteral("Startup"),
            15000);

        if (result.success) {
            logger.info(QStringLiteral("已删除开机自启任务。"));
            return true;
        }

        if (result.standardError.contains(QStringLiteral("cannot find"), Qt::CaseInsensitive)
            || result.standardError.contains(QStringLiteral("找不到"), Qt::CaseInsensitive)) {
            return true;
        }

        return false;
    }

    QString taskCommand = QDir::toNativeSeparators(exePath);
    taskCommand = QStringLiteral("\"%1\"").arg(taskCommand);
    if (startMinimized) {
        taskCommand += QStringLiteral(" --minimized");
    }

    QStringList arguments {
        QStringLiteral("/Create"),
        QStringLiteral("/TN"), quotedTaskName(),
        QStringLiteral("/SC"), QStringLiteral("ONLOGON"),
        QStringLiteral("/RL"), QStringLiteral("HIGHEST"),
        QStringLiteral("/F"),
        QStringLiteral("/IT"),
        QStringLiteral("/TR"), taskCommand
    };

    const QString userName = currentUserName();
    if (!userName.isEmpty()) {
        arguments << QStringLiteral("/RU") << userName;
    }

    const CommandResult result = m_processService->runProcess(
        QStringLiteral("schtasks.exe"),
        arguments,
        QStringLiteral("Startup"),
        15000);

    if (result.success) {
        logger.info(QStringLiteral("已创建或更新开机自启任务。"));
    }

    return result.success;
}

QString StartupService::currentUserName() const
{
    const QProcessEnvironment env = QProcessEnvironment::systemEnvironment();
    const QString userDomain = env.value(QStringLiteral("USERDOMAIN"));
    const QString userName = env.value(QStringLiteral("USERNAME"));
    if (userName.isEmpty()) {
        return {};
    }

    if (userDomain.isEmpty()) {
        return userName;
    }

    return QStringLiteral("%1\\%2").arg(userDomain, userName);
}

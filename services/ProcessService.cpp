#include "ProcessService.h"

#include "LoggerService.h"

#include <QProcess>

ProcessService::ProcessService(QObject *parent)
    : QObject(parent)
{
}

CommandResult ProcessService::runProcess(const QString &program, const QStringList &arguments, const QString &source, int timeoutMs)
{
    CommandResult result;
    QProcess process;
    const Logger logger(source);

    logger.command(QStringLiteral("%1 %2").arg(program, arguments.join(QStringLiteral(" "))));

    process.start(program, arguments);
    if (!process.waitForStarted(timeoutMs)) {
        result.standardError = QStringLiteral("进程启动失败。");
        logger.error(result.standardError);
        return result;
    }

    if (!process.waitForFinished(timeoutMs)) {
        process.kill();
        process.waitForFinished();
        result.standardError = QStringLiteral("进程执行超时，已终止。");
        logger.error(result.standardError);
        return result;
    }

    result.exitCode = process.exitCode();
    result.standardOutput = QString::fromLocal8Bit(process.readAllStandardOutput());
    result.standardError = QString::fromLocal8Bit(process.readAllStandardError());
    result.success = process.exitStatus() == QProcess::NormalExit && result.exitCode == 0;

    if (!result.standardOutput.trimmed().isEmpty()) {
        logger.output(result.standardOutput.trimmed());
    }

    if (!result.standardError.trimmed().isEmpty()) {
        logger.output(result.standardError.trimmed());
    }

    if (!result.success) {
        logger.error(QStringLiteral("命令执行失败，退出码 %1。").arg(result.exitCode));
    }

    return result;
}

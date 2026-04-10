#pragma once

#include <QObject>
#include <QString>
#include <QStringList>

struct CommandResult
{
    int exitCode = -1;
    QString standardOutput;
    QString standardError;
    bool success = false;
};

class ProcessService : public QObject
{
    Q_OBJECT

public:
    explicit ProcessService(QObject *parent = nullptr);

    CommandResult runProcess(const QString &program, const QStringList &arguments, const QString &source, int timeoutMs = 30000);
};

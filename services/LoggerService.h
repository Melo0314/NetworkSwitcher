#pragma once

#include <QAbstractItemModel>
#include <QAbstractListModel>
#include <QDebug>
#include <QHash>
#include <QMetaType>
#include <QObject>
#include <QString>
#include <QVariant>
#include <QVector>

#include <memory>

class QFile;
class QTextStream;
class QThread;

class LoggerEnums
{
    Q_GADGET

public:
    enum class LogLevel {
        Info,
        Warn,
        Error,
        Command,
        Output,
        Debug
    };
    Q_ENUM(LogLevel)
};

using LogLevel = LoggerEnums::LogLevel;

struct LogEntry
{
    QString timestamp;
    LogLevel level = LogLevel::Info;
    QString source;
    QString message;
    QString text;
};

Q_DECLARE_METATYPE(LogLevel)
Q_DECLARE_METATYPE(LogEntry)

class LogListModel : public QAbstractListModel
{
    Q_OBJECT

public:
    enum Role {
        TimestampRole = Qt::UserRole + 1,
        LevelRole,
        SourceRole,
        MessageRole,
        TextRole
    };

    explicit LogListModel(QObject *parent = nullptr);

    int rowCount(const QModelIndex &parent = QModelIndex()) const override;
    QVariant data(const QModelIndex &index, int role = Qt::DisplayRole) const override;
    QHash<int, QByteArray> roleNames() const override;

    void clear();
    void appendEntry(const LogEntry &entry);

private:
    QVector<LogEntry> m_entries;
};

class LoggerWorker : public QObject
{
    Q_OBJECT

public:
    explicit LoggerWorker(QObject *parent = nullptr);
    ~LoggerWorker() override;

public slots:
    void initialize(const QString &filePath);
    void append(LogLevel level, const QString &source, const QString &message);
    void shutdown();

signals:
    void entryCommitted(const LogEntry &entry);

private:
    void closeFile();

    QString m_logFilePath;
    QFile *m_file;
    QTextStream *m_stream;
    bool m_fileReady;
};

class LoggerService : public QObject
{
    Q_OBJECT
    Q_PROPERTY(QAbstractItemModel *logsModel READ logsModel CONSTANT)

public:
    static void initializeSingleton(const QString &filePath, QObject *parent = nullptr);
    static LoggerService &instance();
    static void shutdownSingleton();

    explicit LoggerService(QObject *parent = nullptr);
    ~LoggerService() override;

    void log(LogLevel level, const QString &source, const QString &message);

    QAbstractItemModel *logsModel();
    QString logFilePath() const;

private slots:
    void handleEntryCommitted(const LogEntry &entry);

private:
    void initialize(const QString &filePath);
    void shutdownWorkerThread();

    QString m_logFilePath;
    LogListModel m_logsModel;
    LoggerWorker *m_worker;
    QThread *m_workerThread;
};

struct LogStreamData
{
    LogStreamData(LogLevel level, const QString &source);

    QString source;
    LogLevel level;
    QString buffer;
    QDebug debug;
    bool flushed;
};

class LogStream
{
public:
    LogStream(LogLevel level, const QString &source);
    ~LogStream();

    LogStream(LogStream &&other) noexcept;
    LogStream &operator=(LogStream &&other) noexcept;

    template<typename T>
    LogStream &operator<<(const T &value)
    {
        if (m_data) {
            m_data->debug << value;
        }

        return *this;
    }

private:
    Q_DISABLE_COPY(LogStream)

    void flush();

    std::unique_ptr<LogStreamData> m_data;
};

class Logger
{
public:
    explicit Logger(const QString &source)
        : m_source(source)
    {
    }

    const Logger &operator()() const
    {
        return *this;
    }

    LogStream info() const
    {
        return stream(LogLevel::Info);
    }

    LogStream warn() const
    {
        return stream(LogLevel::Warn);
    }

    LogStream error() const
    {
        return stream(LogLevel::Error);
    }

    LogStream command() const
    {
        return stream(LogLevel::Command);
    }

    LogStream output() const
    {
        return stream(LogLevel::Output);
    }

    LogStream debug() const
    {
        return stream(LogLevel::Debug);
    }

    void info(const QString &message) const
    {
        logDirect(LogLevel::Info, message);
    }

    void warn(const QString &message) const
    {
        logDirect(LogLevel::Warn, message);
    }

    void error(const QString &message) const
    {
        logDirect(LogLevel::Error, message);
    }

    void command(const QString &message) const
    {
        logDirect(LogLevel::Command, message);
    }

    void output(const QString &message) const
    {
        logDirect(LogLevel::Output, message);
    }

    void debug(const QString &message) const
    {
        logDirect(LogLevel::Debug, message);
    }

private:
    LogStream stream(LogLevel level) const
    {
        return LogStream(level, m_source);
    }

    void logDirect(LogLevel level, const QString &message) const
    {
        LoggerService::instance().log(level, m_source, message);
    }

    QString m_source;
};

#define DECL_LOGGER(name, source) static const Logger name(QStringLiteral(source))

#include "LoggerService.h"

#include <QDateTime>
#include <QDir>
#include <QFile>
#include <QFileInfo>
#include <QMetaObject>
#include <QTextStream>
#include <QThread>
#include <QVariant>

namespace {

LoggerService *s_loggerService = nullptr;

QString levelToString(LogLevel level)
{
    switch (level) {
    case LogLevel::Info:
        return QStringLiteral("INFO");
    case LogLevel::Warn:
        return QStringLiteral("WARN");
    case LogLevel::Error:
        return QStringLiteral("ERROR");
    case LogLevel::Command:
        return QStringLiteral("CMD");
    case LogLevel::Output:
        return QStringLiteral("OUT");
    case LogLevel::Debug:
        return QStringLiteral("DEBUG");
    }

    return QStringLiteral("INFO");
}

QString formatLine(const LogEntry &entry)
{
    return QStringLiteral("[%1][%2][%3] %4")
        .arg(entry.timestamp, levelToString(entry.level), entry.source, entry.message);
}

} // namespace

LogListModel::LogListModel(QObject *parent)
    : QAbstractListModel(parent)
{
}

int LogListModel::rowCount(const QModelIndex &parent) const
{
    return parent.isValid() ? 0 : m_entries.count();
}

QVariant LogListModel::data(const QModelIndex &index, int role) const
{
    if (!index.isValid() || index.row() < 0 || index.row() >= m_entries.count()) {
        return {};
    }

    const LogEntry &entry = m_entries.at(index.row());
    switch (role) {
    case TimestampRole:
        return entry.timestamp;
    case LevelRole:
        return levelToString(entry.level);
    case SourceRole:
        return entry.source;
    case MessageRole:
        return entry.message;
    case TextRole:
    case Qt::DisplayRole:
        return entry.text;
    default:
        return {};
    }
}

QHash<int, QByteArray> LogListModel::roleNames() const
{
    return {
        { TimestampRole, QByteArrayLiteral("timestamp") },
        { LevelRole, QByteArrayLiteral("level") },
        { SourceRole, QByteArrayLiteral("source") },
        { MessageRole, QByteArrayLiteral("message") },
        { TextRole, QByteArrayLiteral("text") }
    };
}

void LogListModel::clear()
{
    if (m_entries.isEmpty()) {
        return;
    }

    beginResetModel();
    m_entries.clear();
    endResetModel();
}

void LogListModel::appendEntry(const LogEntry &entry)
{
    const int row = m_entries.count();
    beginInsertRows(QModelIndex(), row, row);
    m_entries.append(entry);
    endInsertRows();
}

LoggerWorker::LoggerWorker(QObject *parent)
    : QObject(parent)
    , m_file(nullptr)
    , m_stream(nullptr)
    , m_fileReady(false)
{
}

LoggerWorker::~LoggerWorker()
{
    closeFile();
}

void LoggerWorker::initialize(const QString &filePath)
{
    closeFile();

    m_logFilePath = QDir::toNativeSeparators(filePath);
    QFileInfo fileInfo(m_logFilePath);
    QDir().mkpath(fileInfo.absolutePath());

    QFile resetFile(m_logFilePath);
    if (resetFile.open(QIODevice::WriteOnly | QIODevice::Truncate | QIODevice::Text)) {
        resetFile.close();
    }

    m_file = new QFile(m_logFilePath);
    if (!m_file->open(QIODevice::WriteOnly | QIODevice::Append | QIODevice::Text)) {
        delete m_file;
        m_file = nullptr;
        m_fileReady = false;
        return;
    }

    m_stream = new QTextStream(m_file);
    m_stream->setCodec("UTF-8");
    m_fileReady = true;
}

void LoggerWorker::append(LogLevel level, const QString &source, const QString &message)
{
    LogEntry entry;
    entry.timestamp = QDateTime::currentDateTime().toString(QStringLiteral("yyyy-MM-dd HH:mm:ss"));
    entry.level = level;
    entry.source = source;
    entry.message = message;
    entry.text = formatLine(entry);

    if (m_fileReady && m_stream) {
        *m_stream << entry.text << '\n';
        m_stream->flush();
    }

    emit entryCommitted(entry);
}

void LoggerWorker::shutdown()
{
    closeFile();
}

void LoggerWorker::closeFile()
{
    m_fileReady = false;

    if (m_stream) {
        m_stream->flush();
        delete m_stream;
        m_stream = nullptr;
    }

    if (m_file) {
        if (m_file->isOpen()) {
            m_file->close();
        }

        delete m_file;
        m_file = nullptr;
    }
}

void LoggerService::initializeSingleton(const QString &filePath, QObject *parent)
{
    if (!s_loggerService) {
        s_loggerService = new LoggerService(parent);
    }

    s_loggerService->initialize(filePath);
}

LoggerService &LoggerService::instance()
{
    if (!s_loggerService) {
        qFatal("LoggerService singleton is not initialized.");
    }

    return *s_loggerService;
}

void LoggerService::shutdownSingleton()
{
    delete s_loggerService;
    s_loggerService = nullptr;
}

LoggerService::LoggerService(QObject *parent)
    : QObject(parent)
    , m_logsModel(this)
    , m_worker(nullptr)
    , m_workerThread(nullptr)
{
    qRegisterMetaType<LogLevel>("LogLevel");
    qRegisterMetaType<LogEntry>("LogEntry");
}

LoggerService::~LoggerService()
{
    shutdownWorkerThread();

    if (s_loggerService == this) {
        s_loggerService = nullptr;
    }
}

void LoggerService::log(LogLevel level, const QString &source, const QString &message)
{
    if (!m_worker) {
        return;
    }

    QMetaObject::invokeMethod(
        m_worker,
        "append",
        Qt::QueuedConnection,
        Q_ARG(LogLevel, level),
        Q_ARG(QString, source),
        Q_ARG(QString, message));
}

QAbstractItemModel *LoggerService::logsModel()
{
    return &m_logsModel;
}

QString LoggerService::logFilePath() const
{
    return m_logFilePath;
}

void LoggerService::handleEntryCommitted(const LogEntry &entry)
{
    m_logsModel.appendEntry(entry);
}

void LoggerService::initialize(const QString &filePath)
{
    m_logFilePath = QDir::toNativeSeparators(filePath);
    m_logsModel.clear();

    shutdownWorkerThread();

    m_workerThread = new QThread();
    m_worker = new LoggerWorker();
    m_worker->moveToThread(m_workerThread);
    connect(m_worker, &LoggerWorker::entryCommitted, this, &LoggerService::handleEntryCommitted);
    m_workerThread->start();

    QMetaObject::invokeMethod(
        m_worker,
        "initialize",
        Qt::BlockingQueuedConnection,
        Q_ARG(QString, m_logFilePath));
}

void LoggerService::shutdownWorkerThread()
{
    if (!m_workerThread || !m_worker) {
        delete m_workerThread;
        m_workerThread = nullptr;
        delete m_worker;
        m_worker = nullptr;
        return;
    }

    QMetaObject::invokeMethod(m_worker, "shutdown", Qt::BlockingQueuedConnection);
    m_workerThread->quit();
    m_workerThread->wait();

    delete m_worker;
    m_worker = nullptr;

    delete m_workerThread;
    m_workerThread = nullptr;
}

LogStreamData::LogStreamData(LogLevel streamLevel, const QString &streamSource)
    : source(streamSource)
    , level(streamLevel)
    , debug(&buffer)
    , flushed(false)
{
    debug.noquote();
}

LogStream::LogStream(LogLevel level, const QString &source)
    : m_data(new LogStreamData(level, source))
{
}

LogStream::~LogStream()
{
    flush();
}

LogStream::LogStream(LogStream &&other) noexcept
    : m_data(std::move(other.m_data))
{
}

LogStream &LogStream::operator=(LogStream &&other) noexcept
{
    if (this != &other) {
        flush();
        m_data = std::move(other.m_data);
    }

    return *this;
}

void LogStream::flush()
{
    if (!m_data || m_data->flushed) {
        return;
    }

    m_data->flushed = true;
    LoggerService::instance().log(m_data->level, m_data->source, m_data->buffer);
}

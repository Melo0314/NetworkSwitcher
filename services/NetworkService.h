#pragma once

#include "../core/NetworkTypes.h"

#include <QList>
#include <QObject>
#include <QStringList>

class ProcessService;

class NetworkService : public QObject
{
    Q_OBJECT

public:
    explicit NetworkService(ProcessService *processService, QObject *parent = nullptr);

    QList<AdapterInfo> queryAdapters() const;
    QList<RouteEntry> queryRoutes() const;
    AdapterInfo findAdapterById(const QString &adapterId) const;
    bool enableAdapter(const QString &adapterId);
    bool disableAdapter(const QString &adapterId);
    bool removeRoutes(const QStringList &cidrs);
    bool addRoutes(const QStringList &cidrs, const AdapterInfo &adapter);
    bool clearAllRoutes();
    bool openHostsInExplorer();

private:
    QString prefixLengthToSubnetMask(int prefixLength) const;
    bool setAdapterEnabled(const QString &adapterId, bool enabled);

    ProcessService *m_processService;
};

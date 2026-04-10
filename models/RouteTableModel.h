#pragma once

#include "../core/NetworkTypes.h"

#include <QAbstractListModel>
#include <QList>

class RouteTableModel : public QAbstractListModel
{
    Q_OBJECT

public:
    enum Roles {
        DestinationPrefixRole = Qt::UserRole + 1,
        NextHopRole,
        InterfaceAliasRole,
        RouteMetricRole,
        ProtocolRole,
        StateRole,
        PolicyStoreRole
    };

    explicit RouteTableModel(QObject *parent = nullptr);

    int rowCount(const QModelIndex &parent = QModelIndex()) const override;
    QVariant data(const QModelIndex &index, int role = Qt::DisplayRole) const override;
    QHash<int, QByteArray> roleNames() const override;

    void setRoutes(const QList<RouteEntry> &routes);

private:
    QList<RouteEntry> m_routes;
};

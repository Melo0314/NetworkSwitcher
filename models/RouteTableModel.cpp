#include "RouteTableModel.h"

RouteTableModel::RouteTableModel(QObject *parent)
    : QAbstractListModel(parent)
{
}

int RouteTableModel::rowCount(const QModelIndex &parent) const
{
    if (parent.isValid()) {
        return 0;
    }

    return m_routes.count();
}

QVariant RouteTableModel::data(const QModelIndex &index, int role) const
{
    if (!index.isValid() || index.row() < 0 || index.row() >= m_routes.count()) {
        return {};
    }

    const RouteEntry &route = m_routes.at(index.row());
    switch (role) {
    case DestinationPrefixRole:
    case Qt::DisplayRole:
        return route.destinationPrefix;
    case NextHopRole:
        return route.nextHop;
    case InterfaceAliasRole:
        return route.interfaceAlias;
    case RouteMetricRole:
        return route.routeMetric;
    case ProtocolRole:
        return route.protocol;
    case StateRole:
        return route.state;
    case PolicyStoreRole:
        return route.policyStore;
    default:
        return {};
    }
}

QHash<int, QByteArray> RouteTableModel::roleNames() const
{
    return {
        { DestinationPrefixRole, "destinationPrefix" },
        { NextHopRole, "nextHop" },
        { InterfaceAliasRole, "interfaceAlias" },
        { RouteMetricRole, "routeMetric" },
        { ProtocolRole, "protocol" },
        { StateRole, "state" },
        { PolicyStoreRole, "policyStore" }
    };
}

void RouteTableModel::setRoutes(const QList<RouteEntry> &routes)
{
    beginResetModel();
    m_routes = routes;
    endResetModel();
}

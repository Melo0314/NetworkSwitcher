#pragma once

#include <QString>

struct AdapterInfo
{
    QString id;
    QString name;
    QString description;
    QString status;
    QString ipv4;
    QString defaultGateway;
    int interfaceIndex = -1;
    int prefixLength = 0;
    QString subnetMask;
};

struct RouteEntry
{
    QString destinationPrefix;
    QString nextHop;
    QString interfaceAlias;
    int routeMetric = 0;
    QString protocol;
    QString state;
    QString policyStore;
};

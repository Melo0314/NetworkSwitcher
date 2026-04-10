#include "AdapterListModel.h"

namespace {

QString normalizeAdapterId(const QString &value)
{
    QString normalized = value.trimmed();
    if (normalized.startsWith(QLatin1Char('{')) && normalized.endsWith(QLatin1Char('}')) && normalized.size() > 2) {
        normalized = normalized.mid(1, normalized.size() - 2);
    }

    return normalized.toLower();
}

QString statusIconFor(const QString &status)
{
    return status.compare(QStringLiteral("Up"), Qt::CaseInsensitive) == 0
        ? QStringLiteral("qrc:/icons/plug-connected.svg")
        : QStringLiteral("qrc:/icons/plug-connected-x.svg");
}

QString summaryFor(const AdapterInfo &adapter)
{
    if (!adapter.name.isEmpty() && !adapter.ipv4.isEmpty()) {
        return QStringLiteral("%1 · %2").arg(adapter.name, adapter.ipv4);
    }

    if (!adapter.name.isEmpty()) {
        return adapter.name;
    }

    return QStringLiteral("未命名网卡");
}

} // namespace

AdapterListModel::AdapterListModel(QObject *parent)
    : QAbstractListModel(parent)
{
}

int AdapterListModel::rowCount(const QModelIndex &parent) const
{
    if (parent.isValid()) {
        return 0;
    }

    return m_adapters.count();
}

QVariant AdapterListModel::data(const QModelIndex &index, int role) const
{
    if (!index.isValid() || index.row() < 0 || index.row() >= m_adapters.count()) {
        return {};
    }

    const AdapterInfo &adapter = m_adapters.at(index.row());
    switch (role) {
    case IdRole:
        return adapter.id;
    case NameRole:
        return adapter.name;
    case DescriptionRole:
        return adapter.description;
    case StatusRole:
        return adapter.status;
    case StatusIconRole:
        return statusIconFor(adapter.status);
    case Ipv4Role:
        return adapter.ipv4;
    case PrefixLengthRole:
        return adapter.prefixLength;
    case SubnetMaskRole:
        return adapter.subnetMask;
    case SummaryRole:
    case Qt::DisplayRole:
        return summaryFor(adapter);
    default:
        return {};
    }
}

QHash<int, QByteArray> AdapterListModel::roleNames() const
{
    return {
        { IdRole, "adapterId" },
        { NameRole, "name" },
        { DescriptionRole, "description" },
        { StatusRole, "statusText" },
        { StatusIconRole, "statusIcon" },
        { Ipv4Role, "ipv4" },
        { PrefixLengthRole, "prefixLength" },
        { SubnetMaskRole, "subnetMask" },
        { SummaryRole, "summary" }
    };
}

void AdapterListModel::setAdapters(const QList<AdapterInfo> &adapters)
{
    beginResetModel();
    m_adapters = adapters;
    endResetModel();
}

int AdapterListModel::indexOfAdapterId(const QString &adapterId) const
{
    const QString targetId = normalizeAdapterId(adapterId);
    if (targetId.isEmpty()) {
        return -1;
    }

    for (int index = 0; index < m_adapters.count(); ++index) {
        if (normalizeAdapterId(m_adapters.at(index).id) == targetId) {
            return index;
        }
    }

    return -1;
}

QString AdapterListModel::adapterIdAt(int index) const
{
    if (index < 0 || index >= m_adapters.count()) {
        return {};
    }

    return m_adapters.at(index).id;
}

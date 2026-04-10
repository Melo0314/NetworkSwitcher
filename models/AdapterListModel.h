#pragma once

#include "../core/NetworkTypes.h"

#include <QAbstractListModel>
#include <QList>

class AdapterListModel : public QAbstractListModel
{
    Q_OBJECT

public:
    enum Roles {
        IdRole = Qt::UserRole + 1,
        NameRole,
        DescriptionRole,
        StatusRole,
        StatusIconRole,
        Ipv4Role,
        PrefixLengthRole,
        SubnetMaskRole,
        SummaryRole
    };

    explicit AdapterListModel(QObject *parent = nullptr);

    int rowCount(const QModelIndex &parent = QModelIndex()) const override;
    QVariant data(const QModelIndex &index, int role = Qt::DisplayRole) const override;
    QHash<int, QByteArray> roleNames() const override;

    void setAdapters(const QList<AdapterInfo> &adapters);

    Q_INVOKABLE int indexOfAdapterId(const QString &adapterId) const;
    Q_INVOKABLE QString adapterIdAt(int index) const;

private:
    QList<AdapterInfo> m_adapters;
};

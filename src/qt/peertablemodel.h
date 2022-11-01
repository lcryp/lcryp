#ifndef LCRYP_QT_PEERTABLEMODEL_H
#define LCRYP_QT_PEERTABLEMODEL_H
#include <net_processing.h>
#include <net.h>
#include <QAbstractTableModel>
#include <QList>
#include <QModelIndex>
#include <QStringList>
#include <QVariant>
class PeerTablePriv;
namespace interfaces {
class Node;
}
QT_BEGIN_NAMESPACE
class QTimer;
QT_END_NAMESPACE
struct CNodeCombinedStats {
    CNodeStats nodeStats;
    CNodeStateStats nodeStateStats;
    bool fNodeStateStatsAvailable;
};
Q_DECLARE_METATYPE(CNodeCombinedStats*)
class PeerTableModel : public QAbstractTableModel
{
    Q_OBJECT
public:
    explicit PeerTableModel(interfaces::Node& node, QObject* parent);
    ~PeerTableModel();
    void startAutoRefresh();
    void stopAutoRefresh();
    enum ColumnIndex {
        NetNodeId = 0,
        Age,
        Address,
        Direction,
        ConnectionType,
        Network,
        Ping,
        Sent,
        Received,
        Subversion
    };
    enum {
        StatsRole = Qt::UserRole,
    };
    int rowCount(const QModelIndex& parent = QModelIndex()) const override;
    int columnCount(const QModelIndex& parent = QModelIndex()) const override;
    QVariant data(const QModelIndex& index, int role = Qt::DisplayRole) const override;
    QVariant headerData(int section, Qt::Orientation orientation, int role = Qt::DisplayRole) const override;
    QModelIndex index(int row, int column, const QModelIndex& parent = QModelIndex()) const override;
    Qt::ItemFlags flags(const QModelIndex &index) const override;
public Q_SLOTS:
    void refresh();
private:
    QList<CNodeCombinedStats> m_peers_data{};
    interfaces::Node& m_node;
    const QStringList columns{
        tr("Peer"),
        tr("Age"),
        tr("Address"),
        tr("Direction"),
        tr("Type"),
        tr("Network"),
        tr("Ping"),
        tr("Sent"),
        tr("Received"),
        tr("User Agent")};
    QTimer *timer;
};
#endif

#ifndef LCRYP_QT_BANTABLEMODEL_H
#define LCRYP_QT_BANTABLEMODEL_H
#include <addrdb.h>
#include <net.h>
#include <memory>
#include <QAbstractTableModel>
#include <QStringList>
class BanTablePriv;
namespace interfaces {
    class Node;
}
struct CCombinedBan {
    CSubNet subnet;
    CBanEntry banEntry;
};
class BannedNodeLessThan
{
public:
    BannedNodeLessThan(int nColumn, Qt::SortOrder fOrder) :
        column(nColumn), order(fOrder) {}
    bool operator()(const CCombinedBan& left, const CCombinedBan& right) const;
private:
    int column;
    Qt::SortOrder order;
};
class BanTableModel : public QAbstractTableModel
{
    Q_OBJECT
public:
    explicit BanTableModel(interfaces::Node& node, QObject* parent);
    ~BanTableModel();
    void startAutoRefresh();
    void stopAutoRefresh();
    enum ColumnIndex {
        Address = 0,
        Bantime = 1
    };
    int rowCount(const QModelIndex &parent) const override;
    int columnCount(const QModelIndex &parent) const override;
    QVariant data(const QModelIndex &index, int role) const override;
    QVariant headerData(int section, Qt::Orientation orientation, int role) const override;
    QModelIndex index(int row, int column, const QModelIndex &parent) const override;
    Qt::ItemFlags flags(const QModelIndex &index) const override;
    void sort(int column, Qt::SortOrder order) override;
    bool shouldShow();
public Q_SLOTS:
    void refresh();
private:
    interfaces::Node& m_node;
    QStringList columns;
    std::unique_ptr<BanTablePriv> priv;
};
#endif

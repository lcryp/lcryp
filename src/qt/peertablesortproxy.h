#ifndef LCRYP_QT_PEERTABLESORTPROXY_H
#define LCRYP_QT_PEERTABLESORTPROXY_H
#include <QSortFilterProxyModel>
QT_BEGIN_NAMESPACE
class QModelIndex;
QT_END_NAMESPACE
class PeerTableSortProxy : public QSortFilterProxyModel
{
    Q_OBJECT
public:
    explicit PeerTableSortProxy(QObject* parent = nullptr);
protected:
    bool lessThan(const QModelIndex& left_index, const QModelIndex& right_index) const override;
};
#endif

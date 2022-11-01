#ifndef LCRYP_QT_TRANSACTIONFILTERPROXY_H
#define LCRYP_QT_TRANSACTIONFILTERPROXY_H
#include <consensus/amount.h>
#include <QDateTime>
#include <QSortFilterProxyModel>
#include <optional>
class TransactionFilterProxy : public QSortFilterProxyModel
{
    Q_OBJECT
public:
    explicit TransactionFilterProxy(QObject *parent = nullptr);
    static const quint32 ALL_TYPES = 0xFFFFFFFF;
    static quint32 TYPE(int type) { return 1<<type; }
    enum WatchOnlyFilter
    {
        WatchOnlyFilter_All,
        WatchOnlyFilter_Yes,
        WatchOnlyFilter_No
    };
    void setDateRange(const std::optional<QDateTime>& from, const std::optional<QDateTime>& to);
    void setSearchString(const QString &);
    void setTypeFilter(quint32 modes);
    void setMinAmount(const CAmount& minimum);
    void setWatchOnlyFilter(WatchOnlyFilter filter);
    void setLimit(int limit);
    void setShowInactive(bool showInactive);
    int rowCount(const QModelIndex &parent = QModelIndex()) const override;
protected:
    bool filterAcceptsRow(int source_row, const QModelIndex & source_parent) const override;
private:
    std::optional<QDateTime> dateFrom;
    std::optional<QDateTime> dateTo;
    QString m_search_string;
    quint32 typeFilter;
    WatchOnlyFilter watchOnlyFilter;
    CAmount minAmount;
    int limitRows;
    bool showInactive;
};
#endif

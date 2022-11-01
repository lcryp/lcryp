#ifndef LCRYP_QT_RECENTREQUESTSTABLEMODEL_H
#define LCRYP_QT_RECENTREQUESTSTABLEMODEL_H
#include <qt/sendcoinsrecipient.h>
#include <string>
#include <QAbstractTableModel>
#include <QStringList>
#include <QDateTime>
class WalletModel;
class RecentRequestEntry
{
public:
    RecentRequestEntry() : nVersion(RecentRequestEntry::CURRENT_VERSION), id(0) { }
    static const int CURRENT_VERSION = 1;
    int nVersion;
    int64_t id;
    QDateTime date;
    SendCoinsRecipient recipient;
    SERIALIZE_METHODS(RecentRequestEntry, obj) {
        unsigned int date_timet;
        SER_WRITE(obj, date_timet = obj.date.toSecsSinceEpoch());
        READWRITE(obj.nVersion, obj.id, date_timet, obj.recipient);
        SER_READ(obj, obj.date = QDateTime::fromSecsSinceEpoch(date_timet));
    }
};
class RecentRequestEntryLessThan
{
public:
    RecentRequestEntryLessThan(int nColumn, Qt::SortOrder fOrder):
        column(nColumn), order(fOrder) {}
    bool operator()(const RecentRequestEntry& left, const RecentRequestEntry& right) const;
private:
    int column;
    Qt::SortOrder order;
};
class RecentRequestsTableModel: public QAbstractTableModel
{
    Q_OBJECT
public:
    explicit RecentRequestsTableModel(WalletModel *parent);
    ~RecentRequestsTableModel();
    enum ColumnIndex {
        Date = 0,
        Label = 1,
        Message = 2,
        Amount = 3,
        NUMBER_OF_COLUMNS
    };
    int rowCount(const QModelIndex &parent) const override;
    int columnCount(const QModelIndex &parent) const override;
    QVariant data(const QModelIndex &index, int role) const override;
    bool setData(const QModelIndex &index, const QVariant &value, int role) override;
    QVariant headerData(int section, Qt::Orientation orientation, int role) const override;
    QModelIndex index(int row, int column, const QModelIndex &parent = QModelIndex()) const override;
    bool removeRows(int row, int count, const QModelIndex &parent = QModelIndex()) override;
    Qt::ItemFlags flags(const QModelIndex &index) const override;
    void sort(int column, Qt::SortOrder order = Qt::AscendingOrder) override;
    const RecentRequestEntry &entry(int row) const { return list[row]; }
    void addNewRequest(const SendCoinsRecipient &recipient);
    void addNewRequest(const std::string &recipient);
    void addNewRequest(RecentRequestEntry &recipient);
public Q_SLOTS:
    void updateDisplayUnit();
private:
    WalletModel *walletModel;
    QStringList columns;
    QList<RecentRequestEntry> list;
    int64_t nReceiveRequestsMaxId{0};
    void updateAmountColumnTitle();
    QString getAmountTitle();
};
#endif

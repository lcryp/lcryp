#ifndef LCRYP_QT_TRANSACTIONTABLEMODEL_H
#define LCRYP_QT_TRANSACTIONTABLEMODEL_H
#include <qt/lcrypunits.h>
#include <QAbstractTableModel>
#include <QStringList>
#include <memory>
namespace interfaces {
class Handler;
}
class PlatformStyle;
class TransactionRecord;
class TransactionTablePriv;
class WalletModel;
class TransactionTableModel : public QAbstractTableModel
{
    Q_OBJECT
public:
    explicit TransactionTableModel(const PlatformStyle *platformStyle, WalletModel *parent = nullptr);
    ~TransactionTableModel();
    enum ColumnIndex {
        Status = 0,
        Watchonly = 1,
        Date = 2,
        Type = 3,
        ToAddress = 4,
        Amount = 5
    };
    enum RoleIndex {
        TypeRole = Qt::UserRole,
        DateRole,
        WatchonlyRole,
        WatchonlyDecorationRole,
        LongDescriptionRole,
        AddressRole,
        LabelRole,
        AmountRole,
        TxHashRole,
        TxHexRole,
        TxPlainTextRole,
        ConfirmedRole,
        FormattedAmountRole,
        StatusRole,
        RawDecorationRole,
    };
    int rowCount(const QModelIndex &parent) const override;
    int columnCount(const QModelIndex &parent) const override;
    QVariant data(const QModelIndex &index, int role) const override;
    QVariant headerData(int section, Qt::Orientation orientation, int role) const override;
    QModelIndex index(int row, int column, const QModelIndex & parent = QModelIndex()) const override;
    bool processingQueuedTransactions() const { return fProcessingQueuedTransactions; }
private:
    WalletModel *walletModel;
    std::unique_ptr<interfaces::Handler> m_handler_transaction_changed;
    std::unique_ptr<interfaces::Handler> m_handler_show_progress;
    QStringList columns;
    TransactionTablePriv *priv;
    bool fProcessingQueuedTransactions;
    const PlatformStyle *platformStyle;
    void subscribeToCoreSignals();
    void unsubscribeFromCoreSignals();
    QString lookupAddress(const std::string &address, bool tooltip) const;
    QVariant addressColor(const TransactionRecord *wtx) const;
    QString formatTxStatus(const TransactionRecord *wtx) const;
    QString formatTxDate(const TransactionRecord *wtx) const;
    QString formatTxType(const TransactionRecord *wtx) const;
    QString formatTxToAddress(const TransactionRecord *wtx, bool tooltip) const;
    QString formatTxAmount(const TransactionRecord *wtx, bool showUnconfirmed=true, LcRypUnits::SeparatorStyle separators=LcRypUnits::SeparatorStyle::STANDARD) const;
    QString formatTooltip(const TransactionRecord *rec) const;
    QVariant txStatusDecoration(const TransactionRecord *wtx) const;
    QVariant txWatchonlyDecoration(const TransactionRecord *wtx) const;
    QVariant txAddressDecoration(const TransactionRecord *wtx) const;
public Q_SLOTS:
    void updateTransaction(const QString &hash, int status, bool showTransaction);
    void updateConfirmations();
    void updateDisplayUnit();
    void updateAmountColumnTitle();
    void setProcessingQueuedTransactions(bool value) { fProcessingQueuedTransactions = value; }
    friend class TransactionTablePriv;
};
#endif

#ifndef LCRYP_QT_ADDRESSTABLEMODEL_H
#define LCRYP_QT_ADDRESSTABLEMODEL_H
#include <QAbstractTableModel>
#include <QStringList>
enum class OutputType;
class AddressTablePriv;
class WalletModel;
namespace interfaces {
class Wallet;
}
class AddressTableModel : public QAbstractTableModel
{
    Q_OBJECT
public:
    explicit AddressTableModel(WalletModel *parent = nullptr, bool pk_hash_only = false);
    ~AddressTableModel();
    enum ColumnIndex {
        Label = 0,
        Address = 1
    };
    enum RoleIndex {
        TypeRole = Qt::UserRole
    };
    enum EditStatus {
        OK,
        NO_CHANGES,
        INVALID_ADDRESS,
        DUPLICATE_ADDRESS,
        WALLET_UNLOCK_FAILURE,
        KEY_GENERATION_FAILURE
    };
    static const QString Send;
    static const QString Receive;
    int rowCount(const QModelIndex &parent) const override;
    int columnCount(const QModelIndex &parent) const override;
    QVariant data(const QModelIndex &index, int role) const override;
    bool setData(const QModelIndex &index, const QVariant &value, int role) override;
    QVariant headerData(int section, Qt::Orientation orientation, int role) const override;
    QModelIndex index(int row, int column, const QModelIndex &parent) const override;
    bool removeRows(int row, int count, const QModelIndex &parent = QModelIndex()) override;
    Qt::ItemFlags flags(const QModelIndex &index) const override;
    QString addRow(const QString &type, const QString &label, const QString &address, const OutputType address_type);
    QString labelForAddress(const QString &address) const;
    QString purposeForAddress(const QString &address) const;
    int lookupAddress(const QString &address) const;
    EditStatus getEditStatus() const { return editStatus; }
    OutputType GetDefaultAddressType() const;
private:
    WalletModel* const walletModel;
    AddressTablePriv *priv = nullptr;
    QStringList columns;
    EditStatus editStatus = OK;
    bool getAddressData(const QString &address, std::string* name, std::string* purpose) const;
    void emitDataChanged(int index);
public Q_SLOTS:
    void updateEntry(const QString &address, const QString &label, bool isMine, const QString &purpose, int status);
    friend class AddressTablePriv;
};
#endif

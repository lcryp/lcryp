#include <qt/addresstablemodel.h>
#include <qt/guiutil.h>
#include <qt/walletmodel.h>
#include <key_io.h>
#include <wallet/wallet.h>
#include <algorithm>
#include <QFont>
#include <QDebug>
const QString AddressTableModel::Send = "S";
const QString AddressTableModel::Receive = "R";
struct AddressTableEntry
{
    enum Type {
        Sending,
        Receiving,
        Hidden
    };
    Type type;
    QString label;
    QString address;
    AddressTableEntry() = default;
    AddressTableEntry(Type _type, const QString &_label, const QString &_address):
        type(_type), label(_label), address(_address) {}
};
struct AddressTableEntryLessThan
{
    bool operator()(const AddressTableEntry &a, const AddressTableEntry &b) const
    {
        return a.address < b.address;
    }
    bool operator()(const AddressTableEntry &a, const QString &b) const
    {
        return a.address < b;
    }
    bool operator()(const QString &a, const AddressTableEntry &b) const
    {
        return a < b.address;
    }
};
static AddressTableEntry::Type translateTransactionType(const QString &strPurpose, bool isMine)
{
    AddressTableEntry::Type addressType = AddressTableEntry::Hidden;
    if (strPurpose == "send")
        addressType = AddressTableEntry::Sending;
    else if (strPurpose == "receive")
        addressType = AddressTableEntry::Receiving;
    else if (strPurpose == "unknown" || strPurpose == "")
        addressType = (isMine ? AddressTableEntry::Receiving : AddressTableEntry::Sending);
    return addressType;
}
class AddressTablePriv
{
public:
    QList<AddressTableEntry> cachedAddressTable;
    AddressTableModel *parent;
    explicit AddressTablePriv(AddressTableModel *_parent):
        parent(_parent) {}
    void refreshAddressTable(interfaces::Wallet& wallet, bool pk_hash_only = false)
    {
        cachedAddressTable.clear();
        {
            for (const auto& address : wallet.getAddresses())
            {
                if (pk_hash_only && !std::holds_alternative<PKHash>(address.dest)) {
                    continue;
                }
                AddressTableEntry::Type addressType = translateTransactionType(
                        QString::fromStdString(address.purpose), address.is_mine);
                cachedAddressTable.append(AddressTableEntry(addressType,
                                  QString::fromStdString(address.name),
                                  QString::fromStdString(EncodeDestination(address.dest))));
            }
        }
        std::sort(cachedAddressTable.begin(), cachedAddressTable.end(), AddressTableEntryLessThan());
    }
    void updateEntry(const QString &address, const QString &label, bool isMine, const QString &purpose, int status)
    {
        QList<AddressTableEntry>::iterator lower = std::lower_bound(
            cachedAddressTable.begin(), cachedAddressTable.end(), address, AddressTableEntryLessThan());
        QList<AddressTableEntry>::iterator upper = std::upper_bound(
            cachedAddressTable.begin(), cachedAddressTable.end(), address, AddressTableEntryLessThan());
        int lowerIndex = (lower - cachedAddressTable.begin());
        int upperIndex = (upper - cachedAddressTable.begin());
        bool inModel = (lower != upper);
        AddressTableEntry::Type newEntryType = translateTransactionType(purpose, isMine);
        switch(status)
        {
        case CT_NEW:
            if(inModel)
            {
                qWarning() << "AddressTablePriv::updateEntry: Warning: Got CT_NEW, but entry is already in model";
                break;
            }
            parent->beginInsertRows(QModelIndex(), lowerIndex, lowerIndex);
            cachedAddressTable.insert(lowerIndex, AddressTableEntry(newEntryType, label, address));
            parent->endInsertRows();
            break;
        case CT_UPDATED:
            if(!inModel)
            {
                qWarning() << "AddressTablePriv::updateEntry: Warning: Got CT_UPDATED, but entry is not in model";
                break;
            }
            lower->type = newEntryType;
            lower->label = label;
            parent->emitDataChanged(lowerIndex);
            break;
        case CT_DELETED:
            if(!inModel)
            {
                qWarning() << "AddressTablePriv::updateEntry: Warning: Got CT_DELETED, but entry is not in model";
                break;
            }
            parent->beginRemoveRows(QModelIndex(), lowerIndex, upperIndex-1);
            cachedAddressTable.erase(lower, upper);
            parent->endRemoveRows();
            break;
        }
    }
    int size()
    {
        return cachedAddressTable.size();
    }
    AddressTableEntry *index(int idx)
    {
        if(idx >= 0 && idx < cachedAddressTable.size())
        {
            return &cachedAddressTable[idx];
        }
        else
        {
            return nullptr;
        }
    }
};
AddressTableModel::AddressTableModel(WalletModel *parent, bool pk_hash_only) :
    QAbstractTableModel(parent), walletModel(parent)
{
    columns << tr("Label") << tr("Address");
    priv = new AddressTablePriv(this);
    priv->refreshAddressTable(parent->wallet(), pk_hash_only);
}
AddressTableModel::~AddressTableModel()
{
    delete priv;
}
int AddressTableModel::rowCount(const QModelIndex &parent) const
{
    if (parent.isValid()) {
        return 0;
    }
    return priv->size();
}
int AddressTableModel::columnCount(const QModelIndex &parent) const
{
    if (parent.isValid()) {
        return 0;
    }
    return columns.length();
}
QVariant AddressTableModel::data(const QModelIndex &index, int role) const
{
    if(!index.isValid())
        return QVariant();
    AddressTableEntry *rec = static_cast<AddressTableEntry*>(index.internalPointer());
    const auto column = static_cast<ColumnIndex>(index.column());
    if (role == Qt::DisplayRole || role == Qt::EditRole) {
        switch (column) {
        case Label:
            if (rec->label.isEmpty() && role == Qt::DisplayRole) {
                return tr("(no label)");
            } else {
                return rec->label;
            }
        case Address:
            return rec->address;
        }
        assert(false);
    } else if (role == Qt::FontRole) {
        switch (column) {
        case Label:
            return QFont();
        case Address:
            return GUIUtil::fixedPitchFont();
        }
        assert(false);
    } else if (role == TypeRole) {
        switch(rec->type)
        {
        case AddressTableEntry::Sending:
            return Send;
        case AddressTableEntry::Receiving:
            return Receive;
        case AddressTableEntry::Hidden:
            return {};
        }
        assert(false);
    }
    return QVariant();
}
bool AddressTableModel::setData(const QModelIndex &index, const QVariant &value, int role)
{
    if(!index.isValid())
        return false;
    AddressTableEntry *rec = static_cast<AddressTableEntry*>(index.internalPointer());
    std::string strPurpose = (rec->type == AddressTableEntry::Sending ? "send" : "receive");
    editStatus = OK;
    if(role == Qt::EditRole)
    {
        CTxDestination curAddress = DecodeDestination(rec->address.toStdString());
        if(index.column() == Label)
        {
            if(rec->label == value.toString())
            {
                editStatus = NO_CHANGES;
                return false;
            }
            walletModel->wallet().setAddressBook(curAddress, value.toString().toStdString(), strPurpose);
        } else if(index.column() == Address) {
            CTxDestination newAddress = DecodeDestination(value.toString().toStdString());
            if(std::get_if<CNoDestination>(&newAddress))
            {
                editStatus = INVALID_ADDRESS;
                return false;
            }
            else if(newAddress == curAddress)
            {
                editStatus = NO_CHANGES;
                return false;
            }
            if (walletModel->wallet().getAddress(
                    newAddress,   nullptr,   nullptr,   nullptr))
            {
                editStatus = DUPLICATE_ADDRESS;
                return false;
            }
            else if(rec->type == AddressTableEntry::Sending)
            {
                walletModel->wallet().delAddressBook(curAddress);
                walletModel->wallet().setAddressBook(newAddress, value.toString().toStdString(), strPurpose);
            }
        }
        return true;
    }
    return false;
}
QVariant AddressTableModel::headerData(int section, Qt::Orientation orientation, int role) const
{
    if(orientation == Qt::Horizontal)
    {
        if(role == Qt::DisplayRole && section < columns.size())
        {
            return columns[section];
        }
    }
    return QVariant();
}
Qt::ItemFlags AddressTableModel::flags(const QModelIndex &index) const
{
    if (!index.isValid()) return Qt::NoItemFlags;
    AddressTableEntry *rec = static_cast<AddressTableEntry*>(index.internalPointer());
    Qt::ItemFlags retval = Qt::ItemIsSelectable | Qt::ItemIsEnabled;
    if(rec->type == AddressTableEntry::Sending ||
      (rec->type == AddressTableEntry::Receiving && index.column()==Label))
    {
        retval |= Qt::ItemIsEditable;
    }
    return retval;
}
QModelIndex AddressTableModel::index(int row, int column, const QModelIndex &parent) const
{
    Q_UNUSED(parent);
    AddressTableEntry *data = priv->index(row);
    if(data)
    {
        return createIndex(row, column, priv->index(row));
    }
    else
    {
        return QModelIndex();
    }
}
void AddressTableModel::updateEntry(const QString &address,
        const QString &label, bool isMine, const QString &purpose, int status)
{
    priv->updateEntry(address, label, isMine, purpose, status);
}
QString AddressTableModel::addRow(const QString &type, const QString &label, const QString &address, const OutputType address_type)
{
    std::string strLabel = label.toStdString();
    std::string strAddress = address.toStdString();
    editStatus = OK;
    if(type == Send)
    {
        if(!walletModel->validateAddress(address))
        {
            editStatus = INVALID_ADDRESS;
            return QString();
        }
        {
            if (walletModel->wallet().getAddress(
                    DecodeDestination(strAddress),   nullptr,   nullptr,   nullptr))
            {
                editStatus = DUPLICATE_ADDRESS;
                return QString();
            }
        }
        walletModel->wallet().setAddressBook(DecodeDestination(strAddress), strLabel, "send");
    }
    else if(type == Receive)
    {
        auto op_dest = walletModel->wallet().getNewDestination(address_type, strLabel);
        if (!op_dest) {
            WalletModel::UnlockContext ctx(walletModel->requestUnlock());
            if (!ctx.isValid()) {
                editStatus = WALLET_UNLOCK_FAILURE;
                return QString();
            }
            op_dest = walletModel->wallet().getNewDestination(address_type, strLabel);
            if (!op_dest) {
                editStatus = KEY_GENERATION_FAILURE;
                return QString();
            }
        }
        strAddress = EncodeDestination(*op_dest);
    }
    else
    {
        return QString();
    }
    return QString::fromStdString(strAddress);
}
bool AddressTableModel::removeRows(int row, int count, const QModelIndex &parent)
{
    Q_UNUSED(parent);
    AddressTableEntry *rec = priv->index(row);
    if(count != 1 || !rec || rec->type == AddressTableEntry::Receiving)
    {
        return false;
    }
    walletModel->wallet().delAddressBook(DecodeDestination(rec->address.toStdString()));
    return true;
}
QString AddressTableModel::labelForAddress(const QString &address) const
{
    std::string name;
    if (getAddressData(address, &name,   nullptr)) {
        return QString::fromStdString(name);
    }
    return QString();
}
QString AddressTableModel::purposeForAddress(const QString &address) const
{
    std::string purpose;
    if (getAddressData(address,   nullptr, &purpose)) {
        return QString::fromStdString(purpose);
    }
    return QString();
}
bool AddressTableModel::getAddressData(const QString &address,
        std::string* name,
        std::string* purpose) const {
    CTxDestination destination = DecodeDestination(address.toStdString());
    return walletModel->wallet().getAddress(destination, name,   nullptr, purpose);
}
int AddressTableModel::lookupAddress(const QString &address) const
{
    QModelIndexList lst = match(index(0, Address, QModelIndex()),
                                Qt::EditRole, address, 1, Qt::MatchExactly);
    if(lst.isEmpty())
    {
        return -1;
    }
    else
    {
        return lst.at(0).row();
    }
}
OutputType AddressTableModel::GetDefaultAddressType() const { return walletModel->wallet().getDefaultAddressType(); };
void AddressTableModel::emitDataChanged(int idx)
{
    Q_EMIT dataChanged(index(idx, 0, QModelIndex()), index(idx, columns.length()-1, QModelIndex()));
}

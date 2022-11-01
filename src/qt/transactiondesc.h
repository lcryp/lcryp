#ifndef LCRYP_QT_TRANSACTIONDESC_H
#define LCRYP_QT_TRANSACTIONDESC_H
#include <qt/lcrypunits.h>
#include <QObject>
#include <QString>
class TransactionRecord;
namespace interfaces {
class Node;
class Wallet;
struct WalletTx;
struct WalletTxStatus;
}
class TransactionDesc: public QObject
{
    Q_OBJECT
public:
    static QString toHTML(interfaces::Node& node, interfaces::Wallet& wallet, TransactionRecord* rec, LcRypUnit unit);
private:
    TransactionDesc() {}
    static QString FormatTxStatus(const interfaces::WalletTxStatus& status, bool inMempool);
};
#endif

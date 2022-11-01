#ifndef LCRYP_QT_WALLETMODELTRANSACTION_H
#define LCRYP_QT_WALLETMODELTRANSACTION_H
#include <primitives/transaction.h>
#include <qt/sendcoinsrecipient.h>
#include <consensus/amount.h>
#include <QObject>
class SendCoinsRecipient;
namespace interfaces {
class Node;
}
class WalletModelTransaction
{
public:
    explicit WalletModelTransaction(const QList<SendCoinsRecipient> &recipients);
    QList<SendCoinsRecipient> getRecipients() const;
    CTransactionRef& getWtx();
    void setWtx(const CTransactionRef&);
    unsigned int getTransactionSize();
    void setTransactionFee(const CAmount& newFee);
    CAmount getTransactionFee() const;
    CAmount getTotalTransactionAmount() const;
    void reassignAmounts(int nChangePosRet);
private:
    QList<SendCoinsRecipient> recipients;
    CTransactionRef wtx;
    CAmount fee;
};
#endif

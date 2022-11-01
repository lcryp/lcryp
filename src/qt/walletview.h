#ifndef LCRYP_QT_WALLETVIEW_H
#define LCRYP_QT_WALLETVIEW_H
#include <consensus/amount.h>
#include <qt/lcrypunits.h>
#include <QStackedWidget>
class ClientModel;
class OverviewPage;
class PlatformStyle;
class ReceiveCoinsDialog;
class SendCoinsDialog;
class MiningDialog;
class SendCoinsRecipient;
class TransactionView;
class WalletModel;
class AddressBookPage;
QT_BEGIN_NAMESPACE
class QModelIndex;
class QProgressDialog;
QT_END_NAMESPACE
class WalletView : public QStackedWidget
{
    Q_OBJECT
public:
    explicit WalletView(WalletModel* wallet_model, const PlatformStyle* platformStyle, QWidget* parent);
    ~WalletView();
    void setClientModel(ClientModel *clientModel);
    WalletModel* getWalletModel() const noexcept { return walletModel; }
    bool handlePaymentRequest(const SendCoinsRecipient& recipient);
    void showOutOfSyncWarning(bool fShow);
private:
    ClientModel *clientModel;
    WalletModel* const walletModel;
    OverviewPage *overviewPage;
    MiningDialog* miningDialog;
    QWidget *transactionsPage;
    ReceiveCoinsDialog *receiveCoinsPage;
    SendCoinsDialog *sendCoinsPage;
    AddressBookPage *usedSendingAddressesPage;
    AddressBookPage *usedReceivingAddressesPage;
    TransactionView *transactionView;
    QProgressDialog* progressDialog{nullptr};
    const PlatformStyle *platformStyle;
public Q_SLOTS:
    void gotoOverviewPage();
    void gotoHistoryPage();
    void gotoMinerPage();
    void gotoReceiveCoinsPage();
    void gotoSendCoinsPage(QString addr = "");
    void gotoSignMessageTab(QString addr = "");
    void gotoVerifyMessageTab(QString addr = "");
    void processNewTransaction(const QModelIndex& parent, int start, int  );
    void encryptWallet();
    void backupWallet();
    void changePassphrase();
    void unlockWallet();
    void usedSendingAddresses();
    void usedReceivingAddresses();
    void showProgress(const QString &title, int nProgress);
Q_SIGNALS:
    void setPrivacy(bool privacy);
    void transactionClicked();
    void coinsSent();
    void message(const QString &title, const QString &message, unsigned int style);
    void encryptionStatusChanged();
    void incomingTransaction(const QString& date, LcRypUnit unit, const CAmount& amount, const QString& type, const QString& address, const QString& label, const QString& walletName);
    void outOfSyncWarningClicked();
};
#endif

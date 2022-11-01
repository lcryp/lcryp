#ifndef LCRYP_QT_WALLETFRAME_H
#define LCRYP_QT_WALLETFRAME_H
#include <QFrame>
#include <QMap>
class ClientModel;
class PlatformStyle;
class SendCoinsRecipient;
class WalletModel;
class WalletView;
QT_BEGIN_NAMESPACE
class QStackedWidget;
QT_END_NAMESPACE
class WalletFrame : public QFrame
{
    Q_OBJECT
public:
    explicit WalletFrame(const PlatformStyle* platformStyle, QWidget* parent);
    ~WalletFrame();
    void setClientModel(ClientModel *clientModel);
    bool addView(WalletView* walletView);
    void setCurrentWallet(WalletModel* wallet_model);
    void removeWallet(WalletModel* wallet_model);
    void removeAllWallets();
    bool handlePaymentRequest(const SendCoinsRecipient& recipient);
    void showOutOfSyncWarning(bool fShow);
    QSize sizeHint() const override { return m_size_hint; }
Q_SIGNALS:
    void createWalletButtonClicked();
    void message(const QString& title, const QString& message, unsigned int style);
    void currentWalletSet();
private:
    QStackedWidget *walletStack;
    ClientModel *clientModel;
    QMap<WalletModel*, WalletView*> mapWalletViews;
    bool bOutOfSync;
    const PlatformStyle *platformStyle;
    const QSize m_size_hint;
public:
    WalletView* currentWalletView() const;
    WalletModel* currentWalletModel() const;
public Q_SLOTS:
    void gotoOverviewPage();
    void gotoHistoryPage();
    void gotoMinerPage();
    void gotoReceiveCoinsPage();
    void gotoSendCoinsPage(QString addr = "");
    void gotoSignMessageTab(QString addr = "");
    void gotoVerifyMessageTab(QString addr = "");
    void gotoLoadPSBT(bool from_clipboard = false);
    void encryptWallet();
    void backupWallet();
    void changePassphrase();
    void unlockWallet();
    void usedSendingAddresses();
    void usedReceivingAddresses();
};
#endif

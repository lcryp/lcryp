#ifndef LCRYP_QT_OVERVIEWPAGE_H
#define LCRYP_QT_OVERVIEWPAGE_H
#include <interfaces/wallet.h>
#include <QWidget>
#include <memory>
class ClientModel;
class TransactionFilterProxy;
class TxViewDelegate;
class PlatformStyle;
class WalletModel;
namespace Ui {
    class OverviewPage;
}
QT_BEGIN_NAMESPACE
class QModelIndex;
QT_END_NAMESPACE
class OverviewPage : public QWidget
{
    Q_OBJECT
public:
    explicit OverviewPage(const PlatformStyle *platformStyle, QWidget *parent = nullptr);
    ~OverviewPage();
    void setClientModel(ClientModel *clientModel);
    void setWalletModel(WalletModel *walletModel);
    void showOutOfSyncWarning(bool fShow);
public Q_SLOTS:
    void setBalance(const interfaces::WalletBalances& balances);
    void setPrivacy(bool privacy);
Q_SIGNALS:
    void transactionClicked(const QModelIndex &index);
    void outOfSyncWarningClicked();
protected:
    void changeEvent(QEvent* e) override;
private:
    Ui::OverviewPage *ui;
    ClientModel *clientModel;
    WalletModel *walletModel;
    bool m_privacy{false};
    const PlatformStyle* m_platform_style;
    TxViewDelegate *txdelegate;
    std::unique_ptr<TransactionFilterProxy> filter;
private Q_SLOTS:
    void updateDisplayUnit();
    void handleTransactionClicked(const QModelIndex &index);
    void updateAlerts(const QString &warnings);
    void updateWatchOnlyLabels(bool showWatchOnly);
    void setMonospacedFont(bool use_embedded_font);
};
#endif

#ifndef LCRYP_QT_SENDCOINSDIALOG_H
#define LCRYP_QT_SENDCOINSDIALOG_H
#include <qt/clientmodel.h>
#include <qt/walletmodel.h>
#include <QDialog>
#include <QMessageBox>
#include <QString>
#include <QTimer>
class PlatformStyle;
class SendCoinsEntry;
class SendCoinsRecipient;
enum class SynchronizationState;
namespace wallet {
class CCoinControl;
}
namespace Ui {
    class SendCoinsDialog;
}
QT_BEGIN_NAMESPACE
class QUrl;
QT_END_NAMESPACE
class SendCoinsDialog : public QDialog
{
    Q_OBJECT
public:
    explicit SendCoinsDialog(const PlatformStyle *platformStyle, QWidget *parent = nullptr);
    ~SendCoinsDialog();
    void setClientModel(ClientModel *clientModel);
    void setModel(WalletModel *model);
    QWidget *setupTabChain(QWidget *prev);
    void setAddress(const QString &address);
    void pasteEntry(const SendCoinsRecipient &rv);
    bool handlePaymentRequest(const SendCoinsRecipient &recipient);
public Q_SLOTS:
    void clear();
    void reject() override;
    void accept() override;
    SendCoinsEntry *addEntry();
    void updateTabsAndLabels();
    void setBalance(const interfaces::WalletBalances& balances);
Q_SIGNALS:
    void coinsSent(const uint256& txid);
private:
    Ui::SendCoinsDialog *ui;
    ClientModel *clientModel;
    WalletModel *model;
    std::unique_ptr<wallet::CCoinControl> m_coin_control;
    std::unique_ptr<WalletModelTransaction> m_current_transaction;
    bool fNewRecipientAllowed;
    bool fFeeMinimized;
    const PlatformStyle *platformStyle;
    void presentPSBT(PartiallySignedTransaction& psbt);
    void processSendCoinsReturn(const WalletModel::SendCoinsReturn &sendCoinsReturn, const QString &msgArg = QString());
    void minimizeFeeSection(bool fMinimize);
    bool PrepareSendText(QString& question_string, QString& informative_text, QString& detailed_text);
    bool signWithExternalSigner(PartiallySignedTransaction& psbt, CMutableTransaction& mtx, bool& complete);
    void updateFeeMinimizedLabel();
    void updateCoinControlState();
private Q_SLOTS:
    void sendButtonClicked(bool checked);
    void on_buttonChooseFee_clicked();
    void on_buttonMinimizeFee_clicked();
    void removeEntry(SendCoinsEntry* entry);
    void useAvailableBalance(SendCoinsEntry* entry);
    void refreshBalance();
    void coinControlFeatureChanged(bool);
    void coinControlButtonClicked();
    void coinControlChangeChecked(int);
    void coinControlChangeEdited(const QString &);
    void coinControlUpdateLabels();
    void coinControlClipboardQuantity();
    void coinControlClipboardAmount();
    void coinControlClipboardFee();
    void coinControlClipboardAfterFee();
    void coinControlClipboardBytes();
    void coinControlClipboardLowOutput();
    void coinControlClipboardChange();
    void updateFeeSectionControls();
    void updateNumberOfBlocks(int count, const QDateTime& blockDate, double nVerificationProgress, SyncType synctype, SynchronizationState sync_state);
    void updateSmartFeeLabel();
Q_SIGNALS:
    void message(const QString &title, const QString &message, unsigned int style);
};
#define SEND_CONFIRM_DELAY   3
class SendConfirmationDialog : public QMessageBox
{
    Q_OBJECT
public:
    SendConfirmationDialog(const QString& title, const QString& text, const QString& informative_text = "", const QString& detailed_text = "", int secDelay = SEND_CONFIRM_DELAY, bool enable_send = true, bool always_show_unsigned = true, QWidget* parent = nullptr);
    int exec() override;
private Q_SLOTS:
    void countDown();
    void updateButtons();
private:
    QAbstractButton *yesButton;
    QAbstractButton *m_psbt_button;
    QTimer countDownTimer;
    int secDelay;
    QString confirmButtonText{tr("Send")};
    bool m_enable_send;
    QString m_psbt_button_text{tr("Create Unsigned")};
};
#endif

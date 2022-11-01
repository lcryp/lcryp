#ifndef LCRYP_QT_PSBTOPERATIONSDIALOG_H
#define LCRYP_QT_PSBTOPERATIONSDIALOG_H
#include <QDialog>
#include <psbt.h>
#include <qt/clientmodel.h>
#include <qt/walletmodel.h>
namespace Ui {
class PSBTOperationsDialog;
}
class PSBTOperationsDialog : public QDialog
{
    Q_OBJECT
public:
    explicit PSBTOperationsDialog(QWidget* parent, WalletModel* walletModel, ClientModel* clientModel);
    ~PSBTOperationsDialog();
    void openWithPSBT(PartiallySignedTransaction psbtx);
public Q_SLOTS:
    void signTransaction();
    void broadcastTransaction();
    void copyToClipboard();
    void saveTransaction();
private:
    Ui::PSBTOperationsDialog* m_ui;
    PartiallySignedTransaction m_transaction_data;
    WalletModel* m_wallet_model;
    ClientModel* m_client_model;
    enum class StatusLevel {
        INFO,
        WARN,
        ERR
    };
    size_t couldSignInputs(const PartiallySignedTransaction &psbtx);
    void updateTransactionDisplay();
    std::string renderTransaction(const PartiallySignedTransaction &psbtx);
    void showStatus(const QString &msg, StatusLevel level);
    void showTransactionStatus(const PartiallySignedTransaction &psbtx);
};
#endif

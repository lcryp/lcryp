#ifndef LCRYP_QT_RECEIVEREQUESTDIALOG_H
#define LCRYP_QT_RECEIVEREQUESTDIALOG_H
#include <qt/sendcoinsrecipient.h>
#include <QDialog>
class WalletModel;
namespace Ui {
    class ReceiveRequestDialog;
}
class ReceiveRequestDialog : public QDialog
{
    Q_OBJECT
public:
    explicit ReceiveRequestDialog(QWidget *parent = nullptr);
    ~ReceiveRequestDialog();
    void setModel(WalletModel *model);
    void setInfo(const SendCoinsRecipient &info);
private Q_SLOTS:
    void on_btnCopyURI_clicked();
    void on_btnCopyAddress_clicked();
    void updateDisplayUnit();
private:
    Ui::ReceiveRequestDialog *ui;
    WalletModel *model;
    SendCoinsRecipient info;
};
#endif

#ifndef LCRYP_QT_CREATEWALLETDIALOG_H
#define LCRYP_QT_CREATEWALLETDIALOG_H
#include <QDialog>
#include <memory>
namespace interfaces {
class ExternalSigner;
}
class WalletModel;
namespace Ui {
    class CreateWalletDialog;
}
class CreateWalletDialog : public QDialog
{
    Q_OBJECT
public:
    explicit CreateWalletDialog(QWidget* parent);
    virtual ~CreateWalletDialog();
    void setSigners(const std::vector<std::unique_ptr<interfaces::ExternalSigner>>& signers);
    QString walletName() const;
    bool isEncryptWalletChecked() const;
    bool isDisablePrivateKeysChecked() const;
    bool isMakeBlankWalletChecked() const;
    bool isDescriptorWalletChecked() const;
    bool isExternalSignerChecked() const;
private:
    Ui::CreateWalletDialog *ui;
    bool m_has_signers = false;
};
#endif

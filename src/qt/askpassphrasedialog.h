#ifndef LCRYP_QT_ASKPASSPHRASEDIALOG_H
#define LCRYP_QT_ASKPASSPHRASEDIALOG_H
#include <QDialog>
#include <support/allocators/secure.h>
class WalletModel;
namespace Ui {
    class AskPassphraseDialog;
}
class AskPassphraseDialog : public QDialog
{
    Q_OBJECT
public:
    enum Mode {
        Encrypt,
        Unlock,
        ChangePass,
    };
    explicit AskPassphraseDialog(Mode mode, QWidget *parent, SecureString* passphrase_out = nullptr);
    ~AskPassphraseDialog();
    void accept() override;
    void setModel(WalletModel *model);
private:
    Ui::AskPassphraseDialog *ui;
    Mode mode;
    WalletModel *model;
    bool fCapsLock;
    SecureString* m_passphrase_out;
private Q_SLOTS:
    void textChanged();
    void secureClearPassFields();
    void toggleShowPassword(bool);
protected:
    bool event(QEvent *event) override;
    bool eventFilter(QObject *object, QEvent *event) override;
};
#endif

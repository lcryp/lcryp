#ifndef LCRYP_QT_OPTIONSDIALOG_H
#define LCRYP_QT_OPTIONSDIALOG_H
#include <QDialog>
#include <QValidator>
class ClientModel;
class OptionsModel;
class QValidatedLineEdit;
QT_BEGIN_NAMESPACE
class QDataWidgetMapper;
QT_END_NAMESPACE
namespace Ui {
class OptionsDialog;
}
class ProxyAddressValidator : public QValidator
{
    Q_OBJECT
public:
    explicit ProxyAddressValidator(QObject *parent);
    State validate(QString &input, int &pos) const override;
};
class OptionsDialog : public QDialog
{
    Q_OBJECT
public:
    explicit OptionsDialog(QWidget *parent, bool enableWallet);
    ~OptionsDialog();
    enum Tab {
        TAB_MAIN,
        TAB_NETWORK,
    };
    void setClientModel(ClientModel* client_model);
    void setModel(OptionsModel *model);
    void setMapper();
    void setCurrentTab(OptionsDialog::Tab tab);
private Q_SLOTS:
    void setOkButtonState(bool fState);
    void on_resetButton_clicked();
    void on_openLcRypConfButton_clicked();
    void on_okButton_clicked();
    void on_cancelButton_clicked();
    void on_showTrayIcon_stateChanged(int state);
    void togglePruneWarning(bool enabled);
    void showRestartWarning(bool fPersistent = false);
    void clearStatusLabel();
    void updateProxyValidationState();
    void updateDefaultProxyNets();
Q_SIGNALS:
    void proxyIpChecks(QValidatedLineEdit *pUiProxyIp, uint16_t nProxyPort);
    void quitOnReset();
private:
    Ui::OptionsDialog *ui;
    ClientModel* m_client_model{nullptr};
    OptionsModel *model;
    QDataWidgetMapper *mapper;
};
#endif

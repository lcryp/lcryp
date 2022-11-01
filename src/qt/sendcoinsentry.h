#ifndef LCRYP_QT_SENDCOINSENTRY_H
#define LCRYP_QT_SENDCOINSENTRY_H
#include <qt/sendcoinsrecipient.h>
#include <QWidget>
class WalletModel;
class PlatformStyle;
namespace interfaces {
class Node;
}
namespace Ui {
    class SendCoinsEntry;
}
class SendCoinsEntry : public QWidget
{
    Q_OBJECT
public:
    explicit SendCoinsEntry(const PlatformStyle *platformStyle, QWidget *parent = nullptr);
    ~SendCoinsEntry();
    void setModel(WalletModel *model);
    bool validate(interfaces::Node& node);
    SendCoinsRecipient getValue();
    bool isClear();
    void setValue(const SendCoinsRecipient &value);
    void setAddress(const QString &address);
    void setAmount(const CAmount &amount);
    QWidget *setupTabChain(QWidget *prev);
    void setFocus();
public Q_SLOTS:
    void clear();
    void checkSubtractFeeFromAmount();
Q_SIGNALS:
    void removeEntry(SendCoinsEntry *entry);
    void useAvailableBalance(SendCoinsEntry* entry);
    void payAmountChanged();
    void subtractFeeFromAmountChanged();
private Q_SLOTS:
    void deleteClicked();
    void useAvailableBalanceClicked();
    void on_payTo_textChanged(const QString &address);
    void on_addressBookButton_clicked();
    void on_pasteButton_clicked();
    void updateDisplayUnit();
protected:
    void changeEvent(QEvent* e) override;
private:
    SendCoinsRecipient recipient;
    Ui::SendCoinsEntry *ui;
    WalletModel *model;
    const PlatformStyle *platformStyle;
    bool updateLabel(const QString &address);
};
#endif

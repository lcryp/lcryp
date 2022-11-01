#ifndef LCRYP_QT_EDITADDRESSDIALOG_H
#define LCRYP_QT_EDITADDRESSDIALOG_H
#include <QDialog>
class AddressTableModel;
namespace Ui {
    class EditAddressDialog;
}
QT_BEGIN_NAMESPACE
class QDataWidgetMapper;
QT_END_NAMESPACE
class EditAddressDialog : public QDialog
{
    Q_OBJECT
public:
    enum Mode {
        NewSendingAddress,
        EditReceivingAddress,
        EditSendingAddress
    };
    explicit EditAddressDialog(Mode mode, QWidget *parent = nullptr);
    ~EditAddressDialog();
    void setModel(AddressTableModel *model);
    void loadRow(int row);
    QString getAddress() const;
    void setAddress(const QString &address);
public Q_SLOTS:
    void accept() override;
private:
    bool saveCurrentRow();
    QString getDuplicateAddressWarning() const;
    Ui::EditAddressDialog *ui;
    QDataWidgetMapper *mapper;
    Mode mode;
    AddressTableModel *model;
    QString address;
};
#endif

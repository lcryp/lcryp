#ifndef LCRYP_QT_TRANSACTIONDESCDIALOG_H
#define LCRYP_QT_TRANSACTIONDESCDIALOG_H
#include <QDialog>
namespace Ui {
    class TransactionDescDialog;
}
QT_BEGIN_NAMESPACE
class QModelIndex;
QT_END_NAMESPACE
class TransactionDescDialog : public QDialog
{
    Q_OBJECT
public:
    explicit TransactionDescDialog(const QModelIndex &idx, QWidget *parent = nullptr);
    ~TransactionDescDialog();
private:
    Ui::TransactionDescDialog *ui;
};
#endif

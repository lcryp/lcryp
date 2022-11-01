#ifndef LCRYP_QT_MiningDialog_H
#define LCRYP_QT_MiningDialog_H
#include <qt/guiutil.h>
#include <qt/clientmodel.h>
#include <qt/walletmodel.h>
#include <QDialog>
#include <QHeaderView>
#include <QItemSelection>
#include <QKeyEvent>
#include <QMenu>
#include <QPoint>
#include <QVariant>
#include <string.h>
namespace Ui {
    class MiningDialog;
}
class PlatformStyle;
QT_BEGIN_NAMESPACE
class QModelIndex;
QT_END_NAMESPACE
class MiningDialog : public QDialog
{
    Q_OBJECT
public:
    enum ColumnWidths {
        DATE_COLUMN_WIDTH = 130,
        LABEL_COLUMN_WIDTH = 120,
        AMOUNT_MINIMUM_COLUMN_WIDTH = 180,
        MINIMUM_COLUMN_WIDTH = 130
    };
    explicit MiningDialog(const PlatformStyle *_platformStyle, ClientModel* client_model, QWidget *parent = nullptr);
    ~MiningDialog();
    void setClientModel(ClientModel* client_model);
public Q_SLOTS:
    void clear();
    void reject() override;
    void accept() override;
private:
    Ui::MiningDialog *ui;
    const PlatformStyle *platformStyle;
    ClientModel* m_client_model;
    QString address;
    bool flagMining;
    int countCore;
    void LoadSettingMinig(const bool action);
    QString get_Address() const;
    void set_Address(const QString &_address);
    bool get_flagMining() const;
    void set_flagMining(const bool &_flagMining);
    int get_countCore() const;
    void set_countCore(const int &_countCore);
private Q_SLOTS:
    void on_startMining_clicked();
    void on_stopMining_clicked();
};
#endif

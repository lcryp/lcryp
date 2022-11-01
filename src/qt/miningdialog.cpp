#include <wallet/wallet.h>
#include <qt/miningdialog.h>
#include <qt/forms/ui_miningdialog.h>
#include <qt/addresstablemodel.h>
#include <qt/guiutil.h>
#include <qt/optionsmodel.h>
#include <qt/platformstyle.h>
#include <qt/receiverequestdialog.h>
#include <qt/recentrequeststablemodel.h>
#include <qt/walletmodel.h>
#include <interfaces/node.h>
#include <key_io.h>
#include <node/interface_ui.h>
#include <QAction>
#include <QCursor>
#include <QMessageBox>
#include <QScrollBar>
#include <QSettings>
#include <QTextDocument>
#include <QDataWidgetMapper>
#include <string.h>
MiningDialog::MiningDialog(const PlatformStyle *_platformStyle, ClientModel* client_model, QWidget *parent) :
    QDialog(parent, GUIUtil::dialog_flags),
    ui(new Ui::MiningDialog),
    m_client_model(client_model),
    platformStyle(_platformStyle)
{
    ui->setupUi(this);
    if (!_platformStyle->getImagesOnButtons()) {
        ui->startMining->setIcon(QIcon());
        ui->stopMining->setIcon(QIcon());
    } else {
        ui->startMining->setIcon(_platformStyle->SingleColorIcon(":/icons/mining-start"));
        ui->stopMining->setIcon(_platformStyle->SingleColorIcon(":/icons/mining-stop"));
    }
}
MiningDialog::~MiningDialog()
{
     delete ui;
}
void MiningDialog::setClientModel(ClientModel* client_model)
{
    if (client_model)
    {
        m_client_model = client_model;
        clear();
    }
}
void MiningDialog::clear()
{
    LoadSettingMinig(true);
}
void MiningDialog::reject()
{
    clear();
}
void MiningDialog::accept()
{
    clear();
}
QString MiningDialog::get_Address() const
{
    return address;
}
void MiningDialog::set_Address(const QString &_address)
{
    this->address = _address;
    ui->edWallet->setText(_address);
}
bool MiningDialog::get_flagMining() const
{
    return this->flagMining;
}
int MiningDialog::get_countCore() const
{
    return this->countCore;
}
void MiningDialog::set_countCore(const int &_countCore)
{
    this->countCore = _countCore;
    ui->edcountCore->setValue(_countCore);
}
void MiningDialog::set_flagMining(const bool &_flagMining)
{
    this->flagMining = _flagMining;
    if (_flagMining)
    {
        ui->edWallet->setEnabled(false);
        ui->edcountCore->setEnabled(false);
        ui->startMining->setEnabled(false);
        ui->stopMining->setEnabled(true);
        QPalette pal_lineEdit(ui->colorMining->palette());
        pal_lineEdit.setColor(QPalette::Base, Qt::red);
        ui->colorMining->setPalette(pal_lineEdit);
    } else
    {
        ui->edWallet->setEnabled(true);
        ui->edcountCore->setEnabled(true);
        ui->startMining->setEnabled(true);
        ui->stopMining->setEnabled(false);
        QPalette pal_lineEdit(ui->colorMining->palette());
        pal_lineEdit.setColor(QPalette::Base, Qt::white);
       ui->colorMining->setPalette(pal_lineEdit);
    }
}
void MiningDialog::LoadSettingMinig(const bool action)
{
    QSettings settings;
    int _countCore = -1;
    QString _address = "";
    bool _flagMining = false;
    if (gArgs.IsArgSet("-gen")) {
        std::string gen = gArgs.GetArg("-gen","0");
        if (gen=="0") _flagMining = false;
        if (gen=="1") _flagMining = true;
    } else  {
        _flagMining = settings.value("mining_action").toBool();
    }
    if (gArgs.IsArgSet("-genproclimit")) {
        std::string genproclimit = gArgs.GetArg("-genproclimit","-1");
        _countCore = stoi(genproclimit);
    } else  {
        _countCore = settings.value("mining_core").toInt();
    }
    if (gArgs.IsArgSet("-genwallet")) {
        std::string genwallet = gArgs.GetArg("-genwallet","");
        _address = QString::fromStdString(genwallet);
    } else  {
        _address = settings.value("mining_address").toString();
    }
    set_Address(_address);
    set_countCore(_countCore);
    set_flagMining(_flagMining);
    if (action) {
        m_client_model->node().miningGUI(flagMining, countCore, address.toStdString());
    }
}
void MiningDialog::on_startMining_clicked()
{
    set_Address(ui->edWallet->text());
    set_countCore(ui->edcountCore->value());
    set_flagMining(true);
    QSettings settings;
    settings.setValue("mining_address", this->address);
    settings.setValue("mining_core", this->countCore);
    settings.setValue("mining_action", this->flagMining);
    m_client_model->node().miningGUI(flagMining, countCore, address.toStdString());
}
void MiningDialog::on_stopMining_clicked()
{
    set_flagMining(false);
    QSettings settings;
    settings.setValue("mining_action", this->flagMining);
    m_client_model->node().miningGUI(flagMining, countCore, address.toStdString());
}

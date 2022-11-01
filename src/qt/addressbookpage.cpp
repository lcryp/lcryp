#if defined(HAVE_CONFIG_H)
#include <config/lcryp-config.h>
#endif
#include <qt/addressbookpage.h>
#include <qt/forms/ui_addressbookpage.h>
#include <qt/addresstablemodel.h>
#include <qt/csvmodelwriter.h>
#include <qt/editaddressdialog.h>
#include <qt/guiutil.h>
#include <qt/platformstyle.h>
#include <QIcon>
#include <QMenu>
#include <QMessageBox>
#include <QSortFilterProxyModel>
#if (QT_VERSION >= QT_VERSION_CHECK(6, 0, 0))
#include <QRegularExpression>
#else
#include <QRegExp>
#endif
class AddressBookSortFilterProxyModel final : public QSortFilterProxyModel
{
    const QString m_type;
public:
    AddressBookSortFilterProxyModel(const QString& type, QObject* parent)
        : QSortFilterProxyModel(parent)
        , m_type(type)
    {
        setDynamicSortFilter(true);
        setFilterCaseSensitivity(Qt::CaseInsensitive);
        setSortCaseSensitivity(Qt::CaseInsensitive);
    }
protected:
    bool filterAcceptsRow(int row, const QModelIndex& parent) const override
    {
        auto model = sourceModel();
        auto label = model->index(row, AddressTableModel::Label, parent);
        if (model->data(label, AddressTableModel::TypeRole).toString() != m_type) {
            return false;
        }
        auto address = model->index(row, AddressTableModel::Address, parent);
#if (QT_VERSION >= QT_VERSION_CHECK(6, 0, 0))
        const auto pattern = filterRegularExpression();
#else
        const auto pattern = filterRegExp();
#endif
        return (model->data(address).toString().contains(pattern) ||
                model->data(label).toString().contains(pattern));
    }
};
AddressBookPage::AddressBookPage(const PlatformStyle *platformStyle, Mode _mode, Tabs _tab, QWidget *parent) :
    QDialog(parent, GUIUtil::dialog_flags),
    ui(new Ui::AddressBookPage),
    model(nullptr),
    mode(_mode),
    tab(_tab)
{
    ui->setupUi(this);
    if (!platformStyle->getImagesOnButtons()) {
        ui->newAddress->setIcon(QIcon());
        ui->copyAddress->setIcon(QIcon());
        ui->deleteAddress->setIcon(QIcon());
        ui->exportButton->setIcon(QIcon());
    } else {
        ui->newAddress->setIcon(platformStyle->SingleColorIcon(":/icons/add"));
        ui->copyAddress->setIcon(platformStyle->SingleColorIcon(":/icons/editcopy"));
        ui->deleteAddress->setIcon(platformStyle->SingleColorIcon(":/icons/remove"));
        ui->exportButton->setIcon(platformStyle->SingleColorIcon(":/icons/export"));
    }
    switch(mode)
    {
    case ForSelection:
        switch(tab)
        {
        case SendingTab: setWindowTitle(tr("Choose the address to send coins to")); break;
        case ReceivingTab: setWindowTitle(tr("Choose the address to receive coins with")); break;
        }
        connect(ui->tableView, &QTableView::doubleClicked, this, &QDialog::accept);
        ui->tableView->setEditTriggers(QAbstractItemView::NoEditTriggers);
        ui->tableView->setFocus();
        ui->closeButton->setText(tr("C&hoose"));
        ui->exportButton->hide();
        break;
    case ForEditing:
        switch(tab)
        {
        case SendingTab: setWindowTitle(tr("Sending addresses")); break;
        case ReceivingTab: setWindowTitle(tr("Receiving addresses")); break;
        }
        break;
    }
    switch(tab)
    {
    case SendingTab:
        ui->labelExplanation->setText(tr("These are your LcRyp addresses for sending payments. Always check the amount and the receiving address before sending coins."));
        ui->deleteAddress->setVisible(true);
        ui->newAddress->setVisible(true);
        break;
    case ReceivingTab:
        ui->labelExplanation->setText(tr("These are your LcRyp addresses for receiving payments. Use the 'Create new receiving address' button in the receive tab to create new addresses.\nSigning is only possible with addresses of the type 'legacy'."));
        ui->deleteAddress->setVisible(false);
        ui->newAddress->setVisible(false);
        break;
    }
    contextMenu = new QMenu(this);
    contextMenu->addAction(tr("&Copy Address"), this, &AddressBookPage::on_copyAddress_clicked);
    contextMenu->addAction(tr("Copy &Label"), this, &AddressBookPage::onCopyLabelAction);
    contextMenu->addAction(tr("&Edit"), this, &AddressBookPage::onEditAction);
    if (tab == SendingTab) {
        contextMenu->addAction(tr("&Delete"), this, &AddressBookPage::on_deleteAddress_clicked);
    }
    connect(ui->tableView, &QWidget::customContextMenuRequested, this, &AddressBookPage::contextualMenu);
    connect(ui->closeButton, &QPushButton::clicked, this, &QDialog::accept);
    GUIUtil::handleCloseWindowShortcut(this);
}
AddressBookPage::~AddressBookPage()
{
    delete ui;
}
void AddressBookPage::setModel(AddressTableModel *_model)
{
    this->model = _model;
    if(!_model)
        return;
    auto type = tab == ReceivingTab ? AddressTableModel::Receive : AddressTableModel::Send;
    proxyModel = new AddressBookSortFilterProxyModel(type, this);
    proxyModel->setSourceModel(_model);
    connect(ui->searchLineEdit, &QLineEdit::textChanged, proxyModel, &QSortFilterProxyModel::setFilterWildcard);
    ui->tableView->setModel(proxyModel);
    ui->tableView->sortByColumn(0, Qt::AscendingOrder);
    ui->tableView->horizontalHeader()->setSectionResizeMode(AddressTableModel::Label, QHeaderView::Stretch);
    ui->tableView->horizontalHeader()->setSectionResizeMode(AddressTableModel::Address, QHeaderView::ResizeToContents);
    connect(ui->tableView->selectionModel(), &QItemSelectionModel::selectionChanged,
        this, &AddressBookPage::selectionChanged);
    connect(_model, &AddressTableModel::rowsInserted, this, &AddressBookPage::selectNewAddress);
    selectionChanged();
}
void AddressBookPage::on_copyAddress_clicked()
{
    GUIUtil::copyEntryData(ui->tableView, AddressTableModel::Address);
}
void AddressBookPage::onCopyLabelAction()
{
    GUIUtil::copyEntryData(ui->tableView, AddressTableModel::Label);
}
void AddressBookPage::onEditAction()
{
    if(!model)
        return;
    if(!ui->tableView->selectionModel())
        return;
    QModelIndexList indexes = ui->tableView->selectionModel()->selectedRows();
    if(indexes.isEmpty())
        return;
    auto dlg = new EditAddressDialog(
        tab == SendingTab ?
        EditAddressDialog::EditSendingAddress :
        EditAddressDialog::EditReceivingAddress, this);
    dlg->setModel(model);
    QModelIndex origIndex = proxyModel->mapToSource(indexes.at(0));
    dlg->loadRow(origIndex.row());
    GUIUtil::ShowModalDialogAsynchronously(dlg);
}
void AddressBookPage::on_newAddress_clicked()
{
    if(!model)
        return;
    if (tab == ReceivingTab) {
        return;
    }
    EditAddressDialog dlg(EditAddressDialog::NewSendingAddress, this);
    dlg.setModel(model);
    if(dlg.exec())
    {
        newAddressToSelect = dlg.getAddress();
    }
}
void AddressBookPage::on_deleteAddress_clicked()
{
    QTableView *table = ui->tableView;
    if(!table->selectionModel())
        return;
    QModelIndexList indexes = table->selectionModel()->selectedRows();
    if(!indexes.isEmpty())
    {
        table->model()->removeRow(indexes.at(0).row());
    }
}
void AddressBookPage::selectionChanged()
{
    QTableView *table = ui->tableView;
    if(!table->selectionModel())
        return;
    if(table->selectionModel()->hasSelection())
    {
        switch(tab)
        {
        case SendingTab:
            ui->deleteAddress->setEnabled(true);
            ui->deleteAddress->setVisible(true);
            break;
        case ReceivingTab:
            ui->deleteAddress->setEnabled(false);
            ui->deleteAddress->setVisible(false);
            break;
        }
        ui->copyAddress->setEnabled(true);
    }
    else
    {
        ui->deleteAddress->setEnabled(false);
        ui->copyAddress->setEnabled(false);
    }
}
void AddressBookPage::done(int retval)
{
    QTableView *table = ui->tableView;
    if(!table->selectionModel() || !table->model())
        return;
    QModelIndexList indexes = table->selectionModel()->selectedRows(AddressTableModel::Address);
    for (const QModelIndex& index : indexes) {
        QVariant address = table->model()->data(index);
        returnValue = address.toString();
    }
    if(returnValue.isEmpty())
    {
        retval = Rejected;
    }
    QDialog::done(retval);
}
void AddressBookPage::on_exportButton_clicked()
{
    QString filename = GUIUtil::getSaveFileName(this,
        tr("Export Address List"), QString(),
        tr("Comma separated file") + QLatin1String(" (*.csv)"), nullptr);
    if (filename.isNull())
        return;
    CSVModelWriter writer(filename);
    writer.setModel(proxyModel);
    writer.addColumn("Label", AddressTableModel::Label, Qt::EditRole);
    writer.addColumn("Address", AddressTableModel::Address, Qt::EditRole);
    if(!writer.write()) {
        QMessageBox::critical(this, tr("Exporting Failed"),
            tr("There was an error trying to save the address list to %1. Please try again.").arg(filename));
    }
}
void AddressBookPage::contextualMenu(const QPoint &point)
{
    QModelIndex index = ui->tableView->indexAt(point);
    if(index.isValid())
    {
        contextMenu->exec(QCursor::pos());
    }
}
void AddressBookPage::selectNewAddress(const QModelIndex &parent, int begin, int  )
{
    QModelIndex idx = proxyModel->mapFromSource(model->index(begin, AddressTableModel::Address, parent));
    if(idx.isValid() && (idx.data(Qt::EditRole).toString() == newAddressToSelect))
    {
        ui->tableView->setFocus();
        ui->tableView->selectRow(idx.row());
        newAddressToSelect.clear();
    }
}

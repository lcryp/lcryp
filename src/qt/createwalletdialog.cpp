#if defined(HAVE_CONFIG_H)
#include <config/lcryp-config.h>
#endif
#include <interfaces/node.h>
#include <qt/createwalletdialog.h>
#include <qt/forms/ui_createwalletdialog.h>
#include <qt/guiutil.h>
#include <QPushButton>
CreateWalletDialog::CreateWalletDialog(QWidget* parent) :
    QDialog(parent, GUIUtil::dialog_flags),
    ui(new Ui::CreateWalletDialog)
{
    ui->setupUi(this);
    ui->buttonBox->button(QDialogButtonBox::Ok)->setText(tr("Create"));
    ui->buttonBox->button(QDialogButtonBox::Ok)->setEnabled(false);
    ui->wallet_name_line_edit->setFocus(Qt::ActiveWindowFocusReason);
    connect(ui->wallet_name_line_edit, &QLineEdit::textEdited, [this](const QString& text) {
        ui->buttonBox->button(QDialogButtonBox::Ok)->setEnabled(!text.isEmpty());
    });
    connect(ui->encrypt_wallet_checkbox, &QCheckBox::toggled, [this](bool checked) {
        ui->disable_privkeys_checkbox->setEnabled(!checked);
#ifdef ENABLE_EXTERNAL_SIGNER
        ui->external_signer_checkbox->setEnabled(m_has_signers && !checked);
#endif
        if (!ui->disable_privkeys_checkbox->isEnabled()) {
            ui->disable_privkeys_checkbox->setChecked(false);
        }
        if (!ui->external_signer_checkbox->isEnabled()) {
            ui->external_signer_checkbox->setChecked(false);
        }
    });
    connect(ui->external_signer_checkbox, &QCheckBox::toggled, [this](bool checked) {
        ui->encrypt_wallet_checkbox->setEnabled(!checked);
        ui->blank_wallet_checkbox->setEnabled(!checked);
        ui->disable_privkeys_checkbox->setEnabled(!checked);
        ui->descriptor_checkbox->setEnabled(!checked);
        ui->descriptor_checkbox->setChecked(checked);
        ui->encrypt_wallet_checkbox->setChecked(false);
        ui->disable_privkeys_checkbox->setChecked(checked);
        ui->blank_wallet_checkbox->setChecked(checked);
    });
    connect(ui->disable_privkeys_checkbox, &QCheckBox::toggled, [this](bool checked) {
        ui->encrypt_wallet_checkbox->setEnabled(!checked);
        if (checked) {
            ui->blank_wallet_checkbox->setChecked(true);
        }
        if (!ui->encrypt_wallet_checkbox->isEnabled()) {
            ui->encrypt_wallet_checkbox->setChecked(false);
        }
    });
    connect(ui->blank_wallet_checkbox, &QCheckBox::toggled, [this](bool checked) {
        if (!checked) {
          ui->disable_privkeys_checkbox->setChecked(false);
        }
    });
#ifndef USE_SQLITE
        ui->descriptor_checkbox->setToolTip(tr("Compiled without sqlite support (required for descriptor wallets)"));
        ui->descriptor_checkbox->setEnabled(false);
        ui->descriptor_checkbox->setChecked(false);
        ui->external_signer_checkbox->setEnabled(false);
        ui->external_signer_checkbox->setChecked(false);
#endif
#ifndef USE_BDB
        ui->descriptor_checkbox->setEnabled(false);
        ui->descriptor_checkbox->setChecked(true);
#endif
#ifndef ENABLE_EXTERNAL_SIGNER
        ui->external_signer_checkbox->setToolTip(tr("Compiled without external signing support (required for external signing)"));
        ui->external_signer_checkbox->setEnabled(false);
        ui->external_signer_checkbox->setChecked(false);
#endif
}
CreateWalletDialog::~CreateWalletDialog()
{
    delete ui;
}
void CreateWalletDialog::setSigners(const std::vector<std::unique_ptr<interfaces::ExternalSigner>>& signers)
{
    m_has_signers = !signers.empty();
    if (m_has_signers) {
        ui->external_signer_checkbox->setEnabled(true);
        ui->external_signer_checkbox->setChecked(true);
        ui->encrypt_wallet_checkbox->setEnabled(false);
        ui->encrypt_wallet_checkbox->setChecked(false);
        ui->blank_wallet_checkbox->setEnabled(false);
        ui->blank_wallet_checkbox->setChecked(false);
        ui->disable_privkeys_checkbox->setEnabled(false);
        ui->disable_privkeys_checkbox->setChecked(true);
        const std::string label = signers[0]->getName();
        ui->wallet_name_line_edit->setText(QString::fromStdString(label));
        ui->buttonBox->button(QDialogButtonBox::Ok)->setEnabled(true);
    } else {
        ui->external_signer_checkbox->setEnabled(false);
    }
}
QString CreateWalletDialog::walletName() const
{
    return ui->wallet_name_line_edit->text();
}
bool CreateWalletDialog::isEncryptWalletChecked() const
{
    return ui->encrypt_wallet_checkbox->isChecked();
}
bool CreateWalletDialog::isDisablePrivateKeysChecked() const
{
    return ui->disable_privkeys_checkbox->isChecked();
}
bool CreateWalletDialog::isMakeBlankWalletChecked() const
{
    return ui->blank_wallet_checkbox->isChecked();
}
bool CreateWalletDialog::isDescriptorWalletChecked() const
{
    return ui->descriptor_checkbox->isChecked();
}
bool CreateWalletDialog::isExternalSignerChecked() const
{
    return ui->external_signer_checkbox->isChecked();
}

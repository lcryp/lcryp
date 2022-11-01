#include <qt/walletcontroller.h>
#include <qt/askpassphrasedialog.h>
#include <qt/clientmodel.h>
#include <qt/createwalletdialog.h>
#include <qt/guiconstants.h>
#include <qt/guiutil.h>
#include <qt/walletmodel.h>
#include <external_signer.h>
#include <interfaces/handler.h>
#include <interfaces/node.h>
#include <util/string.h>
#include <util/threadnames.h>
#include <util/translation.h>
#include <wallet/wallet.h>
#include <algorithm>
#include <chrono>
#include <QApplication>
#include <QMessageBox>
#include <QMetaObject>
#include <QMutexLocker>
#include <QThread>
#include <QTimer>
#include <QWindow>
using wallet::WALLET_FLAG_BLANK_WALLET;
using wallet::WALLET_FLAG_DESCRIPTORS;
using wallet::WALLET_FLAG_DISABLE_PRIVATE_KEYS;
using wallet::WALLET_FLAG_EXTERNAL_SIGNER;
WalletController::WalletController(ClientModel& client_model, const PlatformStyle* platform_style, QObject* parent)
    : QObject(parent)
    , m_activity_thread(new QThread(this))
    , m_activity_worker(new QObject)
    , m_client_model(client_model)
    , m_node(client_model.node())
    , m_platform_style(platform_style)
    , m_options_model(client_model.getOptionsModel())
{
    m_handler_load_wallet = m_node.walletLoader().handleLoadWallet([this](std::unique_ptr<interfaces::Wallet> wallet) {
        getOrCreateWallet(std::move(wallet));
    });
    m_activity_worker->moveToThread(m_activity_thread);
    m_activity_thread->start();
    QTimer::singleShot(0, m_activity_worker, []() {
        util::ThreadRename("qt-walletctrl");
    });
}
WalletController::~WalletController()
{
    m_activity_thread->quit();
    m_activity_thread->wait();
    delete m_activity_worker;
}
std::map<std::string, bool> WalletController::listWalletDir() const
{
    QMutexLocker locker(&m_mutex);
    std::map<std::string, bool> wallets;
    for (const std::string& name : m_node.walletLoader().listWalletDir()) {
        wallets[name] = false;
    }
    for (WalletModel* wallet_model : m_wallets) {
        auto it = wallets.find(wallet_model->wallet().getWalletName());
        if (it != wallets.end()) it->second = true;
    }
    return wallets;
}
void WalletController::closeWallet(WalletModel* wallet_model, QWidget* parent)
{
    QMessageBox box(parent);
    box.setWindowTitle(tr("Close wallet"));
    box.setText(tr("Are you sure you wish to close the wallet <i>%1</i>?").arg(GUIUtil::HtmlEscape(wallet_model->getDisplayName())));
    box.setInformativeText(tr("Closing the wallet for too long can result in having to resync the entire chain if pruning is enabled."));
    box.setStandardButtons(QMessageBox::Yes|QMessageBox::Cancel);
    box.setDefaultButton(QMessageBox::Yes);
    if (box.exec() != QMessageBox::Yes) return;
    wallet_model->wallet().remove();
    removeAndDeleteWallet(wallet_model);
}
void WalletController::closeAllWallets(QWidget* parent)
{
    QMessageBox::StandardButton button = QMessageBox::question(parent, tr("Close all wallets"),
        tr("Are you sure you wish to close all wallets?"),
        QMessageBox::Yes|QMessageBox::Cancel,
        QMessageBox::Yes);
    if (button != QMessageBox::Yes) return;
    QMutexLocker locker(&m_mutex);
    for (WalletModel* wallet_model : m_wallets) {
        wallet_model->wallet().remove();
        Q_EMIT walletRemoved(wallet_model);
        delete wallet_model;
    }
    m_wallets.clear();
}
WalletModel* WalletController::getOrCreateWallet(std::unique_ptr<interfaces::Wallet> wallet)
{
    QMutexLocker locker(&m_mutex);
    if (!m_wallets.empty()) {
        std::string name = wallet->getWalletName();
        for (WalletModel* wallet_model : m_wallets) {
            if (wallet_model->wallet().getWalletName() == name) {
                return wallet_model;
            }
        }
    }
    WalletModel* wallet_model = new WalletModel(std::move(wallet), m_client_model, m_platform_style,
                                                nullptr  );
    wallet_model->moveToThread(thread());
    QMetaObject::invokeMethod(this, [wallet_model, this] {
        wallet_model->setParent(this);
    }, GUIUtil::blockingGUIThreadConnection());
    m_wallets.push_back(wallet_model);
    const bool called = QMetaObject::invokeMethod(wallet_model, "startPollBalance");
    assert(called);
    connect(wallet_model, &WalletModel::unload, this, [this, wallet_model] {
        if (QApplication::activeModalWidget()) {
            connect(qApp, &QApplication::focusWindowChanged, wallet_model, [this, wallet_model]() {
                if (!QApplication::activeModalWidget()) {
                    removeAndDeleteWallet(wallet_model);
                }
            }, Qt::QueuedConnection);
        } else {
            removeAndDeleteWallet(wallet_model);
        }
    }, Qt::QueuedConnection);
    connect(wallet_model, &WalletModel::coinsSent, this, &WalletController::coinsSent);
    Q_EMIT walletAdded(wallet_model);
    return wallet_model;
}
void WalletController::removeAndDeleteWallet(WalletModel* wallet_model)
{
    {
        QMutexLocker locker(&m_mutex);
        m_wallets.erase(std::remove(m_wallets.begin(), m_wallets.end(), wallet_model));
    }
    Q_EMIT walletRemoved(wallet_model);
    delete wallet_model;
}
WalletControllerActivity::WalletControllerActivity(WalletController* wallet_controller, QWidget* parent_widget)
    : QObject(wallet_controller)
    , m_wallet_controller(wallet_controller)
    , m_parent_widget(parent_widget)
{
    connect(this, &WalletControllerActivity::finished, this, &QObject::deleteLater);
}
void WalletControllerActivity::showProgressDialog(const QString& title_text, const QString& label_text)
{
    auto progress_dialog = new QProgressDialog(m_parent_widget);
    progress_dialog->setAttribute(Qt::WA_DeleteOnClose);
    connect(this, &WalletControllerActivity::finished, progress_dialog, &QWidget::close);
    progress_dialog->setWindowTitle(title_text);
    progress_dialog->setLabelText(label_text);
    progress_dialog->setRange(0, 0);
    progress_dialog->setCancelButton(nullptr);
    progress_dialog->setWindowModality(Qt::ApplicationModal);
    GUIUtil::PolishProgressDialog(progress_dialog);
    progress_dialog->setValue(0);
}
CreateWalletActivity::CreateWalletActivity(WalletController* wallet_controller, QWidget* parent_widget)
    : WalletControllerActivity(wallet_controller, parent_widget)
{
    m_passphrase.reserve(MAX_PASSPHRASE_SIZE);
}
CreateWalletActivity::~CreateWalletActivity()
{
    delete m_create_wallet_dialog;
    delete m_passphrase_dialog;
}
void CreateWalletActivity::askPassphrase()
{
    m_passphrase_dialog = new AskPassphraseDialog(AskPassphraseDialog::Encrypt, m_parent_widget, &m_passphrase);
    m_passphrase_dialog->setWindowModality(Qt::ApplicationModal);
    m_passphrase_dialog->show();
    connect(m_passphrase_dialog, &QObject::destroyed, [this] {
        m_passphrase_dialog = nullptr;
    });
    connect(m_passphrase_dialog, &QDialog::accepted, [this] {
        createWallet();
    });
    connect(m_passphrase_dialog, &QDialog::rejected, [this] {
        Q_EMIT finished();
    });
}
void CreateWalletActivity::createWallet()
{
    showProgressDialog(
        tr("Create Wallet"),
        tr("Creating Wallet <b>%1</b>…").arg(m_create_wallet_dialog->walletName().toHtmlEscaped()));
    std::string name = m_create_wallet_dialog->walletName().toStdString();
    uint64_t flags = 0;
    if (m_create_wallet_dialog->isDisablePrivateKeysChecked()) {
        flags |= WALLET_FLAG_DISABLE_PRIVATE_KEYS;
    }
    if (m_create_wallet_dialog->isMakeBlankWalletChecked()) {
        flags |= WALLET_FLAG_BLANK_WALLET;
    }
    if (m_create_wallet_dialog->isDescriptorWalletChecked()) {
        flags |= WALLET_FLAG_DESCRIPTORS;
    }
    if (m_create_wallet_dialog->isExternalSignerChecked()) {
        flags |= WALLET_FLAG_EXTERNAL_SIGNER;
    }
    QTimer::singleShot(500ms, worker(), [this, name, flags] {
        auto wallet{node().walletLoader().createWallet(name, m_passphrase, flags, m_warning_message)};
        if (wallet) {
            m_wallet_model = m_wallet_controller->getOrCreateWallet(std::move(*wallet));
        } else {
            m_error_message = util::ErrorString(wallet);
        }
        QTimer::singleShot(500ms, this, &CreateWalletActivity::finish);
    });
}
void CreateWalletActivity::finish()
{
    if (!m_error_message.empty()) {
        QMessageBox::critical(m_parent_widget, tr("Create wallet failed"), QString::fromStdString(m_error_message.translated));
    } else if (!m_warning_message.empty()) {
        QMessageBox::warning(m_parent_widget, tr("Create wallet warning"), QString::fromStdString(Join(m_warning_message, Untranslated("\n")).translated));
    }
    if (m_wallet_model) Q_EMIT created(m_wallet_model);
    Q_EMIT finished();
}
void CreateWalletActivity::create()
{
    m_create_wallet_dialog = new CreateWalletDialog(m_parent_widget);
    std::vector<std::unique_ptr<interfaces::ExternalSigner>> signers;
    try {
        signers = node().listExternalSigners();
    } catch (const std::runtime_error& e) {
        QMessageBox::critical(nullptr, tr("Can't list signers"), e.what());
    }
    if (signers.size() > 1) {
        QMessageBox::critical(nullptr, tr("Too many external signers found"), QString::fromStdString("More than one external signer found. Please connect only one at a time."));
        signers.clear();
    }
    m_create_wallet_dialog->setSigners(signers);
    m_create_wallet_dialog->setWindowModality(Qt::ApplicationModal);
    m_create_wallet_dialog->show();
    connect(m_create_wallet_dialog, &QObject::destroyed, [this] {
        m_create_wallet_dialog = nullptr;
    });
    connect(m_create_wallet_dialog, &QDialog::rejected, [this] {
        Q_EMIT finished();
    });
    connect(m_create_wallet_dialog, &QDialog::accepted, [this] {
        if (m_create_wallet_dialog->isEncryptWalletChecked()) {
            askPassphrase();
        } else {
            createWallet();
        }
    });
}
OpenWalletActivity::OpenWalletActivity(WalletController* wallet_controller, QWidget* parent_widget)
    : WalletControllerActivity(wallet_controller, parent_widget)
{
}
void OpenWalletActivity::finish()
{
    if (!m_error_message.empty()) {
        QMessageBox::critical(m_parent_widget, tr("Open wallet failed"), QString::fromStdString(m_error_message.translated));
    } else if (!m_warning_message.empty()) {
        QMessageBox::warning(m_parent_widget, tr("Open wallet warning"), QString::fromStdString(Join(m_warning_message, Untranslated("\n")).translated));
    }
    if (m_wallet_model) Q_EMIT opened(m_wallet_model);
    Q_EMIT finished();
}
void OpenWalletActivity::open(const std::string& path)
{
    QString name = path.empty() ? QString("["+tr("default wallet")+"]") : QString::fromStdString(path);
    showProgressDialog(
        tr("Open Wallet"),
        tr("Opening Wallet <b>%1</b>…").arg(name.toHtmlEscaped()));
    QTimer::singleShot(0, worker(), [this, path] {
        auto wallet{node().walletLoader().loadWallet(path, m_warning_message)};
        if (wallet) {
            m_wallet_model = m_wallet_controller->getOrCreateWallet(std::move(*wallet));
        } else {
            m_error_message = util::ErrorString(wallet);
        }
        QTimer::singleShot(0, this, &OpenWalletActivity::finish);
    });
}
LoadWalletsActivity::LoadWalletsActivity(WalletController* wallet_controller, QWidget* parent_widget)
    : WalletControllerActivity(wallet_controller, parent_widget)
{
}
void LoadWalletsActivity::load()
{
    showProgressDialog(
        tr("Load Wallets"),
        tr("Loading wallets…"));
    QTimer::singleShot(0, worker(), [this] {
        for (auto& wallet : node().walletLoader().getWallets()) {
            m_wallet_controller->getOrCreateWallet(std::move(wallet));
        }
        QTimer::singleShot(0, this, [this] { Q_EMIT finished(); });
    });
}
RestoreWalletActivity::RestoreWalletActivity(WalletController* wallet_controller, QWidget* parent_widget)
    : WalletControllerActivity(wallet_controller, parent_widget)
{
}
void RestoreWalletActivity::restore(const fs::path& backup_file, const std::string& wallet_name)
{
    QString name = QString::fromStdString(wallet_name);
    showProgressDialog(
        tr("Restore Wallet"),
        tr("Restoring Wallet <b>%1</b>…").arg(name.toHtmlEscaped()));
    QTimer::singleShot(0, worker(), [this, backup_file, wallet_name] {
        auto wallet{node().walletLoader().restoreWallet(backup_file, wallet_name, m_warning_message)};
        if (wallet) {
            m_wallet_model = m_wallet_controller->getOrCreateWallet(std::move(*wallet));
        } else {
            m_error_message = util::ErrorString(wallet);
        }
        QTimer::singleShot(0, this, &RestoreWalletActivity::finish);
    });
}
void RestoreWalletActivity::finish()
{
    if (!m_error_message.empty()) {
        QMessageBox::critical(m_parent_widget, tr("Restore wallet failed"), QString::fromStdString(m_error_message.translated));
    } else if (!m_warning_message.empty()) {
        QMessageBox::warning(m_parent_widget, tr("Restore wallet warning"), QString::fromStdString(Join(m_warning_message, Untranslated("\n")).translated));
    } else {
        QMessageBox::information(m_parent_widget, tr("Restore wallet message"), QString::fromStdString(Untranslated("Wallet restored successfully \n").translated));
    }
    if (m_wallet_model) Q_EMIT restored(m_wallet_model);
    Q_EMIT finished();
}

#ifndef LCRYP_QT_LCRYPGUI_H
#define LCRYP_QT_LCRYPGUI_H
#if defined(HAVE_CONFIG_H)
#include <config/lcryp-config.h>
#endif
#include <qt/lcrypunits.h>
#include <qt/clientmodel.h>
#include <qt/guiutil.h>
#include <qt/optionsdialog.h>
#include <consensus/amount.h>
#include <QLabel>
#include <QMainWindow>
#include <QMap>
#include <QMenu>
#include <QPoint>
#include <QSystemTrayIcon>
#ifdef Q_OS_MACOS
#include <qt/macos_appnap.h>
#endif
#include <memory>
class NetworkStyle;
class Notificator;
class OptionsModel;
class PlatformStyle;
class RPCConsole;
class SendCoinsRecipient;
class UnitDisplayStatusBarControl;
class WalletController;
class WalletFrame;
class WalletModel;
class HelpMessageDialog;
class ModalOverlay;
enum class SynchronizationState;
namespace interfaces {
class Handler;
class Node;
struct BlockAndHeaderTipInfo;
}
QT_BEGIN_NAMESPACE
class QAction;
class QComboBox;
class QDateTime;
class QProgressBar;
class QProgressDialog;
QT_END_NAMESPACE
namespace GUIUtil {
class ClickableLabel;
class ClickableProgressBar;
}
class LcRypGUI : public QMainWindow
{
    Q_OBJECT
public:
    static const std::string DEFAULT_UIPLATFORM;
    explicit LcRypGUI(interfaces::Node& node, const PlatformStyle *platformStyle, const NetworkStyle *networkStyle, QWidget *parent = nullptr);
    ~LcRypGUI();
    void setClientModel(ClientModel *clientModel = nullptr, interfaces::BlockAndHeaderTipInfo* tip_info = nullptr);
#ifdef ENABLE_WALLET
    void setWalletController(WalletController* wallet_controller);
    WalletController* getWalletController();
#endif
#ifdef ENABLE_WALLET
    void addWallet(WalletModel* walletModel);
    void removeWallet(WalletModel* walletModel);
    void removeAllWallets();
#endif
    bool enableWallet = false;
    bool hasTrayIcon() const { return trayIcon; }
    void unsubscribeFromCoreSignals();
    bool isPrivacyModeActivated() const;
protected:
    void changeEvent(QEvent *e) override;
    void closeEvent(QCloseEvent *event) override;
    void showEvent(QShowEvent *event) override;
    void dragEnterEvent(QDragEnterEvent *event) override;
    void dropEvent(QDropEvent *event) override;
    bool eventFilter(QObject *object, QEvent *event) override;
private:
    interfaces::Node& m_node;
    WalletController* m_wallet_controller{nullptr};
    std::unique_ptr<interfaces::Handler> m_handler_message_box;
    std::unique_ptr<interfaces::Handler> m_handler_question;
    ClientModel* clientModel = nullptr;
    WalletFrame* walletFrame = nullptr;
    UnitDisplayStatusBarControl* unitDisplayControl = nullptr;
    GUIUtil::ThemedLabel* labelWalletEncryptionIcon = nullptr;
    GUIUtil::ThemedLabel* labelWalletHDStatusIcon = nullptr;
    GUIUtil::ClickableLabel* labelProxyIcon = nullptr;
    GUIUtil::ClickableLabel* connectionsControl = nullptr;
    GUIUtil::ClickableLabel* labelBlocksIcon = nullptr;
    QLabel* progressBarLabel = nullptr;
    GUIUtil::ClickableProgressBar* progressBar = nullptr;
    QProgressDialog* progressDialog = nullptr;
    QMenuBar* appMenuBar = nullptr;
    QToolBar* appToolBar = nullptr;
    QAction* overviewAction = nullptr;
    QAction* historyAction = nullptr;
    QAction* minerAction = nullptr;
    QAction* quitAction = nullptr;
    QAction* sendCoinsAction = nullptr;
    QAction* usedSendingAddressesAction = nullptr;
    QAction* usedReceivingAddressesAction = nullptr;
    QAction* signMessageAction = nullptr;
    QAction* verifyMessageAction = nullptr;
    QAction* m_load_psbt_action = nullptr;
    QAction* m_load_psbt_clipboard_action = nullptr;
    QAction* aboutAction = nullptr;
    QAction* receiveCoinsAction = nullptr;
    QAction* optionsAction = nullptr;
    QAction* encryptWalletAction = nullptr;
    QAction* backupWalletAction = nullptr;
    QAction* changePassphraseAction = nullptr;
    QAction* aboutQtAction = nullptr;
    QAction* openRPCConsoleAction = nullptr;
    QAction* openAction = nullptr;
    QAction* showHelpMessageAction = nullptr;
    QAction* m_create_wallet_action{nullptr};
    QAction* m_open_wallet_action{nullptr};
    QMenu* m_open_wallet_menu{nullptr};
    QAction* m_restore_wallet_action{nullptr};
    QAction* m_close_wallet_action{nullptr};
    QAction* m_close_all_wallets_action{nullptr};
    QAction* m_wallet_selector_label_action = nullptr;
    QAction* m_wallet_selector_action = nullptr;
    QAction* m_mask_values_action{nullptr};
    QLabel *m_wallet_selector_label = nullptr;
    QComboBox* m_wallet_selector = nullptr;
    QSystemTrayIcon* trayIcon = nullptr;
    const std::unique_ptr<QMenu> trayIconMenu;
    Notificator* notificator = nullptr;
    RPCConsole* rpcConsole = nullptr;
    HelpMessageDialog* helpMessageDialog = nullptr;
    ModalOverlay* modalOverlay = nullptr;
    QMenu* m_network_context_menu = new QMenu(this);
#ifdef Q_OS_MACOS
    CAppNapInhibitor* m_app_nap_inhibitor = nullptr;
#endif
    int prevBlocks = 0;
    int spinnerFrame = 0;
    const PlatformStyle *platformStyle;
    const NetworkStyle* const m_network_style;
    void createActions();
    void createMenuBar();
    void createToolBars();
    void createTrayIcon();
    void createTrayIconMenu();
    void setWalletActionsEnabled(bool enabled);
    void subscribeToCoreSignals();
    void updateNetworkState();
    void updateHeadersSyncProgressLabel();
    void updateHeadersPresyncProgressLabel(int64_t height, const QDateTime& blockDate);
    void openOptionsDialogWithTab(OptionsDialog::Tab tab);
Q_SIGNALS:
    void quitRequested();
    void receivedURI(const QString &uri);
    void consoleShown(RPCConsole* console);
    void setPrivacy(bool privacy);
public Q_SLOTS:
    void setNumConnections(int count);
    void setNetworkActive(bool network_active);
    void setNumBlocks(int count, const QDateTime& blockDate, double nVerificationProgress, SyncType synctype, SynchronizationState sync_state);
    void message(const QString& title, QString message, unsigned int style, bool* ret = nullptr, const QString& detailed_message = QString());
#ifdef ENABLE_WALLET
    void setCurrentWallet(WalletModel* wallet_model);
    void setCurrentWalletBySelectorIndex(int index);
    void updateWalletStatus();
private:
    void setEncryptionStatus(int status);
    void setHDStatus(bool privkeyDisabled, int hdEnabled);
public Q_SLOTS:
    bool handlePaymentRequest(const SendCoinsRecipient& recipient);
    void incomingTransaction(const QString& date, LcRypUnit unit, const CAmount& amount, const QString& type, const QString& address, const QString& label, const QString& walletName);
#endif
private:
    void updateProxyIcon();
    void updateWindowTitle();
public Q_SLOTS:
#ifdef ENABLE_WALLET
    void gotoOverviewPage();
    void gotoHistoryPage();
    void gotoReceiveCoinsPage();
    void gotoMinerPage();
    void gotoSendCoinsPage(QString addr = "");
    void gotoSignMessageTab(QString addr = "");
    void gotoVerifyMessageTab(QString addr = "");
    void gotoLoadPSBT(bool from_clipboard = false);
    void openClicked();
#endif
    void optionsClicked();
    void aboutClicked();
    void showDebugWindow();
    void showDebugWindowActivateConsole();
    void showHelpMessageClicked();
    void showNormalIfMinimized() { showNormalIfMinimized(false); }
    void showNormalIfMinimized(bool fToggleHidden);
    void toggleHidden();
    void detectShutdown();
    void showProgress(const QString &title, int nProgress);
    void showModalOverlay();
};
class UnitDisplayStatusBarControl : public QLabel
{
    Q_OBJECT
public:
    explicit UnitDisplayStatusBarControl(const PlatformStyle *platformStyle);
    void setOptionsModel(OptionsModel *optionsModel);
protected:
    void mousePressEvent(QMouseEvent *event) override;
    void changeEvent(QEvent* e) override;
private:
    OptionsModel *optionsModel;
    QMenu* menu;
    const PlatformStyle* m_platform_style;
    void onDisplayUnitsClicked(const QPoint& point);
    void createContextMenu();
private Q_SLOTS:
    void updateDisplayUnit(LcRypUnit newUnits);
    void onMenuSelection(QAction* action);
};
#endif

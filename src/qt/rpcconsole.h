#ifndef LCRYP_QT_RPCCONSOLE_H
#define LCRYP_QT_RPCCONSOLE_H
#if defined(HAVE_CONFIG_H)
#include <config/lcryp-config.h>
#endif
#include <qt/clientmodel.h>
#include <qt/guiutil.h>
#include <qt/peertablemodel.h>
#include <net.h>
#include <QByteArray>
#include <QCompleter>
#include <QThread>
#include <QWidget>
class PlatformStyle;
class RPCExecutor;
class RPCTimerInterface;
class WalletModel;
namespace interfaces {
    class Node;
}
namespace Ui {
    class RPCConsole;
}
QT_BEGIN_NAMESPACE
class QDateTime;
class QMenu;
class QItemSelection;
QT_END_NAMESPACE
class RPCConsole: public QWidget
{
    Q_OBJECT
public:
    explicit RPCConsole(interfaces::Node& node, const PlatformStyle *platformStyle, QWidget *parent);
    ~RPCConsole();
    static bool RPCParseCommandLine(interfaces::Node* node, std::string &strResult, const std::string &strCommand, bool fExecute, std::string * const pstrFilteredOut = nullptr, const WalletModel* wallet_model = nullptr);
    static bool RPCExecuteCommandLine(interfaces::Node& node, std::string &strResult, const std::string &strCommand, std::string * const pstrFilteredOut = nullptr, const WalletModel* wallet_model = nullptr) {
        return RPCParseCommandLine(&node, strResult, strCommand, true, pstrFilteredOut, wallet_model);
    }
    void setClientModel(ClientModel *model = nullptr, int bestblock_height = 0, int64_t bestblock_date = 0, double verification_progress = 0.0);
#ifdef ENABLE_WALLET
    void addWallet(WalletModel* const walletModel);
    void removeWallet(WalletModel* const walletModel);
#endif
    enum MessageClass {
        MC_ERROR,
        MC_DEBUG,
        CMD_REQUEST,
        CMD_REPLY,
        CMD_ERROR
    };
    enum class TabTypes {
        INFO,
        CONSOLE,
        GRAPH,
        PEERS
    };
    std::vector<TabTypes> tabs() const { return {TabTypes::INFO, TabTypes::CONSOLE, TabTypes::GRAPH, TabTypes::PEERS}; }
    QString tabTitle(TabTypes tab_type) const;
    QKeySequence tabShortcut(TabTypes tab_type) const;
protected:
    virtual bool eventFilter(QObject* obj, QEvent *event) override;
    void keyPressEvent(QKeyEvent *) override;
    void changeEvent(QEvent* e) override;
private Q_SLOTS:
    void on_lineEdit_returnPressed();
    void on_tabWidget_currentChanged(int index);
    void on_openDebugLogfileButton_clicked();
    void on_sldGraphRange_valueChanged(int value);
    void updateTrafficStats(quint64 totalBytesIn, quint64 totalBytesOut);
    void resizeEvent(QResizeEvent *event) override;
    void showEvent(QShowEvent *event) override;
    void hideEvent(QHideEvent *event) override;
    void showPeersTableContextMenu(const QPoint& point);
    void showBanTableContextMenu(const QPoint& point);
    void showOrHideBanTableIfRequired();
    void clearSelectedNode();
    void updateDetailWidget();
public Q_SLOTS:
    void clear(bool keep_prompt = false);
    void fontBigger();
    void fontSmaller();
    void setFontSize(int newSize);
    void message(int category, const QString &msg) { message(category, msg, false); }
    void message(int category, const QString &message, bool html);
    void setNumConnections(int count);
    void setNetworkActive(bool networkActive);
    void setNumBlocks(int count, const QDateTime& blockDate, double nVerificationProgress, SyncType synctype);
    void setMempoolSize(long numberOfTxs, size_t dynUsage);
    void browseHistory(int offset);
    void scrollToEnd();
    void disconnectSelectedNode();
    void banSelectedNode(int bantime);
    void unbanSelectedNode();
    void setTabFocus(enum TabTypes tabType);
private:
    struct TranslatedStrings {
        const QString yes{tr("Yes")}, no{tr("No")}, to{tr("To")}, from{tr("From")},
            ban_for{tr("Ban for")}, na{tr("N/A")}, unknown{tr("Unknown")};
    } const ts;
    void startExecutor();
    void setTrafficGraphRange(int mins);
    enum ColumnWidths
    {
        ADDRESS_COLUMN_WIDTH = 200,
        SUBVERSION_COLUMN_WIDTH = 150,
        PING_COLUMN_WIDTH = 80,
        BANSUBNET_COLUMN_WIDTH = 200,
        BANTIME_COLUMN_WIDTH = 250
    };
    interfaces::Node& m_node;
    Ui::RPCConsole* const ui;
    ClientModel *clientModel = nullptr;
    QStringList history;
    int historyPtr = 0;
    QString cmdBeforeBrowsing;
    QList<NodeId> cachedNodeids;
    const PlatformStyle* const platformStyle;
    RPCTimerInterface *rpcTimerInterface = nullptr;
    QMenu *peersTableContextMenu = nullptr;
    QMenu *banTableContextMenu = nullptr;
    int consoleFontSize = 0;
    QCompleter *autoCompleter = nullptr;
    QThread thread;
    RPCExecutor* m_executor{nullptr};
    WalletModel* m_last_wallet_model{nullptr};
    bool m_is_executing{false};
    QByteArray m_peer_widget_header_state;
    QByteArray m_banlist_widget_header_state;
    void updateNetworkState();
    QString TimeDurationField(std::chrono::seconds time_now, std::chrono::seconds time_at_event) const
    {
        return time_at_event.count() ? GUIUtil::formatDurationStr(time_now - time_at_event) : tr("Never");
    }
private Q_SLOTS:
    void updateAlerts(const QString& warnings);
};
#endif

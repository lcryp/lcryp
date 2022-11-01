#ifndef LCRYP_QT_WALLETMODEL_H
#define LCRYP_QT_WALLETMODEL_H
#if defined(HAVE_CONFIG_H)
#include <config/lcryp-config.h>
#endif
#include <key.h>
#include <script/standard.h>
#include <qt/walletmodeltransaction.h>
#include <interfaces/wallet.h>
#include <support/allocators/secure.h>
#include <vector>
#include <QObject>
enum class OutputType;
class AddressTableModel;
class ClientModel;
class OptionsModel;
class PlatformStyle;
class RecentRequestsTableModel;
class SendCoinsRecipient;
class TransactionTableModel;
class WalletModelTransaction;
class CKeyID;
class COutPoint;
class CPubKey;
class uint256;
namespace interfaces {
class Node;
}
namespace wallet {
class CCoinControl;
}
QT_BEGIN_NAMESPACE
class QTimer;
QT_END_NAMESPACE
class WalletModel : public QObject
{
    Q_OBJECT
public:
    explicit WalletModel(std::unique_ptr<interfaces::Wallet> wallet, ClientModel& client_model, const PlatformStyle *platformStyle, QObject *parent = nullptr);
    ~WalletModel();
    enum StatusCode
    {
        OK,
        InvalidAmount,
        InvalidAddress,
        AmountExceedsBalance,
        AmountWithFeeExceedsBalance,
        DuplicateAddress,
        TransactionCreationFailed,
        AbsurdFee
    };
    enum EncryptionStatus
    {
        NoKeys,
        Unencrypted,
        Locked,
        Unlocked
    };
    OptionsModel* getOptionsModel() const;
    AddressTableModel* getAddressTableModel() const;
    TransactionTableModel* getTransactionTableModel() const;
    RecentRequestsTableModel* getRecentRequestsTableModel() const;
    EncryptionStatus getEncryptionStatus() const;
    bool validateAddress(const QString& address) const;
    struct SendCoinsReturn
    {
        SendCoinsReturn(StatusCode _status = OK, QString _reasonCommitFailed = "")
            : status(_status),
              reasonCommitFailed(_reasonCommitFailed)
        {
        }
        StatusCode status;
        QString reasonCommitFailed;
    };
    SendCoinsReturn prepareTransaction(WalletModelTransaction &transaction, const wallet::CCoinControl& coinControl);
    void sendCoins(WalletModelTransaction& transaction);
    bool setWalletEncrypted(const SecureString& passphrase);
    bool setWalletLocked(bool locked, const SecureString &passPhrase=SecureString());
    bool changePassphrase(const SecureString &oldPass, const SecureString &newPass);
    class UnlockContext
    {
    public:
        UnlockContext(WalletModel *wallet, bool valid, bool relock);
        ~UnlockContext();
        bool isValid() const { return valid; }
        UnlockContext(const UnlockContext&) = delete;
        UnlockContext(UnlockContext&& obj) { CopyFrom(std::move(obj)); }
        UnlockContext& operator=(UnlockContext&& rhs) { CopyFrom(std::move(rhs)); return *this; }
    private:
        WalletModel *wallet;
        bool valid;
        mutable bool relock;
        UnlockContext& operator=(const UnlockContext&) = default;
        void CopyFrom(UnlockContext&& rhs);
    };
    UnlockContext requestUnlock();
    bool bumpFee(uint256 hash, uint256& new_hash);
    bool displayAddress(std::string sAddress) const;
    static bool isWalletEnabled();
    interfaces::Node& node() const { return m_node; }
    interfaces::Wallet& wallet() const { return *m_wallet; }
    ClientModel& clientModel() const { return *m_client_model; }
    void setClientModel(ClientModel* client_model);
    QString getWalletName() const;
    QString getDisplayName() const;
    bool isMultiwallet() const;
    void refresh(bool pk_hash_only = false);
    uint256 getLastBlockProcessed() const;
    interfaces::WalletBalances getCachedBalance() const;
    CAmount getAvailableBalance(const wallet::CCoinControl* control);
private:
    std::unique_ptr<interfaces::Wallet> m_wallet;
    std::unique_ptr<interfaces::Handler> m_handler_unload;
    std::unique_ptr<interfaces::Handler> m_handler_status_changed;
    std::unique_ptr<interfaces::Handler> m_handler_address_book_changed;
    std::unique_ptr<interfaces::Handler> m_handler_transaction_changed;
    std::unique_ptr<interfaces::Handler> m_handler_show_progress;
    std::unique_ptr<interfaces::Handler> m_handler_watch_only_changed;
    std::unique_ptr<interfaces::Handler> m_handler_can_get_addrs_changed;
    ClientModel* m_client_model;
    interfaces::Node& m_node;
    bool fHaveWatchOnly;
    bool fForceCheckBalanceChanged{false};
    OptionsModel *optionsModel;
    AddressTableModel *addressTableModel;
    TransactionTableModel *transactionTableModel;
    RecentRequestsTableModel *recentRequestsTableModel;
    interfaces::WalletBalances m_cached_balances;
    EncryptionStatus cachedEncryptionStatus;
    QTimer* timer;
    uint256 m_cached_last_update_tip{};
    void subscribeToCoreSignals();
    void unsubscribeFromCoreSignals();
    void checkBalanceChanged(const interfaces::WalletBalances& new_balances);
Q_SIGNALS:
    void balanceChanged(const interfaces::WalletBalances& balances);
    void encryptionStatusChanged();
    void requireUnlock();
    void message(const QString &title, const QString &message, unsigned int style);
    void coinsSent(WalletModel* wallet, SendCoinsRecipient recipient, QByteArray transaction);
    void showProgress(const QString &title, int nProgress);
    void notifyWatchonlyChanged(bool fHaveWatchonly);
    void unload();
    void canGetAddressesChanged();
    void timerTimeout();
public Q_SLOTS:
    void startPollBalance();
    void updateStatus();
    void updateTransaction();
    void updateAddressBook(const QString &address, const QString &label, bool isMine, const QString &purpose, int status);
    void updateWatchOnlyFlag(bool fHaveWatchonly);
    void pollBalanceChanged();
};
#endif

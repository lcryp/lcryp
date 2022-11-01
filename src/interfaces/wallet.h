#ifndef LCRYP_INTERFACES_WALLET_H
#define LCRYP_INTERFACES_WALLET_H
#include <consensus/amount.h>
#include <fs.h>
#include <interfaces/chain.h>
#include <pubkey.h>
#include <script/standard.h>
#include <support/allocators/secure.h>
#include <util/message.h>
#include <util/result.h>
#include <util/ui_change_type.h>
#include <cstdint>
#include <functional>
#include <map>
#include <memory>
#include <string>
#include <tuple>
#include <type_traits>
#include <utility>
#include <vector>
class CFeeRate;
class CKey;
enum class FeeReason;
enum class OutputType;
enum class TransactionError;
struct PartiallySignedTransaction;
struct bilingual_str;
namespace wallet {
class CCoinControl;
class CWallet;
enum isminetype : unsigned int;
struct CRecipient;
struct WalletContext;
using isminefilter = std::underlying_type<isminetype>::type;
}
namespace interfaces {
class Handler;
struct WalletAddress;
struct WalletBalances;
struct WalletTx;
struct WalletTxOut;
struct WalletTxStatus;
using WalletOrderForm = std::vector<std::pair<std::string, std::string>>;
using WalletValueMap = std::map<std::string, std::string>;
class Wallet
{
public:
    virtual ~Wallet() {}
    virtual bool encryptWallet(const SecureString& wallet_passphrase) = 0;
    virtual bool isCrypted() = 0;
    virtual bool lock() = 0;
    virtual bool unlock(const SecureString& wallet_passphrase) = 0;
    virtual bool isLocked() = 0;
    virtual bool changeWalletPassphrase(const SecureString& old_wallet_passphrase,
        const SecureString& new_wallet_passphrase) = 0;
    virtual void abortRescan() = 0;
    virtual bool backupWallet(const std::string& filename) = 0;
    virtual std::string getWalletName() = 0;
    virtual util::Result<CTxDestination> getNewDestination(const OutputType type, const std::string& label) = 0;
    virtual bool getPubKey(const CScript& script, const CKeyID& address, CPubKey& pub_key) = 0;
    virtual SigningResult signMessage(const std::string& message, const PKHash& pkhash, std::string& str_sig) = 0;
    virtual bool isSpendable(const CTxDestination& dest) = 0;
    virtual bool haveWatchOnly() = 0;
    virtual bool setAddressBook(const CTxDestination& dest, const std::string& name, const std::string& purpose) = 0;
    virtual bool delAddressBook(const CTxDestination& dest) = 0;
    virtual bool getAddress(const CTxDestination& dest,
        std::string* name,
        wallet::isminetype* is_mine,
        std::string* purpose) = 0;
    virtual std::vector<WalletAddress> getAddresses() const = 0;
    virtual std::vector<std::string> getAddressReceiveRequests() = 0;
    virtual bool setAddressReceiveRequest(const CTxDestination& dest, const std::string& id, const std::string& value) = 0;
    virtual bool displayAddress(const CTxDestination& dest) = 0;
    virtual bool lockCoin(const COutPoint& output, const bool write_to_db) = 0;
    virtual bool unlockCoin(const COutPoint& output) = 0;
    virtual bool isLockedCoin(const COutPoint& output) = 0;
    virtual void listLockedCoins(std::vector<COutPoint>& outputs) = 0;
    virtual util::Result<CTransactionRef> createTransaction(const std::vector<wallet::CRecipient>& recipients,
        const wallet::CCoinControl& coin_control,
        bool sign,
        int& change_pos,
        CAmount& fee) = 0;
    virtual void commitTransaction(CTransactionRef tx,
        WalletValueMap value_map,
        WalletOrderForm order_form) = 0;
    virtual bool transactionCanBeAbandoned(const uint256& txid) = 0;
    virtual bool abandonTransaction(const uint256& txid) = 0;
    virtual bool transactionCanBeBumped(const uint256& txid) = 0;
    virtual bool createBumpTransaction(const uint256& txid,
        const wallet::CCoinControl& coin_control,
        std::vector<bilingual_str>& errors,
        CAmount& old_fee,
        CAmount& new_fee,
        CMutableTransaction& mtx) = 0;
    virtual bool signBumpTransaction(CMutableTransaction& mtx) = 0;
    virtual bool commitBumpTransaction(const uint256& txid,
        CMutableTransaction&& mtx,
        std::vector<bilingual_str>& errors,
        uint256& bumped_txid) = 0;
    virtual CTransactionRef getTx(const uint256& txid) = 0;
    virtual WalletTx getWalletTx(const uint256& txid) = 0;
    virtual std::set<WalletTx> getWalletTxs() = 0;
    virtual bool tryGetTxStatus(const uint256& txid,
        WalletTxStatus& tx_status,
        int& num_blocks,
        int64_t& block_time) = 0;
    virtual WalletTx getWalletTxDetails(const uint256& txid,
        WalletTxStatus& tx_status,
        WalletOrderForm& order_form,
        bool& in_mempool,
        int& num_blocks) = 0;
    virtual TransactionError fillPSBT(int sighash_type,
        bool sign,
        bool bip32derivs,
        size_t* n_signed,
        PartiallySignedTransaction& psbtx,
        bool& complete) = 0;
    virtual WalletBalances getBalances() = 0;
    virtual bool tryGetBalances(WalletBalances& balances, uint256& block_hash) = 0;
    virtual CAmount getBalance() = 0;
    virtual CAmount getAvailableBalance(const wallet::CCoinControl& coin_control) = 0;
    virtual wallet::isminetype txinIsMine(const CTxIn& txin) = 0;
    virtual wallet::isminetype txoutIsMine(const CTxOut& txout) = 0;
    virtual CAmount getDebit(const CTxIn& txin, wallet::isminefilter filter) = 0;
    virtual CAmount getCredit(const CTxOut& txout, wallet::isminefilter filter) = 0;
    using CoinsList = std::map<CTxDestination, std::vector<std::tuple<COutPoint, WalletTxOut>>>;
    virtual CoinsList listCoins() = 0;
    virtual std::vector<WalletTxOut> getCoins(const std::vector<COutPoint>& outputs) = 0;
    virtual CAmount getRequiredFee(unsigned int tx_bytes) = 0;
    virtual CAmount getMinimumFee(unsigned int tx_bytes,
        const wallet::CCoinControl& coin_control,
        int* returned_target,
        FeeReason* reason) = 0;
    virtual unsigned int getConfirmTarget() = 0;
    virtual bool hdEnabled() = 0;
    virtual bool canGetAddresses() = 0;
    virtual bool privateKeysDisabled() = 0;
    virtual bool taprootEnabled() = 0;
    virtual bool hasExternalSigner() = 0;
    virtual OutputType getDefaultAddressType() = 0;
    virtual CAmount getDefaultMaxTxFee() = 0;
    virtual void remove() = 0;
    virtual bool isLegacy() = 0;
    using UnloadFn = std::function<void()>;
    virtual std::unique_ptr<Handler> handleUnload(UnloadFn fn) = 0;
    using ShowProgressFn = std::function<void(const std::string& title, int progress)>;
    virtual std::unique_ptr<Handler> handleShowProgress(ShowProgressFn fn) = 0;
    using StatusChangedFn = std::function<void()>;
    virtual std::unique_ptr<Handler> handleStatusChanged(StatusChangedFn fn) = 0;
    using AddressBookChangedFn = std::function<void(const CTxDestination& address,
        const std::string& label,
        bool is_mine,
        const std::string& purpose,
        ChangeType status)>;
    virtual std::unique_ptr<Handler> handleAddressBookChanged(AddressBookChangedFn fn) = 0;
    using TransactionChangedFn = std::function<void(const uint256& txid, ChangeType status)>;
    virtual std::unique_ptr<Handler> handleTransactionChanged(TransactionChangedFn fn) = 0;
    using WatchOnlyChangedFn = std::function<void(bool have_watch_only)>;
    virtual std::unique_ptr<Handler> handleWatchOnlyChanged(WatchOnlyChangedFn fn) = 0;
    using CanGetAddressesChangedFn = std::function<void()>;
    virtual std::unique_ptr<Handler> handleCanGetAddressesChanged(CanGetAddressesChangedFn fn) = 0;
    virtual wallet::CWallet* wallet() { return nullptr; }
};
class WalletLoader : public ChainClient
{
public:
    virtual util::Result<std::unique_ptr<Wallet>> createWallet(const std::string& name, const SecureString& passphrase, uint64_t wallet_creation_flags, std::vector<bilingual_str>& warnings) = 0;
    virtual util::Result<std::unique_ptr<Wallet>> loadWallet(const std::string& name, std::vector<bilingual_str>& warnings) = 0;
    virtual std::string getWalletDir() = 0;
    virtual util::Result<std::unique_ptr<Wallet>> restoreWallet(const fs::path& backup_file, const std::string& wallet_name, std::vector<bilingual_str>& warnings) = 0;
    virtual std::vector<std::string> listWalletDir() = 0;
    virtual std::vector<std::unique_ptr<Wallet>> getWallets() = 0;
    using LoadWalletFn = std::function<void(std::unique_ptr<Wallet> wallet)>;
    virtual std::unique_ptr<Handler> handleLoadWallet(LoadWalletFn fn) = 0;
    virtual wallet::WalletContext* context() { return nullptr; }
};
struct WalletAddress
{
    CTxDestination dest;
    wallet::isminetype is_mine;
    std::string name;
    std::string purpose;
    WalletAddress(CTxDestination dest, wallet::isminetype is_mine, std::string name, std::string purpose)
        : dest(std::move(dest)), is_mine(is_mine), name(std::move(name)), purpose(std::move(purpose))
    {
    }
};
struct WalletBalances
{
    CAmount balance = 0;
    CAmount unconfirmed_balance = 0;
    CAmount immature_balance = 0;
    bool have_watch_only = false;
    CAmount watch_only_balance = 0;
    CAmount unconfirmed_watch_only_balance = 0;
    CAmount immature_watch_only_balance = 0;
    bool balanceChanged(const WalletBalances& prev) const
    {
        return balance != prev.balance || unconfirmed_balance != prev.unconfirmed_balance ||
               immature_balance != prev.immature_balance || watch_only_balance != prev.watch_only_balance ||
               unconfirmed_watch_only_balance != prev.unconfirmed_watch_only_balance ||
               immature_watch_only_balance != prev.immature_watch_only_balance;
    }
};
struct WalletTx
{
    CTransactionRef tx;
    std::vector<wallet::isminetype> txin_is_mine;
    std::vector<wallet::isminetype> txout_is_mine;
    std::vector<CTxDestination> txout_address;
    std::vector<wallet::isminetype> txout_address_is_mine;
    CAmount credit;
    CAmount debit;
    CAmount change;
    int64_t time;
    std::map<std::string, std::string> value_map;
    bool is_coinbase;
    bool operator<(const WalletTx& a) const { return tx->GetHash() < a.tx->GetHash(); }
};
struct WalletTxStatus
{
    int block_height;
    int blocks_to_maturity;
    int depth_in_main_chain;
    unsigned int time_received;
    uint32_t lock_time;
    bool is_trusted;
    bool is_abandoned;
    bool is_coinbase;
    bool is_in_main_chain;
};
struct WalletTxOut
{
    CTxOut txout;
    int64_t time;
    int depth_in_main_chain = -1;
    bool is_spent = false;
};
std::unique_ptr<Wallet> MakeWallet(wallet::WalletContext& context, const std::shared_ptr<wallet::CWallet>& wallet);
std::unique_ptr<WalletLoader> MakeWalletLoader(Chain& chain, ArgsManager& args);
}
#endif

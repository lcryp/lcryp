#ifndef LCRYP_WALLET_WALLET_H
#define LCRYP_WALLET_WALLET_H
#include <consensus/amount.h>
#include <fs.h>
#include <interfaces/chain.h>
#include <interfaces/handler.h>
#include <outputtype.h>
#include <policy/feerate.h>
#include <psbt.h>
#include <tinyformat.h>
#include <util/hasher.h>
#include <util/message.h>
#include <util/result.h>
#include <util/strencodings.h>
#include <util/string.h>
#include <util/system.h>
#include <util/ui_change_type.h>
#include <validationinterface.h>
#include <wallet/crypter.h>
#include <wallet/scriptpubkeyman.h>
#include <wallet/transaction.h>
#include <wallet/walletdb.h>
#include <wallet/walletutil.h>
#include <algorithm>
#include <atomic>
#include <map>
#include <memory>
#include <optional>
#include <set>
#include <stdexcept>
#include <stdint.h>
#include <string>
#include <utility>
#include <unordered_map>
#include <vector>
#include <boost/signals2/signal.hpp>
using LoadWalletFn = std::function<void(std::unique_ptr<interfaces::Wallet> wallet)>;
class CScript;
enum class FeeEstimateMode;
struct bilingual_str;
namespace wallet {
struct WalletContext;
void UnloadWallet(std::shared_ptr<CWallet>&& wallet);
bool AddWallet(WalletContext& context, const std::shared_ptr<CWallet>& wallet);
bool RemoveWallet(WalletContext& context, const std::shared_ptr<CWallet>& wallet, std::optional<bool> load_on_start, std::vector<bilingual_str>& warnings);
bool RemoveWallet(WalletContext& context, const std::shared_ptr<CWallet>& wallet, std::optional<bool> load_on_start);
std::vector<std::shared_ptr<CWallet>> GetWallets(WalletContext& context);
std::shared_ptr<CWallet> GetDefaultWallet(WalletContext& context, size_t& count);
std::shared_ptr<CWallet> GetWallet(WalletContext& context, const std::string& name);
std::shared_ptr<CWallet> LoadWallet(WalletContext& context, const std::string& name, std::optional<bool> load_on_start, const DatabaseOptions& options, DatabaseStatus& status, bilingual_str& error, std::vector<bilingual_str>& warnings);
std::shared_ptr<CWallet> CreateWallet(WalletContext& context, const std::string& name, std::optional<bool> load_on_start, DatabaseOptions& options, DatabaseStatus& status, bilingual_str& error, std::vector<bilingual_str>& warnings);
std::shared_ptr<CWallet> RestoreWallet(WalletContext& context, const fs::path& backup_file, const std::string& wallet_name, std::optional<bool> load_on_start, DatabaseStatus& status, bilingual_str& error, std::vector<bilingual_str>& warnings);
std::unique_ptr<interfaces::Handler> HandleLoadWallet(WalletContext& context, LoadWalletFn load_wallet);
void NotifyWalletLoaded(WalletContext& context, const std::shared_ptr<CWallet>& wallet);
std::unique_ptr<WalletDatabase> MakeWalletDatabase(const std::string& name, const DatabaseOptions& options, DatabaseStatus& status, bilingual_str& error);
constexpr CAmount DEFAULT_PAY_TX_FEE = 0;
static const CAmount DEFAULT_FALLBACK_FEE = 0;
static const CAmount DEFAULT_DISCARD_FEE = 10000;
static const CAmount DEFAULT_TRANSACTION_MINFEE = 1000;
static const CAmount DEFAULT_CONSOLIDATE_FEERATE{10000};
static const CAmount DEFAULT_MAX_AVOIDPARTIALSPEND_FEE = 0;
constexpr CAmount HIGH_APS_FEE{COIN / 10000};
static const CAmount WALLET_INCREMENTAL_RELAY_FEE = 5000;
static const bool DEFAULT_SPEND_ZEROCONF_CHANGE = true;
static const bool DEFAULT_WALLET_REJECT_LONG_CHAINS{true};
static const unsigned int DEFAULT_TX_CONFIRM_TARGET = 6;
static const bool DEFAULT_WALLET_RBF = true;
static const bool DEFAULT_WALLETBROADCAST = true;
static const bool DEFAULT_DISABLE_WALLET = false;
static const bool DEFAULT_WALLETCROSSCHAIN = false;
constexpr CAmount DEFAULT_TRANSACTION_MAXFEE{COIN / 10};
constexpr CAmount HIGH_TX_FEE_PER_KB{COIN / 100};
constexpr CAmount HIGH_MAX_TX_FEE{100 * HIGH_TX_FEE_PER_KB};
static constexpr size_t DUMMY_NESTED_P2WPKH_INPUT_SIZE = 91;
class CCoinControl;
class CWalletTx;
class ReserveDestination;
constexpr OutputType DEFAULT_ADDRESS_TYPE{OutputType::LEGACY};
static constexpr uint64_t KNOWN_WALLET_FLAGS =
        WALLET_FLAG_AVOID_REUSE
    |   WALLET_FLAG_BLANK_WALLET
    |   WALLET_FLAG_KEY_ORIGIN_METADATA
    |   WALLET_FLAG_LAST_HARDENED_XPUB_CACHED
    |   WALLET_FLAG_DISABLE_PRIVATE_KEYS
    |   WALLET_FLAG_DESCRIPTORS
    |   WALLET_FLAG_EXTERNAL_SIGNER;
static constexpr uint64_t MUTABLE_WALLET_FLAGS =
        WALLET_FLAG_AVOID_REUSE;
static const std::map<std::string,WalletFlags> WALLET_FLAG_MAP{
    {"avoid_reuse", WALLET_FLAG_AVOID_REUSE},
    {"blank", WALLET_FLAG_BLANK_WALLET},
    {"key_origin_metadata", WALLET_FLAG_KEY_ORIGIN_METADATA},
    {"last_hardened_xpub_cached", WALLET_FLAG_LAST_HARDENED_XPUB_CACHED},
    {"disable_private_keys", WALLET_FLAG_DISABLE_PRIVATE_KEYS},
    {"descriptor_wallet", WALLET_FLAG_DESCRIPTORS},
    {"external_signer", WALLET_FLAG_EXTERNAL_SIGNER}
};
extern const std::map<uint64_t,std::string> WALLET_FLAG_CAVEATS;
class ReserveDestination
{
protected:
    const CWallet* const pwallet;
    ScriptPubKeyMan* m_spk_man{nullptr};
    OutputType const type;
    int64_t nIndex{-1};
    CTxDestination address;
    bool fInternal{false};
public:
    explicit ReserveDestination(CWallet* pwallet, OutputType type)
      : pwallet(pwallet)
      , type(type) { }
    ReserveDestination(const ReserveDestination&) = delete;
    ReserveDestination& operator=(const ReserveDestination&) = delete;
    ~ReserveDestination()
    {
        ReturnDestination();
    }
    util::Result<CTxDestination> GetReservedDestination(bool internal);
    void ReturnDestination();
    void KeepDestination();
};
class CAddressBookData
{
private:
    bool m_change{true};
    std::string m_label;
public:
    std::string purpose;
    CAddressBookData() : purpose("unknown") {}
    typedef std::map<std::string, std::string> StringMap;
    StringMap destdata;
    bool IsChange() const { return m_change; }
    const std::string& GetLabel() const { return m_label; }
    void SetLabel(const std::string& label) {
        m_change = false;
        m_label = label;
    }
};
struct CRecipient
{
    CScript scriptPubKey;
    CAmount nAmount;
    bool fSubtractFeeFromAmount;
};
class WalletRescanReserver;
class CWallet final : public WalletStorage, public interfaces::Chain::Notifications
{
private:
    CKeyingMaterial vMasterKey GUARDED_BY(cs_wallet);
    bool Unlock(const CKeyingMaterial& vMasterKeyIn, bool accept_no_keys = false);
    std::atomic<bool> fAbortRescan{false};
    std::atomic<bool> fScanningWallet{false};
    std::atomic<bool> m_attaching_chain{false};
    std::atomic<int64_t> m_scanning_start{0};
    std::atomic<double> m_scanning_progress{0};
    friend class WalletRescanReserver;
    int nWalletVersion GUARDED_BY(cs_wallet){FEATURE_BASE};
    std::atomic<int64_t> m_next_resend{};
    bool fBroadcastTransactions = false;
    std::atomic<int64_t> m_best_block_time {0};
    typedef std::unordered_multimap<COutPoint, uint256, SaltedOutpointHasher> TxSpends;
    TxSpends mapTxSpends GUARDED_BY(cs_wallet);
    void AddToSpends(const COutPoint& outpoint, const uint256& wtxid, WalletBatch* batch = nullptr) EXCLUSIVE_LOCKS_REQUIRED(cs_wallet);
    void AddToSpends(const CWalletTx& wtx, WalletBatch* batch = nullptr) EXCLUSIVE_LOCKS_REQUIRED(cs_wallet);
    bool AddToWalletIfInvolvingMe(const CTransactionRef& tx, const SyncTxState& state, bool fUpdate, bool rescanning_old_block) EXCLUSIVE_LOCKS_REQUIRED(cs_wallet);
    void MarkConflicted(const uint256& hashBlock, int conflicting_height, const uint256& hashTx);
    void MarkInputsDirty(const CTransactionRef& tx) EXCLUSIVE_LOCKS_REQUIRED(cs_wallet);
    void SyncMetaData(std::pair<TxSpends::iterator, TxSpends::iterator>) EXCLUSIVE_LOCKS_REQUIRED(cs_wallet);
    void SyncTransaction(const CTransactionRef& tx, const SyncTxState& state, bool update_tx = true, bool rescanning_old_block = false) EXCLUSIVE_LOCKS_REQUIRED(cs_wallet);
    std::atomic<uint64_t> m_wallet_flags{0};
    bool SetAddressBookWithDB(WalletBatch& batch, const CTxDestination& address, const std::string& strName, const std::string& strPurpose);
    void UnsetWalletFlagWithDB(WalletBatch& batch, uint64_t flag);
    void UnsetBlankWalletFlag(WalletBatch& batch) override;
    const ArgsManager& m_args;
    interfaces::Chain* m_chain;
    std::string m_name;
    std::unique_ptr<WalletDatabase> m_database;
    uint256 m_last_block_processed GUARDED_BY(cs_wallet);
    int m_last_block_processed_height GUARDED_BY(cs_wallet) = -1;
    std::map<OutputType, ScriptPubKeyMan*> m_external_spk_managers;
    std::map<OutputType, ScriptPubKeyMan*> m_internal_spk_managers;
    std::map<uint256, std::unique_ptr<ScriptPubKeyMan>> m_spk_managers;
    static bool AttachChain(const std::shared_ptr<CWallet>& wallet, interfaces::Chain& chain, const bool rescan_required, bilingual_str& error, std::vector<bilingual_str>& warnings);
public:
    mutable RecursiveMutex cs_wallet;
    WalletDatabase& GetDatabase() const override
    {
        assert(static_cast<bool>(m_database));
        return *m_database;
    }
    const std::string& GetName() const { return m_name; }
    typedef std::map<unsigned int, CMasterKey> MasterKeyMap;
    MasterKeyMap mapMasterKeys;
    unsigned int nMasterKeyMaxID = 0;
    CWallet(interfaces::Chain* chain, const std::string& name, const ArgsManager& args, std::unique_ptr<WalletDatabase> database)
        : m_args(args),
          m_chain(chain),
          m_name(name),
          m_database(std::move(database))
    {
    }
    ~CWallet()
    {
        assert(NotifyUnload.empty());
    }
    bool IsCrypted() const;
    bool IsLocked() const override;
    bool Lock();
    bool HaveChain() const { return m_chain ? true : false; }
    std::unordered_map<uint256, CWalletTx, SaltedTxidHasher> mapWallet GUARDED_BY(cs_wallet);
    typedef std::multimap<int64_t, CWalletTx*> TxItems;
    TxItems wtxOrdered;
    int64_t nOrderPosNext GUARDED_BY(cs_wallet) = 0;
    uint64_t nAccountingEntryNumber = 0;
    std::map<CTxDestination, CAddressBookData> m_address_book GUARDED_BY(cs_wallet);
    const CAddressBookData* FindAddressBookEntry(const CTxDestination&, bool allow_change = false) const EXCLUSIVE_LOCKS_REQUIRED(cs_wallet);
    std::set<COutPoint> setLockedCoins GUARDED_BY(cs_wallet);
    std::unique_ptr<interfaces::Handler> m_chain_notifications_handler;
    interfaces::Chain& chain() const { assert(m_chain); return *m_chain; }
    const CWalletTx* GetWalletTx(const uint256& hash) const EXCLUSIVE_LOCKS_REQUIRED(cs_wallet);
    std::set<uint256> GetTxConflicts(const CWalletTx& wtx) const EXCLUSIVE_LOCKS_REQUIRED(cs_wallet);
    int GetTxDepthInMainChain(const CWalletTx& wtx) const EXCLUSIVE_LOCKS_REQUIRED(cs_wallet);
    bool IsTxInMainChain(const CWalletTx& wtx) const EXCLUSIVE_LOCKS_REQUIRED(cs_wallet)
    {
        AssertLockHeld(cs_wallet);
        return GetTxDepthInMainChain(wtx) > 0;
    }
    int GetTxBlocksToMaturity(const CWalletTx& wtx) const EXCLUSIVE_LOCKS_REQUIRED(cs_wallet);
    bool IsTxImmatureCoinBase(const CWalletTx& wtx) const EXCLUSIVE_LOCKS_REQUIRED(cs_wallet);
    bool CanSupportFeature(enum WalletFeature wf) const override EXCLUSIVE_LOCKS_REQUIRED(cs_wallet) { AssertLockHeld(cs_wallet); return IsFeatureSupported(nWalletVersion, wf); }
    bool IsSpent(const COutPoint& outpoint) const EXCLUSIVE_LOCKS_REQUIRED(cs_wallet);
    bool IsSpentKey(const CScript& scriptPubKey) const EXCLUSIVE_LOCKS_REQUIRED(cs_wallet);
    void SetSpentKeyState(WalletBatch& batch, const uint256& hash, unsigned int n, bool used, std::set<CTxDestination>& tx_destinations) EXCLUSIVE_LOCKS_REQUIRED(cs_wallet);
    bool DisplayAddress(const CTxDestination& dest) EXCLUSIVE_LOCKS_REQUIRED(cs_wallet);
    bool IsLockedCoin(const COutPoint& output) const EXCLUSIVE_LOCKS_REQUIRED(cs_wallet);
    bool LockCoin(const COutPoint& output, WalletBatch* batch = nullptr) EXCLUSIVE_LOCKS_REQUIRED(cs_wallet);
    bool UnlockCoin(const COutPoint& output, WalletBatch* batch = nullptr) EXCLUSIVE_LOCKS_REQUIRED(cs_wallet);
    bool UnlockAllCoins() EXCLUSIVE_LOCKS_REQUIRED(cs_wallet);
    void ListLockedCoins(std::vector<COutPoint>& vOutpts) const EXCLUSIVE_LOCKS_REQUIRED(cs_wallet);
    void AbortRescan() { fAbortRescan = true; }
    bool IsAbortingRescan() const { return fAbortRescan; }
    bool IsScanning() const { return fScanningWallet; }
    int64_t ScanningDuration() const { return fScanningWallet ? GetTimeMillis() - m_scanning_start : 0; }
    double ScanningProgress() const { return fScanningWallet ? (double) m_scanning_progress : 0; }
    void UpgradeKeyMetadata() EXCLUSIVE_LOCKS_REQUIRED(cs_wallet);
    void UpgradeDescriptorCache() EXCLUSIVE_LOCKS_REQUIRED(cs_wallet);
    bool LoadMinVersion(int nVersion) EXCLUSIVE_LOCKS_REQUIRED(cs_wallet) { AssertLockHeld(cs_wallet); nWalletVersion = nVersion; return true; }
    void LoadDestData(const CTxDestination& dest, const std::string& key, const std::string& value) EXCLUSIVE_LOCKS_REQUIRED(cs_wallet);
    int64_t nRelockTime GUARDED_BY(cs_wallet){0};
    Mutex m_unlock_mutex;
    bool Unlock(const SecureString& strWalletPassphrase, bool accept_no_keys = false);
    bool ChangeWalletPassphrase(const SecureString& strOldWalletPassphrase, const SecureString& strNewWalletPassphrase);
    bool EncryptWallet(const SecureString& strWalletPassphrase);
    void GetKeyBirthTimes(std::map<CKeyID, int64_t> &mapKeyBirth) const EXCLUSIVE_LOCKS_REQUIRED(cs_wallet);
    unsigned int ComputeTimeSmart(const CWalletTx& wtx, bool rescanning_old_block) const;
    int64_t IncOrderPosNext(WalletBatch *batch = nullptr) EXCLUSIVE_LOCKS_REQUIRED(cs_wallet);
    DBErrors ReorderTransactions();
    void MarkDirty();
    using UpdateWalletTxFn = std::function<bool(CWalletTx& wtx, bool new_tx)>;
    CWalletTx* AddToWallet(CTransactionRef tx, const TxState& state, const UpdateWalletTxFn& update_wtx=nullptr, bool fFlushOnClose=true, bool rescanning_old_block = false);
    bool LoadToWallet(const uint256& hash, const UpdateWalletTxFn& fill_wtx) EXCLUSIVE_LOCKS_REQUIRED(cs_wallet);
    void transactionAddedToMempool(const CTransactionRef& tx, uint64_t mempool_sequence) override;
    void blockConnected(const interfaces::BlockInfo& block) override;
    void blockDisconnected(const interfaces::BlockInfo& block) override;
    void updatedBlockTip() override;
    int64_t RescanFromTime(int64_t startTime, const WalletRescanReserver& reserver, bool update);
    struct ScanResult {
        enum { SUCCESS, FAILURE, USER_ABORT } status = SUCCESS;
        uint256 last_scanned_block;
        std::optional<int> last_scanned_height;
        uint256 last_failed_block;
    };
    ScanResult ScanForWalletTransactions(const uint256& start_block, int start_height, std::optional<int> max_height, const WalletRescanReserver& reserver, bool fUpdate, const bool save_progress);
    void transactionRemovedFromMempool(const CTransactionRef& tx, MemPoolRemovalReason reason, uint64_t mempool_sequence) override;
    void ResubmitWalletTransactions(bool relay, bool force);
    OutputType TransactionChangeType(const std::optional<OutputType>& change_type, const std::vector<CRecipient>& vecSend) const;
    bool SignTransaction(CMutableTransaction& tx) const EXCLUSIVE_LOCKS_REQUIRED(cs_wallet);
    bool SignTransaction(CMutableTransaction& tx, const std::map<COutPoint, Coin>& coins, int sighash, std::map<int, bilingual_str>& input_errors) const;
    SigningResult SignMessage(const std::string& message, const PKHash& pkhash, std::string& str_sig) const;
    TransactionError FillPSBT(PartiallySignedTransaction& psbtx,
                  bool& complete,
                  int sighash_type = SIGHASH_DEFAULT,
                  bool sign = true,
                  bool bip32derivs = true,
                  size_t* n_signed = nullptr,
                  bool finalize = true) const;
    void CommitTransaction(CTransactionRef tx, mapValue_t mapValue, std::vector<std::pair<std::string, std::string>> orderForm);
    bool SubmitTxMemoryPoolAndRelay(CWalletTx& wtx, std::string& err_string, bool relay) const
        EXCLUSIVE_LOCKS_REQUIRED(cs_wallet);
    bool DummySignTx(CMutableTransaction &txNew, const std::set<CTxOut> &txouts, const CCoinControl* coin_control = nullptr) const
    {
        std::vector<CTxOut> v_txouts(txouts.size());
        std::copy(txouts.begin(), txouts.end(), v_txouts.begin());
        return DummySignTx(txNew, v_txouts, coin_control);
    }
    bool DummySignTx(CMutableTransaction &txNew, const std::vector<CTxOut> &txouts, const CCoinControl* coin_control = nullptr) const;
    bool ImportScripts(const std::set<CScript> scripts, int64_t timestamp) EXCLUSIVE_LOCKS_REQUIRED(cs_wallet);
    bool ImportPrivKeys(const std::map<CKeyID, CKey>& privkey_map, const int64_t timestamp) EXCLUSIVE_LOCKS_REQUIRED(cs_wallet);
    bool ImportPubKeys(const std::vector<CKeyID>& ordered_pubkeys, const std::map<CKeyID, CPubKey>& pubkey_map, const std::map<CKeyID, std::pair<CPubKey, KeyOriginInfo>>& key_origins, const bool add_keypool, const bool internal, const int64_t timestamp) EXCLUSIVE_LOCKS_REQUIRED(cs_wallet);
    bool ImportScriptPubKeys(const std::string& label, const std::set<CScript>& script_pub_keys, const bool have_solving_data, const bool apply_label, const int64_t timestamp) EXCLUSIVE_LOCKS_REQUIRED(cs_wallet);
    CFeeRate m_pay_tx_fee{DEFAULT_PAY_TX_FEE};
    unsigned int m_confirm_target{DEFAULT_TX_CONFIRM_TARGET};
    bool m_spend_zero_conf_change{DEFAULT_SPEND_ZEROCONF_CHANGE};
    bool m_signal_rbf{DEFAULT_WALLET_RBF};
    bool m_allow_fallback_fee{true};
    CFeeRate m_min_fee{DEFAULT_TRANSACTION_MINFEE};
    CFeeRate m_fallback_fee{DEFAULT_FALLBACK_FEE};
    CFeeRate m_discard_rate{DEFAULT_DISCARD_FEE};
    CFeeRate m_consolidate_feerate{DEFAULT_CONSOLIDATE_FEERATE};
    CAmount m_max_aps_fee{DEFAULT_MAX_AVOIDPARTIALSPEND_FEE};
    OutputType m_default_address_type{DEFAULT_ADDRESS_TYPE};
    std::optional<OutputType> m_default_change_type{};
    CAmount m_default_max_tx_fee{DEFAULT_TRANSACTION_MAXFEE};
    size_t KeypoolCountExternalKeys() const EXCLUSIVE_LOCKS_REQUIRED(cs_wallet);
    bool TopUpKeyPool(unsigned int kpSize = 0);
    std::optional<int64_t> GetOldestKeyPoolTime() const;
    struct AddrBookFilter {
        std::optional<std::string> m_op_label{std::nullopt};
        bool ignore_change{true};
    };
    std::vector<CTxDestination> ListAddrBookAddresses(const std::optional<AddrBookFilter>& filter) const EXCLUSIVE_LOCKS_REQUIRED(cs_wallet);
    std::set<std::string> ListAddrBookLabels(const std::string& purpose) const EXCLUSIVE_LOCKS_REQUIRED(cs_wallet);
    using ListAddrBookFunc = std::function<void(const CTxDestination& dest, const std::string& label, const std::string& purpose, bool is_change)>;
    void ForEachAddrBookEntry(const ListAddrBookFunc& func) const EXCLUSIVE_LOCKS_REQUIRED(cs_wallet);
    void MarkDestinationsDirty(const std::set<CTxDestination>& destinations) EXCLUSIVE_LOCKS_REQUIRED(cs_wallet);
    util::Result<CTxDestination> GetNewDestination(const OutputType type, const std::string label);
    util::Result<CTxDestination> GetNewChangeDestination(const OutputType type);
    isminetype IsMine(const CTxDestination& dest) const EXCLUSIVE_LOCKS_REQUIRED(cs_wallet);
    isminetype IsMine(const CScript& script) const EXCLUSIVE_LOCKS_REQUIRED(cs_wallet);
    CAmount GetDebit(const CTxIn& txin, const isminefilter& filter) const;
    isminetype IsMine(const CTxOut& txout) const EXCLUSIVE_LOCKS_REQUIRED(cs_wallet);
    bool IsMine(const CTransaction& tx) const EXCLUSIVE_LOCKS_REQUIRED(cs_wallet);
    isminetype IsMine(const COutPoint& outpoint) const EXCLUSIVE_LOCKS_REQUIRED(cs_wallet);
    bool IsFromMe(const CTransaction& tx) const;
    CAmount GetDebit(const CTransaction& tx, const isminefilter& filter) const;
    void chainStateFlushed(const CBlockLocator& loc) override;
    DBErrors LoadWallet();
    DBErrors ZapSelectTx(std::vector<uint256>& vHashIn, std::vector<uint256>& vHashOut) EXCLUSIVE_LOCKS_REQUIRED(cs_wallet);
    bool SetAddressBook(const CTxDestination& address, const std::string& strName, const std::string& purpose);
    bool DelAddressBook(const CTxDestination& address);
    bool IsAddressUsed(const CTxDestination& dest) const EXCLUSIVE_LOCKS_REQUIRED(cs_wallet);
    bool SetAddressUsed(WalletBatch& batch, const CTxDestination& dest, bool used) EXCLUSIVE_LOCKS_REQUIRED(cs_wallet);
    std::vector<std::string> GetAddressReceiveRequests() const EXCLUSIVE_LOCKS_REQUIRED(cs_wallet);
    bool SetAddressReceiveRequest(WalletBatch& batch, const CTxDestination& dest, const std::string& id, const std::string& value) EXCLUSIVE_LOCKS_REQUIRED(cs_wallet);
    unsigned int GetKeyPoolSize() const EXCLUSIVE_LOCKS_REQUIRED(cs_wallet);
    void SetMinVersion(enum WalletFeature, WalletBatch* batch_in = nullptr) override;
    int GetVersion() const { LOCK(cs_wallet); return nWalletVersion; }
    std::set<uint256> GetConflicts(const uint256& txid) const EXCLUSIVE_LOCKS_REQUIRED(cs_wallet);
    bool HasWalletSpend(const CTransactionRef& tx) const EXCLUSIVE_LOCKS_REQUIRED(cs_wallet);
    void Flush();
    void Close();
    boost::signals2::signal<void ()> NotifyUnload;
    boost::signals2::signal<void(const CTxDestination& address,
                                 const std::string& label, bool isMine,
                                 const std::string& purpose, ChangeType status)>
        NotifyAddressBookChanged;
    boost::signals2::signal<void(const uint256& hashTx, ChangeType status)> NotifyTransactionChanged;
    boost::signals2::signal<void (const std::string &title, int nProgress)> ShowProgress;
    boost::signals2::signal<void (bool fHaveWatchOnly)> NotifyWatchonlyChanged;
    boost::signals2::signal<void ()> NotifyCanGetAddressesChanged;
    boost::signals2::signal<void (CWallet* wallet)> NotifyStatusChanged;
    bool GetBroadcastTransactions() const { return fBroadcastTransactions; }
    void SetBroadcastTransactions(bool broadcast) { fBroadcastTransactions = broadcast; }
    bool TransactionCanBeAbandoned(const uint256& hashTx) const;
    bool AbandonTransaction(const uint256& hashTx);
    bool MarkReplaced(const uint256& originalHash, const uint256& newHash);
    static std::shared_ptr<CWallet> Create(WalletContext& context, const std::string& name, std::unique_ptr<WalletDatabase> database, uint64_t wallet_creation_flags, bilingual_str& error, std::vector<bilingual_str>& warnings);
    void postInitProcess();
    bool BackupWallet(const std::string& strDest) const;
    bool IsHDEnabled() const;
    bool CanGetAddresses(bool internal = false) const;
    void BlockUntilSyncedToCurrentChain() const LOCKS_EXCLUDED(::cs_main) EXCLUSIVE_LOCKS_REQUIRED(!cs_wallet);
    void SetWalletFlag(uint64_t flags);
    void UnsetWalletFlag(uint64_t flag);
    bool IsWalletFlagSet(uint64_t flag) const override;
    void InitWalletFlags(uint64_t flags);
    bool LoadWalletFlags(uint64_t flags);
    bool IsLegacy() const;
    const std::string GetDisplayName() const override {
        std::string wallet_name = GetName().length() == 0 ? "default wallet" : GetName();
        return strprintf("[%s]", wallet_name);
    };
    template<typename... Params>
    void WalletLogPrintf(std::string fmt, Params... parameters) const {
        LogPrintf(("%s " + fmt).c_str(), GetDisplayName(), parameters...);
    };
    bool UpgradeWallet(int version, bilingual_str& error);
    std::set<ScriptPubKeyMan*> GetActiveScriptPubKeyMans() const;
    std::set<ScriptPubKeyMan*> GetAllScriptPubKeyMans() const;
    ScriptPubKeyMan* GetScriptPubKeyMan(const OutputType& type, bool internal) const;
    std::set<ScriptPubKeyMan*> GetScriptPubKeyMans(const CScript& script) const;
    ScriptPubKeyMan* GetScriptPubKeyMan(const uint256& id) const;
    std::unique_ptr<SigningProvider> GetSolvingProvider(const CScript& script) const;
    std::unique_ptr<SigningProvider> GetSolvingProvider(const CScript& script, SignatureData& sigdata) const;
    std::vector<WalletDescriptor> GetWalletDescriptors(const CScript& script) const;
    LegacyScriptPubKeyMan* GetLegacyScriptPubKeyMan() const;
    LegacyScriptPubKeyMan* GetOrCreateLegacyScriptPubKeyMan();
    void SetupLegacyScriptPubKeyMan();
    const CKeyingMaterial& GetEncryptionKey() const override;
    bool HasEncryptionKeys() const override;
    int GetLastBlockHeight() const EXCLUSIVE_LOCKS_REQUIRED(cs_wallet)
    {
        AssertLockHeld(cs_wallet);
        assert(m_last_block_processed_height >= 0);
        return m_last_block_processed_height;
    };
    uint256 GetLastBlockHash() const EXCLUSIVE_LOCKS_REQUIRED(cs_wallet)
    {
        AssertLockHeld(cs_wallet);
        assert(m_last_block_processed_height >= 0);
        return m_last_block_processed;
    }
    void SetLastBlockProcessed(int block_height, uint256 block_hash) EXCLUSIVE_LOCKS_REQUIRED(cs_wallet)
    {
        AssertLockHeld(cs_wallet);
        m_last_block_processed_height = block_height;
        m_last_block_processed = block_hash;
    };
    void ConnectScriptPubKeyManNotifiers();
    void LoadDescriptorScriptPubKeyMan(uint256 id, WalletDescriptor& desc);
    void AddActiveScriptPubKeyMan(uint256 id, OutputType type, bool internal);
    void LoadActiveScriptPubKeyMan(uint256 id, OutputType type, bool internal);
    void DeactivateScriptPubKeyMan(uint256 id, OutputType type, bool internal);
    void SetupDescriptorScriptPubKeyMans(const CExtKey& master_key) EXCLUSIVE_LOCKS_REQUIRED(cs_wallet);
    void SetupDescriptorScriptPubKeyMans() EXCLUSIVE_LOCKS_REQUIRED(cs_wallet);
    DescriptorScriptPubKeyMan* GetDescriptorScriptPubKeyMan(const WalletDescriptor& desc) const;
    std::optional<bool> IsInternalScriptPubKeyMan(ScriptPubKeyMan* spk_man) const;
    ScriptPubKeyMan* AddWalletDescriptor(WalletDescriptor& desc, const FlatSigningProvider& signing_provider, const std::string& label, bool internal) EXCLUSIVE_LOCKS_REQUIRED(cs_wallet);
    bool MigrateToSQLite(bilingual_str& error) EXCLUSIVE_LOCKS_REQUIRED(cs_wallet);
    std::optional<MigrationData> GetDescriptorsForLegacy(bilingual_str& error) const EXCLUSIVE_LOCKS_REQUIRED(cs_wallet);
    bool ApplyMigrationData(MigrationData& data, bilingual_str& error) EXCLUSIVE_LOCKS_REQUIRED(cs_wallet);
};
void MaybeResendWalletTxs(WalletContext& context);
class WalletRescanReserver
{
private:
    using Clock = std::chrono::steady_clock;
    using NowFn = std::function<Clock::time_point()>;
    CWallet& m_wallet;
    bool m_could_reserve;
    NowFn m_now;
public:
    explicit WalletRescanReserver(CWallet& w) : m_wallet(w), m_could_reserve(false) {}
    bool reserve()
    {
        assert(!m_could_reserve);
        if (m_wallet.fScanningWallet.exchange(true)) {
            return false;
        }
        m_wallet.m_scanning_start = GetTimeMillis();
        m_wallet.m_scanning_progress = 0;
        m_could_reserve = true;
        return true;
    }
    bool isReserved() const
    {
        return (m_could_reserve && m_wallet.fScanningWallet);
    }
    Clock::time_point now() const { return m_now ? m_now() : Clock::now(); };
    void setNow(NowFn now) { m_now = std::move(now); }
    ~WalletRescanReserver()
    {
        if (m_could_reserve) {
            m_wallet.fScanningWallet = false;
        }
    }
};
bool AddWalletSetting(interfaces::Chain& chain, const std::string& wallet_name);
bool RemoveWalletSetting(interfaces::Chain& chain, const std::string& wallet_name);
bool DummySignInput(const SigningProvider& provider, CTxIn &tx_in, const CTxOut &txout, const CCoinControl* coin_control = nullptr);
bool FillInputToWeight(CTxIn& txin, int64_t target_weight);
struct MigrationResult {
    std::string wallet_name;
    std::shared_ptr<CWallet> watchonly_wallet;
    std::shared_ptr<CWallet> solvables_wallet;
    fs::path backup_path;
};
util::Result<MigrationResult> MigrateLegacyToDescriptor(std::shared_ptr<CWallet>&& wallet, WalletContext& context);
}
#endif

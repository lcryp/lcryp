#ifndef LCRYP_TXMEMPOOL_H
#define LCRYP_TXMEMPOOL_H
#include <atomic>
#include <map>
#include <optional>
#include <set>
#include <string>
#include <utility>
#include <vector>
#include <kernel/mempool_limits.h>
#include <kernel/mempool_options.h>
#include <coins.h>
#include <consensus/amount.h>
#include <indirectmap.h>
#include <policy/feerate.h>
#include <policy/packages.h>
#include <primitives/transaction.h>
#include <random.h>
#include <sync.h>
#include <util/epochguard.h>
#include <util/hasher.h>
#include <boost/multi_index/hashed_index.hpp>
#include <boost/multi_index/ordered_index.hpp>
#include <boost/multi_index/sequenced_index.hpp>
#include <boost/multi_index_container.hpp>
class CBlockIndex;
class CChain;
class Chainstate;
extern RecursiveMutex cs_main;
static const uint32_t MEMPOOL_HEIGHT = 0x7FFFFFFF;
struct LockPoints {
    int height{0};
    int64_t time{0};
    CBlockIndex* maxInputBlock{nullptr};
};
bool TestLockPointValidity(CChain& active_chain, const LockPoints& lp) EXCLUSIVE_LOCKS_REQUIRED(cs_main);
struct CompareIteratorByHash {
    template <typename T>
    bool operator()(const std::reference_wrapper<T>& a, const std::reference_wrapper<T>& b) const
    {
        return a.get().GetTx().GetHash() < b.get().GetTx().GetHash();
    }
    template <typename T>
    bool operator()(const T& a, const T& b) const
    {
        return a->GetTx().GetHash() < b->GetTx().GetHash();
    }
};
class CTxMemPoolEntry
{
public:
    typedef std::reference_wrapper<const CTxMemPoolEntry> CTxMemPoolEntryRef;
    typedef std::set<CTxMemPoolEntryRef, CompareIteratorByHash> Parents;
    typedef std::set<CTxMemPoolEntryRef, CompareIteratorByHash> Children;
private:
    const CTransactionRef tx;
    mutable Parents m_parents;
    mutable Children m_children;
    const CAmount nFee;
    const size_t nTxWeight;
    const size_t nUsageSize;
    const int64_t nTime;
    const unsigned int entryHeight;
    const bool spendsCoinbase;
    const int64_t sigOpCost;
    CAmount m_modified_fee;
    LockPoints lockPoints;
    uint64_t nCountWithDescendants{1};
    uint64_t nSizeWithDescendants;
    CAmount nModFeesWithDescendants;
    uint64_t nCountWithAncestors{1};
    uint64_t nSizeWithAncestors;
    CAmount nModFeesWithAncestors;
    int64_t nSigOpCostWithAncestors;
public:
    CTxMemPoolEntry(const CTransactionRef& tx, CAmount fee,
                    int64_t time, unsigned int entry_height,
                    bool spends_coinbase,
                    int64_t sigops_cost, LockPoints lp);
    const CTransaction& GetTx() const { return *this->tx; }
    CTransactionRef GetSharedTx() const { return this->tx; }
    const CAmount& GetFee() const { return nFee; }
    size_t GetTxSize() const;
    size_t GetTxWeight() const { return nTxWeight; }
    std::chrono::seconds GetTime() const { return std::chrono::seconds{nTime}; }
    unsigned int GetHeight() const { return entryHeight; }
    int64_t GetSigOpCost() const { return sigOpCost; }
    CAmount GetModifiedFee() const { return m_modified_fee; }
    size_t DynamicMemoryUsage() const { return nUsageSize; }
    const LockPoints& GetLockPoints() const { return lockPoints; }
    void UpdateDescendantState(int64_t modifySize, CAmount modifyFee, int64_t modifyCount);
    void UpdateAncestorState(int64_t modifySize, CAmount modifyFee, int64_t modifyCount, int64_t modifySigOps);
    void UpdateModifiedFee(CAmount fee_diff);
    void UpdateLockPoints(const LockPoints& lp);
    uint64_t GetCountWithDescendants() const { return nCountWithDescendants; }
    uint64_t GetSizeWithDescendants() const { return nSizeWithDescendants; }
    CAmount GetModFeesWithDescendants() const { return nModFeesWithDescendants; }
    bool GetSpendsCoinbase() const { return spendsCoinbase; }
    uint64_t GetCountWithAncestors() const { return nCountWithAncestors; }
    uint64_t GetSizeWithAncestors() const { return nSizeWithAncestors; }
    CAmount GetModFeesWithAncestors() const { return nModFeesWithAncestors; }
    int64_t GetSigOpCostWithAncestors() const { return nSigOpCostWithAncestors; }
    const Parents& GetMemPoolParentsConst() const { return m_parents; }
    const Children& GetMemPoolChildrenConst() const { return m_children; }
    Parents& GetMemPoolParents() const { return m_parents; }
    Children& GetMemPoolChildren() const { return m_children; }
    mutable size_t vTxHashesIdx;
    mutable Epoch::Marker m_epoch_marker;
};
struct mempoolentry_txid
{
    typedef uint256 result_type;
    result_type operator() (const CTxMemPoolEntry &entry) const
    {
        return entry.GetTx().GetHash();
    }
    result_type operator() (const CTransactionRef& tx) const
    {
        return tx->GetHash();
    }
};
struct mempoolentry_wtxid
{
    typedef uint256 result_type;
    result_type operator() (const CTxMemPoolEntry &entry) const
    {
        return entry.GetTx().GetWitnessHash();
    }
    result_type operator() (const CTransactionRef& tx) const
    {
        return tx->GetWitnessHash();
    }
};
class CompareTxMemPoolEntryByDescendantScore
{
public:
    bool operator()(const CTxMemPoolEntry& a, const CTxMemPoolEntry& b) const
    {
        double a_mod_fee, a_size, b_mod_fee, b_size;
        GetModFeeAndSize(a, a_mod_fee, a_size);
        GetModFeeAndSize(b, b_mod_fee, b_size);
        double f1 = a_mod_fee * b_size;
        double f2 = a_size * b_mod_fee;
        if (f1 == f2) {
            return a.GetTime() >= b.GetTime();
        }
        return f1 < f2;
    }
    void GetModFeeAndSize(const CTxMemPoolEntry &a, double &mod_fee, double &size) const
    {
        double f1 = (double)a.GetModifiedFee() * a.GetSizeWithDescendants();
        double f2 = (double)a.GetModFeesWithDescendants() * a.GetTxSize();
        if (f2 > f1) {
            mod_fee = a.GetModFeesWithDescendants();
            size = a.GetSizeWithDescendants();
        } else {
            mod_fee = a.GetModifiedFee();
            size = a.GetTxSize();
        }
    }
};
class CompareTxMemPoolEntryByScore
{
public:
    bool operator()(const CTxMemPoolEntry& a, const CTxMemPoolEntry& b) const
    {
        double f1 = (double)a.GetFee() * b.GetTxSize();
        double f2 = (double)b.GetFee() * a.GetTxSize();
        if (f1 == f2) {
            return b.GetTx().GetHash() < a.GetTx().GetHash();
        }
        return f1 > f2;
    }
};
class CompareTxMemPoolEntryByEntryTime
{
public:
    bool operator()(const CTxMemPoolEntry& a, const CTxMemPoolEntry& b) const
    {
        return a.GetTime() < b.GetTime();
    }
};
class CompareTxMemPoolEntryByAncestorFee
{
public:
    template<typename T>
    bool operator()(const T& a, const T& b) const
    {
        double a_mod_fee, a_size, b_mod_fee, b_size;
        GetModFeeAndSize(a, a_mod_fee, a_size);
        GetModFeeAndSize(b, b_mod_fee, b_size);
        double f1 = a_mod_fee * b_size;
        double f2 = a_size * b_mod_fee;
        if (f1 == f2) {
            return a.GetTx().GetHash() < b.GetTx().GetHash();
        }
        return f1 > f2;
    }
    template <typename T>
    void GetModFeeAndSize(const T &a, double &mod_fee, double &size) const
    {
        double f1 = (double)a.GetModifiedFee() * a.GetSizeWithAncestors();
        double f2 = (double)a.GetModFeesWithAncestors() * a.GetTxSize();
        if (f1 > f2) {
            mod_fee = a.GetModFeesWithAncestors();
            size = a.GetSizeWithAncestors();
        } else {
            mod_fee = a.GetModifiedFee();
            size = a.GetTxSize();
        }
    }
};
struct descendant_score {};
struct entry_time {};
struct ancestor_score {};
struct index_by_wtxid {};
class CBlockPolicyEstimator;
struct TxMempoolInfo
{
    CTransactionRef tx;
    std::chrono::seconds m_time;
    CAmount fee;
    size_t vsize;
    int64_t nFeeDelta;
};
enum class MemPoolRemovalReason {
    EXPIRY,
    SIZELIMIT,
    REORG,
    BLOCK,
    CONFLICT,
    REPLACED,
};
class CTxMemPool
{
protected:
    const int m_check_ratio;
    std::atomic<unsigned int> nTransactionsUpdated{0};
    CBlockPolicyEstimator* const minerPolicyEstimator;
    uint64_t totalTxSize GUARDED_BY(cs);
    CAmount m_total_fee GUARDED_BY(cs);
    uint64_t cachedInnerUsage GUARDED_BY(cs);
    mutable int64_t lastRollingFeeUpdate GUARDED_BY(cs);
    mutable bool blockSinceLastRollingFeeBump GUARDED_BY(cs);
    mutable double rollingMinimumFeeRate GUARDED_BY(cs);
    mutable Epoch m_epoch GUARDED_BY(cs);
    mutable uint64_t m_sequence_number GUARDED_BY(cs){1};
    void trackPackageRemoved(const CFeeRate& rate) EXCLUSIVE_LOCKS_REQUIRED(cs);
    bool m_load_tried GUARDED_BY(cs){false};
    CFeeRate GetMinFee(size_t sizelimit) const;
public:
    static const int ROLLING_FEE_HALFLIFE = 60 * 60 * 12;
    typedef boost::multi_index_container<
        CTxMemPoolEntry,
        boost::multi_index::indexed_by<
            boost::multi_index::hashed_unique<mempoolentry_txid, SaltedTxidHasher>,
            boost::multi_index::hashed_unique<
                boost::multi_index::tag<index_by_wtxid>,
                mempoolentry_wtxid,
                SaltedTxidHasher
            >,
            boost::multi_index::ordered_non_unique<
                boost::multi_index::tag<descendant_score>,
                boost::multi_index::identity<CTxMemPoolEntry>,
                CompareTxMemPoolEntryByDescendantScore
            >,
            boost::multi_index::ordered_non_unique<
                boost::multi_index::tag<entry_time>,
                boost::multi_index::identity<CTxMemPoolEntry>,
                CompareTxMemPoolEntryByEntryTime
            >,
            boost::multi_index::ordered_non_unique<
                boost::multi_index::tag<ancestor_score>,
                boost::multi_index::identity<CTxMemPoolEntry>,
                CompareTxMemPoolEntryByAncestorFee
            >
        >
    > indexed_transaction_set;
    mutable RecursiveMutex cs;
    indexed_transaction_set mapTx GUARDED_BY(cs);
    using txiter = indexed_transaction_set::nth_index<0>::type::const_iterator;
    std::vector<std::pair<uint256, txiter>> vTxHashes GUARDED_BY(cs);
    typedef std::set<txiter, CompareIteratorByHash> setEntries;
    uint64_t CalculateDescendantMaximum(txiter entry) const EXCLUSIVE_LOCKS_REQUIRED(cs);
private:
    typedef std::map<txiter, setEntries, CompareIteratorByHash> cacheMap;
    void UpdateParent(txiter entry, txiter parent, bool add) EXCLUSIVE_LOCKS_REQUIRED(cs);
    void UpdateChild(txiter entry, txiter child, bool add) EXCLUSIVE_LOCKS_REQUIRED(cs);
    std::vector<indexed_transaction_set::const_iterator> GetSortedDepthAndScore() const EXCLUSIVE_LOCKS_REQUIRED(cs);
    std::set<uint256> m_unbroadcast_txids GUARDED_BY(cs);
    bool CalculateAncestorsAndCheckLimits(size_t entry_size,
                                          size_t entry_count,
                                          setEntries& setAncestors,
                                          CTxMemPoolEntry::Parents &staged_ancestors,
                                          uint64_t limitAncestorCount,
                                          uint64_t limitAncestorSize,
                                          uint64_t limitDescendantCount,
                                          uint64_t limitDescendantSize,
                                          std::string &errString) const EXCLUSIVE_LOCKS_REQUIRED(cs);
public:
    indirectmap<COutPoint, const CTransaction*> mapNextTx GUARDED_BY(cs);
    std::map<uint256, CAmount> mapDeltas GUARDED_BY(cs);
    using Options = kernel::MemPoolOptions;
    const int64_t m_max_size_bytes;
    const std::chrono::seconds m_expiry;
    const CFeeRate m_incremental_relay_feerate;
    const CFeeRate m_min_relay_feerate;
    const CFeeRate m_dust_relay_feerate;
    const bool m_permit_bare_multisig;
    const std::optional<unsigned> m_max_datacarrier_bytes;
    const bool m_require_standard;
    const bool m_full_rbf;
    using Limits = kernel::MemPoolLimits;
    const Limits m_limits;
    explicit CTxMemPool(const Options& opts);
    void check(const CCoinsViewCache& active_coins_tip, int64_t spendheight) const EXCLUSIVE_LOCKS_REQUIRED(::cs_main);
    void addUnchecked(const CTxMemPoolEntry& entry, bool validFeeEstimate = true) EXCLUSIVE_LOCKS_REQUIRED(cs, cs_main);
    void addUnchecked(const CTxMemPoolEntry& entry, setEntries& setAncestors, bool validFeeEstimate = true) EXCLUSIVE_LOCKS_REQUIRED(cs, cs_main);
    void removeRecursive(const CTransaction& tx, MemPoolRemovalReason reason) EXCLUSIVE_LOCKS_REQUIRED(cs);
    void removeForReorg(CChain& chain, std::function<bool(txiter)> filter_final_and_mature) EXCLUSIVE_LOCKS_REQUIRED(cs, cs_main);
    void removeConflicts(const CTransaction& tx) EXCLUSIVE_LOCKS_REQUIRED(cs);
    void removeForBlock(const std::vector<CTransactionRef>& vtx, unsigned int nBlockHeight) EXCLUSIVE_LOCKS_REQUIRED(cs);
    void clear();
    void _clear() EXCLUSIVE_LOCKS_REQUIRED(cs);
    bool CompareDepthAndScore(const uint256& hasha, const uint256& hashb, bool wtxid=false);
    void queryHashes(std::vector<uint256>& vtxid) const;
    bool isSpent(const COutPoint& outpoint) const;
    unsigned int GetTransactionsUpdated() const;
    void AddTransactionsUpdated(unsigned int n);
    bool HasNoInputsOf(const CTransaction& tx) const EXCLUSIVE_LOCKS_REQUIRED(cs);
    void PrioritiseTransaction(const uint256& hash, const CAmount& nFeeDelta);
    void ApplyDelta(const uint256& hash, CAmount &nFeeDelta) const EXCLUSIVE_LOCKS_REQUIRED(cs);
    void ClearPrioritisation(const uint256& hash) EXCLUSIVE_LOCKS_REQUIRED(cs);
    const CTransaction* GetConflictTx(const COutPoint& prevout) const EXCLUSIVE_LOCKS_REQUIRED(cs);
    std::optional<txiter> GetIter(const uint256& txid) const EXCLUSIVE_LOCKS_REQUIRED(cs);
    setEntries GetIterSet(const std::set<uint256>& hashes) const EXCLUSIVE_LOCKS_REQUIRED(cs);
    void RemoveStaged(setEntries& stage, bool updateDescendants, MemPoolRemovalReason reason) EXCLUSIVE_LOCKS_REQUIRED(cs);
    void UpdateTransactionsFromBlock(const std::vector<uint256>& vHashesToUpdate) EXCLUSIVE_LOCKS_REQUIRED(cs, cs_main) LOCKS_EXCLUDED(m_epoch);
    bool CalculateMemPoolAncestors(const CTxMemPoolEntry& entry, setEntries& setAncestors, uint64_t limitAncestorCount, uint64_t limitAncestorSize, uint64_t limitDescendantCount, uint64_t limitDescendantSize, std::string& errString, bool fSearchForParents = true) const EXCLUSIVE_LOCKS_REQUIRED(cs);
    bool CheckPackageLimits(const Package& package,
                            uint64_t limitAncestorCount,
                            uint64_t limitAncestorSize,
                            uint64_t limitDescendantCount,
                            uint64_t limitDescendantSize,
                            std::string &errString) const EXCLUSIVE_LOCKS_REQUIRED(cs);
    void CalculateDescendants(txiter it, setEntries& setDescendants) const EXCLUSIVE_LOCKS_REQUIRED(cs);
    CFeeRate GetMinFee() const {
        return GetMinFee(m_max_size_bytes);
    }
    void TrimToSize(size_t sizelimit, std::vector<COutPoint>* pvNoSpendsRemaining = nullptr) EXCLUSIVE_LOCKS_REQUIRED(cs);
    int Expire(std::chrono::seconds time) EXCLUSIVE_LOCKS_REQUIRED(cs);
    void GetTransactionAncestry(const uint256& txid, size_t& ancestors, size_t& descendants, size_t* ancestorsize = nullptr, CAmount* ancestorfees = nullptr) const;
    bool GetLoadTried() const;
    void SetLoadTried(bool load_tried);
    unsigned long size() const
    {
        LOCK(cs);
        return mapTx.size();
    }
    uint64_t GetTotalTxSize() const EXCLUSIVE_LOCKS_REQUIRED(cs)
    {
        AssertLockHeld(cs);
        return totalTxSize;
    }
    CAmount GetTotalFee() const EXCLUSIVE_LOCKS_REQUIRED(cs)
    {
        AssertLockHeld(cs);
        return m_total_fee;
    }
    bool exists(const GenTxid& gtxid) const
    {
        LOCK(cs);
        if (gtxid.IsWtxid()) {
            return (mapTx.get<index_by_wtxid>().count(gtxid.GetHash()) != 0);
        }
        return (mapTx.count(gtxid.GetHash()) != 0);
    }
    CTransactionRef get(const uint256& hash) const;
    txiter get_iter_from_wtxid(const uint256& wtxid) const EXCLUSIVE_LOCKS_REQUIRED(cs)
    {
        AssertLockHeld(cs);
        return mapTx.project<0>(mapTx.get<index_by_wtxid>().find(wtxid));
    }
    TxMempoolInfo info(const GenTxid& gtxid) const;
    std::vector<TxMempoolInfo> infoAll() const;
    size_t DynamicMemoryUsage() const;
    void AddUnbroadcastTx(const uint256& txid)
    {
        LOCK(cs);
        if (exists(GenTxid::Txid(txid))) m_unbroadcast_txids.insert(txid);
    };
    void RemoveUnbroadcastTx(const uint256& txid, const bool unchecked = false);
    std::set<uint256> GetUnbroadcastTxs() const
    {
        LOCK(cs);
        return m_unbroadcast_txids;
    }
    bool IsUnbroadcastTx(const uint256& txid) const EXCLUSIVE_LOCKS_REQUIRED(cs)
    {
        AssertLockHeld(cs);
        return m_unbroadcast_txids.count(txid) != 0;
    }
    uint64_t GetAndIncrementSequence() const EXCLUSIVE_LOCKS_REQUIRED(cs) {
        return m_sequence_number++;
    }
    uint64_t GetSequence() const EXCLUSIVE_LOCKS_REQUIRED(cs) {
        return m_sequence_number;
    }
private:
    void UpdateForDescendants(txiter updateIt, cacheMap& cachedDescendants,
                              const std::set<uint256>& setExclude, std::set<uint256>& descendants_to_remove) EXCLUSIVE_LOCKS_REQUIRED(cs);
    void UpdateAncestorsOf(bool add, txiter hash, setEntries &setAncestors) EXCLUSIVE_LOCKS_REQUIRED(cs);
    void UpdateEntryForAncestors(txiter it, const setEntries &setAncestors) EXCLUSIVE_LOCKS_REQUIRED(cs);
    void UpdateForRemoveFromMempool(const setEntries &entriesToRemove, bool updateDescendants) EXCLUSIVE_LOCKS_REQUIRED(cs);
    void UpdateChildrenForRemoval(txiter entry) EXCLUSIVE_LOCKS_REQUIRED(cs);
    void removeUnchecked(txiter entry, MemPoolRemovalReason reason) EXCLUSIVE_LOCKS_REQUIRED(cs);
public:
    bool visited(const txiter it) const EXCLUSIVE_LOCKS_REQUIRED(cs, m_epoch)
    {
        return m_epoch.visited(it->m_epoch_marker);
    }
    bool visited(std::optional<txiter> it) const EXCLUSIVE_LOCKS_REQUIRED(cs, m_epoch)
    {
        assert(m_epoch.guarded());
        return !it || visited(*it);
    }
};
class CCoinsViewMemPool : public CCoinsViewBacked
{
    std::unordered_map<COutPoint, Coin, SaltedOutpointHasher> m_temp_added;
protected:
    const CTxMemPool& mempool;
public:
    CCoinsViewMemPool(CCoinsView* baseIn, const CTxMemPool& mempoolIn);
    bool GetCoin(const COutPoint &outpoint, Coin &coin) const override;
    void PackageAddTransaction(const CTransactionRef& tx);
};
struct txid_index {};
struct insertion_order {};
struct DisconnectedBlockTransactions {
    typedef boost::multi_index_container<
        CTransactionRef,
        boost::multi_index::indexed_by<
            boost::multi_index::hashed_unique<
                boost::multi_index::tag<txid_index>,
                mempoolentry_txid,
                SaltedTxidHasher
            >,
            boost::multi_index::sequenced<
                boost::multi_index::tag<insertion_order>
            >
        >
    > indexed_disconnected_transactions;
    ~DisconnectedBlockTransactions() { assert(queuedTx.empty()); }
    indexed_disconnected_transactions queuedTx;
    uint64_t cachedInnerUsage = 0;
    size_t DynamicMemoryUsage() const {
        return memusage::MallocUsage(sizeof(CTransactionRef) + 6 * sizeof(void*)) * queuedTx.size() + cachedInnerUsage;
    }
    void addTransaction(const CTransactionRef& tx)
    {
        queuedTx.insert(tx);
        cachedInnerUsage += RecursiveDynamicUsage(tx);
    }
    void removeForBlock(const std::vector<CTransactionRef>& vtx)
    {
        if (queuedTx.empty()) {
            return;
        }
        for (auto const &tx : vtx) {
            auto it = queuedTx.find(tx->GetHash());
            if (it != queuedTx.end()) {
                cachedInnerUsage -= RecursiveDynamicUsage(*it);
                queuedTx.erase(it);
            }
        }
    }
    void removeEntry(indexed_disconnected_transactions::index<insertion_order>::type::iterator entry)
    {
        cachedInnerUsage -= RecursiveDynamicUsage(*entry);
        queuedTx.get<insertion_order>().erase(entry);
    }
    void clear()
    {
        cachedInnerUsage = 0;
        queuedTx.clear();
    }
};
#endif

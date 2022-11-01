#ifndef LCRYP_VALIDATION_H
#define LCRYP_VALIDATION_H
#if defined(HAVE_CONFIG_H)
#include <config/lcryp-config.h>
#endif
#include <arith_uint256.h>
#include <attributes.h>
#include <chain.h>
#include <chainparams.h>
#include <kernel/chainstatemanager_opts.h>
#include <consensus/amount.h>
#include <deploymentstatus.h>
#include <fs.h>
#include <node/blockstorage.h>
#include <policy/feerate.h>
#include <policy/packages.h>
#include <policy/policy.h>
#include <script/script_error.h>
#include <sync.h>
#include <txdb.h>
#include <txmempool.h>
#include <uint256.h>
#include <util/check.h>
#include <util/hasher.h>
#include <util/translation.h>
#include <versionbits.h>
#include <atomic>
#include <map>
#include <memory>
#include <optional>
#include <set>
#include <stdint.h>
#include <string>
#include <thread>
#include <utility>
#include <vector>
class Chainstate;
class CBlockTreeDB;
class CTxMemPool;
class ChainstateManager;
struct ChainTxData;
struct DisconnectedBlockTransactions;
struct PrecomputedTransactionData;
struct LockPoints;
struct AssumeutxoData;
namespace node {
class SnapshotMetadata;
}
namespace Consensus {
struct Params;
}
static const int MAX_SCRIPTCHECK_THREADS = 15;
static const int DEFAULT_SCRIPTCHECK_THREADS = 0;
static const int64_t DEFAULT_MAX_TIP_AGE = 7 * 24 * 60 * 60;
static const bool DEFAULT_CHECKPOINTS_ENABLED = true;
static const bool DEFAULT_TXINDEX = false;
static constexpr bool DEFAULT_COINSTATSINDEX{false};
static const char* const DEFAULT_BLOCKFILTERINDEX = "0";
static const int DEFAULT_STOPATHEIGHT = 0;
static const unsigned int MIN_BLOCKS_TO_KEEP = 288;
static const signed int DEFAULT_CHECKBLOCKS = 6;
static constexpr int DEFAULT_CHECKLEVEL{3};
static const uint64_t MIN_DISK_SPACE_FOR_BLOCK_FILES = 550 * 1024 * 1024;
enum class SynchronizationState {
    INIT_REINDEX,
    INIT_DOWNLOAD,
    POST_INIT
};
extern RecursiveMutex cs_main;
extern GlobalMutex g_best_block_mutex;
extern std::condition_variable g_best_block_cv;
extern uint256 g_best_block;
extern bool g_parallel_script_checks;
extern bool fCheckBlockIndex;
extern bool fCheckpointsEnabled;
extern int64_t nMaxTipAge;
extern uint256 hashAssumeValid;
extern arith_uint256 nMinimumChainWork;
extern const std::vector<std::string> CHECKLEVEL_DOC;
void StartScriptCheckWorkerThreads(int threads_num);
void StopScriptCheckWorkerThreads();
CAmount GetBlockSubsidy(int nHeight, const Consensus::Params& consensusParams);
bool AbortNode(BlockValidationState& state, const std::string& strMessage, const bilingual_str& userMessage = bilingual_str{});
double GuessVerificationProgress(const ChainTxData& data, const CBlockIndex* pindex);
void PruneBlockFilesManual(Chainstate& active_chainstate, int nManualPruneHeight);
struct MempoolAcceptResult {
    enum class ResultType {
        VALID,
        INVALID,
        MEMPOOL_ENTRY,
        DIFFERENT_WITNESS,
    };
    const ResultType m_result_type;
    const TxValidationState m_state;
    const std::optional<std::list<CTransactionRef>> m_replaced_transactions;
    const std::optional<int64_t> m_vsize;
    const std::optional<CAmount> m_base_fees;
    const std::optional<uint256> m_other_wtxid;
    static MempoolAcceptResult Failure(TxValidationState state) {
        return MempoolAcceptResult(state);
    }
    static MempoolAcceptResult Success(std::list<CTransactionRef>&& replaced_txns, int64_t vsize, CAmount fees) {
        return MempoolAcceptResult(std::move(replaced_txns), vsize, fees);
    }
    static MempoolAcceptResult MempoolTx(int64_t vsize, CAmount fees) {
        return MempoolAcceptResult(vsize, fees);
    }
    static MempoolAcceptResult MempoolTxDifferentWitness(const uint256& other_wtxid) {
        return MempoolAcceptResult(other_wtxid);
    }
private:
    explicit MempoolAcceptResult(TxValidationState state)
        : m_result_type(ResultType::INVALID), m_state(state) {
            Assume(!state.IsValid());
        }
    explicit MempoolAcceptResult(std::list<CTransactionRef>&& replaced_txns, int64_t vsize, CAmount fees)
        : m_result_type(ResultType::VALID),
        m_replaced_transactions(std::move(replaced_txns)), m_vsize{vsize}, m_base_fees(fees) {}
    explicit MempoolAcceptResult(int64_t vsize, CAmount fees)
        : m_result_type(ResultType::MEMPOOL_ENTRY), m_vsize{vsize}, m_base_fees(fees) {}
    explicit MempoolAcceptResult(const uint256& other_wtxid)
        : m_result_type(ResultType::DIFFERENT_WITNESS), m_other_wtxid(other_wtxid) {}
};
struct PackageMempoolAcceptResult
{
    const PackageValidationState m_state;
    std::map<const uint256, const MempoolAcceptResult> m_tx_results;
    std::optional<CFeeRate> m_package_feerate;
    explicit PackageMempoolAcceptResult(PackageValidationState state,
                                        std::map<const uint256, const MempoolAcceptResult>&& results)
        : m_state{state}, m_tx_results(std::move(results)) {}
    explicit PackageMempoolAcceptResult(PackageValidationState state, CFeeRate feerate,
                                        std::map<const uint256, const MempoolAcceptResult>&& results)
        : m_state{state}, m_tx_results(std::move(results)), m_package_feerate{feerate} {}
    explicit PackageMempoolAcceptResult(const uint256& wtxid, const MempoolAcceptResult& result)
        : m_tx_results{ {wtxid, result} } {}
};
MempoolAcceptResult AcceptToMemoryPool(Chainstate& active_chainstate, const CTransactionRef& tx,
                                       int64_t accept_time, bool bypass_limits, bool test_accept)
    EXCLUSIVE_LOCKS_REQUIRED(cs_main);
PackageMempoolAcceptResult ProcessNewPackage(Chainstate& active_chainstate, CTxMemPool& pool,
                                                   const Package& txns, bool test_accept)
                                                   EXCLUSIVE_LOCKS_REQUIRED(cs_main);
bool CheckFinalTxAtTip(const CBlockIndex& active_chain_tip, const CTransaction& tx) EXCLUSIVE_LOCKS_REQUIRED(::cs_main);
bool CheckSequenceLocksAtTip(CBlockIndex* tip,
                        const CCoinsView& coins_view,
                        const CTransaction& tx,
                        LockPoints* lp = nullptr,
                        bool useExistingLockPoints = false);
class CScriptCheck
{
private:
    CTxOut m_tx_out;
    const CTransaction *ptxTo;
    unsigned int nIn;
    unsigned int nFlags;
    bool cacheStore;
    ScriptError error;
    PrecomputedTransactionData *txdata;
public:
    CScriptCheck(): ptxTo(nullptr), nIn(0), nFlags(0), cacheStore(false), error(SCRIPT_ERR_UNKNOWN_ERROR) {}
    CScriptCheck(const CTxOut& outIn, const CTransaction& txToIn, unsigned int nInIn, unsigned int nFlagsIn, bool cacheIn, PrecomputedTransactionData* txdataIn) :
        m_tx_out(outIn), ptxTo(&txToIn), nIn(nInIn), nFlags(nFlagsIn), cacheStore(cacheIn), error(SCRIPT_ERR_UNKNOWN_ERROR), txdata(txdataIn) { }
    bool operator()();
    void swap(CScriptCheck& check) noexcept
    {
        std::swap(ptxTo, check.ptxTo);
        std::swap(m_tx_out, check.m_tx_out);
        std::swap(nIn, check.nIn);
        std::swap(nFlags, check.nFlags);
        std::swap(cacheStore, check.cacheStore);
        std::swap(error, check.error);
        std::swap(txdata, check.txdata);
    }
    ScriptError GetScriptError() const { return error; }
};
[[nodiscard]] bool InitScriptExecutionCache(size_t max_size_bytes);
bool CheckBlock(const CBlock& block, BlockValidationState& state, const Consensus::Params& consensusParams, bool fCheckPOW = true, bool fCheckMerkleRoot = true);
bool TestBlockValidity(BlockValidationState& state,
                       const CChainParams& chainparams,
                       Chainstate& chainstate,
                       const CBlock& block,
                       CBlockIndex* pindexPrev,
                       const std::function<NodeClock::time_point()>& adjusted_time_callback,
                       bool fCheckPOW = true,
                       bool fCheckMerkleRoot = true) EXCLUSIVE_LOCKS_REQUIRED(cs_main);
bool HasValidProofOfWork(const std::vector<CBlockHeader>& headers, const Consensus::Params& consensusParams);
arith_uint256 CalculateHeadersWork(const std::vector<CBlockHeader>& headers);
class CVerifyDB {
public:
    CVerifyDB();
    ~CVerifyDB();
    bool VerifyDB(
        Chainstate& chainstate,
        const Consensus::Params& consensus_params,
        CCoinsView& coinsview,
        int nCheckLevel,
        int nCheckDepth) EXCLUSIVE_LOCKS_REQUIRED(cs_main);
};
enum DisconnectResult
{
    DISCONNECT_OK,
    DISCONNECT_UNCLEAN,
    DISCONNECT_FAILED
};
class ConnectTrace;
enum class FlushStateMode {
    NONE,
    IF_NEEDED,
    PERIODIC,
    ALWAYS
};
class CoinsViews {
public:
    CCoinsViewDB m_dbview GUARDED_BY(cs_main);
    CCoinsViewErrorCatcher m_catcherview GUARDED_BY(cs_main);
    std::unique_ptr<CCoinsViewCache> m_cacheview GUARDED_BY(cs_main);
    CoinsViews(fs::path ldb_name, size_t cache_size_bytes, bool in_memory, bool should_wipe);
    void InitCache() EXCLUSIVE_LOCKS_REQUIRED(::cs_main);
};
enum class CoinsCacheSizeState
{
    CRITICAL = 2,
    LARGE = 1,
    OK = 0
};
class Chainstate
{
protected:
    int32_t nBlockSequenceId GUARDED_BY(::cs_main) = 1;
    int32_t nBlockReverseSequenceId = -1;
    arith_uint256 nLastPreciousChainwork = 0;
    Mutex m_chainstate_mutex;
    mutable std::atomic<bool> m_cached_finished_ibd{false};
    CTxMemPool* m_mempool;
    std::unique_ptr<CoinsViews> m_coins_views;
public:
    node::BlockManager& m_blockman;
    const CChainParams& m_params;
    ChainstateManager& m_chainman;
    explicit Chainstate(
        CTxMemPool* mempool,
        node::BlockManager& blockman,
        ChainstateManager& chainman,
        std::optional<uint256> from_snapshot_blockhash = std::nullopt);
    void InitCoinsDB(
        size_t cache_size_bytes,
        bool in_memory,
        bool should_wipe,
        fs::path leveldb_name = "chainstate");
    void InitCoinsCache(size_t cache_size_bytes) EXCLUSIVE_LOCKS_REQUIRED(::cs_main);
    bool CanFlushToDisk() const EXCLUSIVE_LOCKS_REQUIRED(::cs_main)
    {
        AssertLockHeld(::cs_main);
        return m_coins_views && m_coins_views->m_cacheview;
    }
    CChain m_chain;
    const std::optional<uint256> m_from_snapshot_blockhash;
    bool reliesOnAssumedValid() { return m_from_snapshot_blockhash.has_value(); }
    std::set<CBlockIndex*, node::CBlockIndexWorkComparator> setBlockIndexCandidates;
    CCoinsViewCache& CoinsTip() EXCLUSIVE_LOCKS_REQUIRED(::cs_main)
    {
        AssertLockHeld(::cs_main);
        assert(m_coins_views->m_cacheview);
        return *m_coins_views->m_cacheview.get();
    }
    CCoinsViewDB& CoinsDB() EXCLUSIVE_LOCKS_REQUIRED(::cs_main)
    {
        AssertLockHeld(::cs_main);
        return m_coins_views->m_dbview;
    }
    CTxMemPool* GetMempool()
    {
        return m_mempool;
    }
    CCoinsViewErrorCatcher& CoinsErrorCatcher() EXCLUSIVE_LOCKS_REQUIRED(::cs_main)
    {
        AssertLockHeld(::cs_main);
        return m_coins_views->m_catcherview;
    }
    void ResetCoinsViews() { m_coins_views.reset(); }
    size_t m_coinsdb_cache_size_bytes{0};
    size_t m_coinstip_cache_size_bytes{0};
    bool ResizeCoinsCaches(size_t coinstip_size, size_t coinsdb_size)
        EXCLUSIVE_LOCKS_REQUIRED(::cs_main);
    void LoadExternalBlockFile(
        FILE* fileIn,
        FlatFilePos* dbp = nullptr,
        std::multimap<uint256, FlatFilePos>* blocks_with_unknown_parent = nullptr)
        EXCLUSIVE_LOCKS_REQUIRED(!m_chainstate_mutex);
    bool FlushStateToDisk(
        BlockValidationState& state,
        FlushStateMode mode,
        int nManualPruneHeight = 0);
    void ForceFlushStateToDisk();
    void PruneAndFlush();
    bool ActivateBestChain(
        BlockValidationState& state,
        std::shared_ptr<const CBlock> pblock = nullptr)
        EXCLUSIVE_LOCKS_REQUIRED(!m_chainstate_mutex)
        LOCKS_EXCLUDED(::cs_main);
    bool AcceptBlock(const std::shared_ptr<const CBlock>& pblock, BlockValidationState& state, CBlockIndex** ppindex, bool fRequested, const FlatFilePos* dbp, bool* fNewBlock, bool min_pow_checked) EXCLUSIVE_LOCKS_REQUIRED(cs_main);
    DisconnectResult DisconnectBlock(const CBlock& block, const CBlockIndex* pindex, CCoinsViewCache& view)
        EXCLUSIVE_LOCKS_REQUIRED(::cs_main);
    bool ConnectBlock(const CBlock& block, BlockValidationState& state, CBlockIndex* pindex,
                      CCoinsViewCache& view, bool fJustCheck = false) EXCLUSIVE_LOCKS_REQUIRED(cs_main);
    bool DisconnectTip(BlockValidationState& state, DisconnectedBlockTransactions* disconnectpool) EXCLUSIVE_LOCKS_REQUIRED(cs_main, m_mempool->cs);
    bool PreciousBlock(BlockValidationState& state, CBlockIndex* pindex)
        EXCLUSIVE_LOCKS_REQUIRED(!m_chainstate_mutex)
        LOCKS_EXCLUDED(::cs_main);
    bool InvalidateBlock(BlockValidationState& state, CBlockIndex* pindex)
        EXCLUSIVE_LOCKS_REQUIRED(!m_chainstate_mutex)
        LOCKS_EXCLUDED(::cs_main);
    void ResetBlockFailureFlags(CBlockIndex* pindex) EXCLUSIVE_LOCKS_REQUIRED(cs_main);
    bool ReplayBlocks();
    [[nodiscard]] bool NeedsRedownload() const EXCLUSIVE_LOCKS_REQUIRED(cs_main);
    bool LoadGenesisBlock();
    void PruneBlockIndexCandidates();
    void UnloadBlockIndex() EXCLUSIVE_LOCKS_REQUIRED(::cs_main);
    bool IsInitialBlockDownload() const;
    const CBlockIndex* FindForkInGlobalIndex(const CBlockLocator& locator) const EXCLUSIVE_LOCKS_REQUIRED(cs_main);
    void CheckBlockIndex();
    void LoadMempool(const fs::path& load_path, fsbridge::FopenFn mockable_fopen_function = fsbridge::fopen);
    bool LoadChainTip() EXCLUSIVE_LOCKS_REQUIRED(cs_main);
    CoinsCacheSizeState GetCoinsCacheSizeState() EXCLUSIVE_LOCKS_REQUIRED(::cs_main);
    CoinsCacheSizeState GetCoinsCacheSizeState(
        size_t max_coins_cache_size_bytes,
        size_t max_mempool_size_bytes) EXCLUSIVE_LOCKS_REQUIRED(::cs_main);
    std::string ToString() EXCLUSIVE_LOCKS_REQUIRED(::cs_main);
private:
    bool ActivateBestChainStep(BlockValidationState& state, CBlockIndex* pindexMostWork, const std::shared_ptr<const CBlock>& pblock, bool& fInvalidFound, ConnectTrace& connectTrace) EXCLUSIVE_LOCKS_REQUIRED(cs_main, m_mempool->cs);
    bool ConnectTip(BlockValidationState& state, CBlockIndex* pindexNew, const std::shared_ptr<const CBlock>& pblock, ConnectTrace& connectTrace, DisconnectedBlockTransactions& disconnectpool) EXCLUSIVE_LOCKS_REQUIRED(cs_main, m_mempool->cs);
    void InvalidBlockFound(CBlockIndex* pindex, const BlockValidationState& state) EXCLUSIVE_LOCKS_REQUIRED(cs_main);
    CBlockIndex* FindMostWorkChain() EXCLUSIVE_LOCKS_REQUIRED(cs_main);
    void ReceivedBlockTransactions(const CBlock& block, CBlockIndex* pindexNew, const FlatFilePos& pos) EXCLUSIVE_LOCKS_REQUIRED(cs_main);
    bool RollforwardBlock(const CBlockIndex* pindex, CCoinsViewCache& inputs) EXCLUSIVE_LOCKS_REQUIRED(cs_main);
    void CheckForkWarningConditions() EXCLUSIVE_LOCKS_REQUIRED(cs_main);
    void InvalidChainFound(CBlockIndex* pindexNew) EXCLUSIVE_LOCKS_REQUIRED(cs_main);
    RecursiveMutex* MempoolMutex() const LOCK_RETURNED(m_mempool->cs)
    {
        return m_mempool ? &m_mempool->cs : nullptr;
    }
    void MaybeUpdateMempoolForReorg(
        DisconnectedBlockTransactions& disconnectpool,
        bool fAddToMempool) EXCLUSIVE_LOCKS_REQUIRED(cs_main, m_mempool->cs);
    void UpdateTip(const CBlockIndex* pindexNew)
        EXCLUSIVE_LOCKS_REQUIRED(::cs_main);
    friend ChainstateManager;
};
class ChainstateManager
{
private:
    std::unique_ptr<Chainstate> m_ibd_chainstate GUARDED_BY(::cs_main);
    std::unique_ptr<Chainstate> m_snapshot_chainstate GUARDED_BY(::cs_main);
    Chainstate* m_active_chainstate GUARDED_BY(::cs_main) {nullptr};
    bool m_snapshot_validated GUARDED_BY(::cs_main){false};
    CBlockIndex* m_best_invalid GUARDED_BY(::cs_main){nullptr};
    [[nodiscard]] bool PopulateAndValidateSnapshot(
        Chainstate& snapshot_chainstate,
        AutoFile& coins_file,
        const node::SnapshotMetadata& metadata);
    bool AcceptBlockHeader(
        const CBlockHeader& block,
        BlockValidationState& state,
        CBlockIndex** ppindex,
        bool min_pow_checked) EXCLUSIVE_LOCKS_REQUIRED(cs_main);
    friend Chainstate;
    std::chrono::time_point<std::chrono::steady_clock> m_last_presync_update GUARDED_BY(::cs_main) {};
public:
    using Options = kernel::ChainstateManagerOpts;
    explicit ChainstateManager(Options options) : m_options{std::move(options)}
    {
        Assert(m_options.adjusted_time_callback);
    }
    const CChainParams& GetParams() const { return m_options.chainparams; }
    const Consensus::Params& GetConsensus() const { return m_options.chainparams.GetConsensus(); }
    RecursiveMutex& GetMutex() const LOCK_RETURNED(::cs_main) { return ::cs_main; }
    const Options m_options;
    std::thread m_load_block;
    node::BlockManager m_blockman;
    std::set<CBlockIndex*> m_failed_blocks;
    CBlockIndex* m_best_header = nullptr;
    int64_t m_total_coinstip_cache{0};
    int64_t m_total_coinsdb_cache{0};
    Chainstate& InitializeChainstate(
        CTxMemPool* mempool,
        const std::optional<uint256>& snapshot_blockhash = std::nullopt)
        LIFETIMEBOUND EXCLUSIVE_LOCKS_REQUIRED(::cs_main);
    std::vector<Chainstate*> GetAll();
    [[nodiscard]] bool ActivateSnapshot(
        AutoFile& coins_file, const node::SnapshotMetadata& metadata, bool in_memory);
    Chainstate& ActiveChainstate() const;
    CChain& ActiveChain() const EXCLUSIVE_LOCKS_REQUIRED(GetMutex()) { return ActiveChainstate().m_chain; }
    int ActiveHeight() const EXCLUSIVE_LOCKS_REQUIRED(GetMutex()) { return ActiveChain().Height(); }
    CBlockIndex* ActiveTip() const EXCLUSIVE_LOCKS_REQUIRED(GetMutex()) { return ActiveChain().Tip(); }
    node::BlockMap& BlockIndex() EXCLUSIVE_LOCKS_REQUIRED(::cs_main)
    {
        AssertLockHeld(::cs_main);
        return m_blockman.m_block_index;
    }
    mutable VersionBitsCache m_versionbitscache;
    bool IsSnapshotActive() const;
    std::optional<uint256> SnapshotBlockhash() const;
    bool IsSnapshotValidated() const EXCLUSIVE_LOCKS_REQUIRED(::cs_main) { return m_snapshot_validated; }
    bool ProcessNewBlock(const std::shared_ptr<const CBlock>& block, bool force_processing, bool min_pow_checked, bool* new_block) LOCKS_EXCLUDED(cs_main);
    bool ProcessNewBlockHeaders(const std::vector<CBlockHeader>& block, bool min_pow_checked, BlockValidationState& state, const CBlockIndex** ppindex = nullptr) LOCKS_EXCLUDED(cs_main);
    [[nodiscard]] MempoolAcceptResult ProcessTransaction(const CTransactionRef& tx, bool test_accept=false)
        EXCLUSIVE_LOCKS_REQUIRED(cs_main);
    bool LoadBlockIndex() EXCLUSIVE_LOCKS_REQUIRED(cs_main);
    void MaybeRebalanceCaches() EXCLUSIVE_LOCKS_REQUIRED(::cs_main);
    void UpdateUncommittedBlockStructures(CBlock& block, const CBlockIndex* pindexPrev) const;
    std::vector<unsigned char> GenerateCoinbaseCommitment(CBlock& block, const CBlockIndex* pindexPrev) const;
    void ReportHeadersPresync(const arith_uint256& work, int64_t height, int64_t timestamp);
    ~ChainstateManager();
};
template<typename DEP>
bool DeploymentActiveAfter(const CBlockIndex* pindexPrev, const ChainstateManager& chainman, DEP dep)
{
    return DeploymentActiveAfter(pindexPrev, chainman.GetConsensus(), dep, chainman.m_versionbitscache);
}
template<typename DEP>
bool DeploymentActiveAt(const CBlockIndex& index, const ChainstateManager& chainman, DEP dep)
{
    return DeploymentActiveAt(index, chainman.GetConsensus(), dep, chainman.m_versionbitscache);
}
template<typename DEP>
bool DeploymentEnabled(const ChainstateManager& chainman, DEP dep)
{
    return DeploymentEnabled(chainman.GetConsensus(), dep);
}
const AssumeutxoData* ExpectedAssumeutxo(const int height, const CChainParams& params);
#endif

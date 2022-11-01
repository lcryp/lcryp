#ifndef LCRYP_NODE_MINER_H
#define LCRYP_NODE_MINER_H
#include <primitives/block.h>
#include <txmempool.h>
#include <memory>
#include <optional>
#include <stdint.h>
#include <script\script.h>
#include <boost/multi_index/ordered_index.hpp>
#include <boost/multi_index_container.hpp>
#include <any>
#include <univalue.h>
class ChainstateManager;
class CBlockIndex;
class CChainParams;
class CScript;
static UniValue GenerError(const std::string value)
{
    UniValue flag_error(UniValue::VBOOL);
    flag_error.pushKV("error", "true");
    UniValue message_error(UniValue::VSTR);
    message_error.pushKV("message", value);
    UniValue function_name(UniValue::VSTR);
    function_name.pushKV("function", __func__);
    UniValue obj_error(UniValue::VOBJ);
    obj_error.push_back(flag_error);
    obj_error.push_back(message_error);
    obj_error.push_back(function_name);
    return obj_error;
}
namespace Consensus { struct Params; };
static const bool DEFAULT_GENERATE = false;
static const int DEFAULT_GENERATE_THREADS = 1;
namespace node {
static const bool DEFAULT_PRINTPRIORITY = false;
struct CBlockTemplate
{
    CBlock block;
    std::vector<CAmount> vTxFees;
    std::vector<int64_t> vTxSigOpsCost;
    std::vector<unsigned char> vchCoinbaseCommitment;
};
struct CTxMemPoolModifiedEntry {
    explicit CTxMemPoolModifiedEntry(CTxMemPool::txiter entry)
    {
        iter = entry;
        nSizeWithAncestors = entry->GetSizeWithAncestors();
        nModFeesWithAncestors = entry->GetModFeesWithAncestors();
        nSigOpCostWithAncestors = entry->GetSigOpCostWithAncestors();
    }
    CAmount GetModifiedFee() const { return iter->GetModifiedFee(); }
    uint64_t GetSizeWithAncestors() const { return nSizeWithAncestors; }
    CAmount GetModFeesWithAncestors() const { return nModFeesWithAncestors; }
    size_t GetTxSize() const { return iter->GetTxSize(); }
    const CTransaction& GetTx() const { return iter->GetTx(); }
    CTxMemPool::txiter iter;
    uint64_t nSizeWithAncestors;
    CAmount nModFeesWithAncestors;
    int64_t nSigOpCostWithAncestors;
};
struct CompareCTxMemPoolIter {
    bool operator()(const CTxMemPool::txiter& a, const CTxMemPool::txiter& b) const
    {
        return &(*a) < &(*b);
    }
};
struct modifiedentry_iter {
    typedef CTxMemPool::txiter result_type;
    result_type operator() (const CTxMemPoolModifiedEntry &entry) const
    {
        return entry.iter;
    }
};
struct CompareTxIterByAncestorCount {
    bool operator()(const CTxMemPool::txiter& a, const CTxMemPool::txiter& b) const
    {
        if (a->GetCountWithAncestors() != b->GetCountWithAncestors()) {
            return a->GetCountWithAncestors() < b->GetCountWithAncestors();
        }
        return CompareIteratorByHash()(a, b);
    }
};
typedef boost::multi_index_container<
    CTxMemPoolModifiedEntry,
    boost::multi_index::indexed_by<
        boost::multi_index::ordered_unique<
            modifiedentry_iter,
            CompareCTxMemPoolIter
        >,
        boost::multi_index::ordered_non_unique<
            boost::multi_index::tag<ancestor_score>,
            boost::multi_index::identity<CTxMemPoolModifiedEntry>,
            CompareTxMemPoolEntryByAncestorFee
        >
    >
> indexed_modified_transaction_set;
typedef indexed_modified_transaction_set::nth_index<0>::type::iterator modtxiter;
typedef indexed_modified_transaction_set::index<ancestor_score>::type::iterator modtxscoreiter;
struct update_for_parent_inclusion
{
    explicit update_for_parent_inclusion(CTxMemPool::txiter it) : iter(it) {}
    void operator() (CTxMemPoolModifiedEntry &e)
    {
        e.nModFeesWithAncestors -= iter->GetModifiedFee();
        e.nSizeWithAncestors -= iter->GetTxSize();
        e.nSigOpCostWithAncestors -= iter->GetSigOpCost();
    }
    CTxMemPool::txiter iter;
};
class BlockAssembler
{
private:
    std::unique_ptr<CBlockTemplate> pblocktemplate;
    unsigned int nBlockMaxWeight;
    CFeeRate blockMinFeeRate;
    uint64_t nBlockWeight;
    uint64_t nBlockTx;
    uint64_t nBlockSigOpsCost;
    CAmount nFees;
    CTxMemPool::setEntries inBlock;
    int nHeight;
    int64_t m_lock_time_cutoff;
    const CChainParams& chainparams;
    const CTxMemPool* const m_mempool;
    Chainstate& m_chainstate;
public:
    struct Options {
        Options();
        size_t nBlockMaxWeight;
        CFeeRate blockMinFeeRate;
    };
    explicit BlockAssembler(Chainstate& chainstate, const CTxMemPool* mempool);
    explicit BlockAssembler(Chainstate& chainstate, const CTxMemPool* mempool, const Options& options);
    std::unique_ptr<CBlockTemplate> CreateNewBlock(const CScript& scriptPubKeyIn);
    inline static std::optional<int64_t> m_last_block_num_txs{};
    inline static std::optional<int64_t> m_last_block_weight{};
private:
    void resetBlock();
    void AddToBlock(CTxMemPool::txiter iter);
    void addPackageTxs(const CTxMemPool& mempool, int& nPackagesSelected, int& nDescendantsUpdated) EXCLUSIVE_LOCKS_REQUIRED(mempool.cs);
    void onlyUnconfirmed(CTxMemPool::setEntries& testSet);
    bool TestPackage(uint64_t packageSize, int64_t packageSigOpsCost) const;
    bool TestPackageTransactions(const CTxMemPool::setEntries& package) const;
    void SortForBlock(const CTxMemPool::setEntries& package, std::vector<CTxMemPool::txiter>& sortedEntries);
};
int64_t UpdateTime(CBlockHeader* pblock, const Consensus::Params& consensusParams, const CBlockIndex* pindexPrev);
void RegenerateCommitments(CBlock& block, ChainstateManager& chainman);
#ifdef ENABLE_WALLET
#define LIMIT_CALC_PROFRESS 10000
#define LIMIT_SEC_PROFRESS 180
class BlockMining
{
private:
    static uint32_t random(uint32_t min, uint32_t max);
private:
    static bool IncrementExtraNonce(const uint32_t th, CBlock* pblock, const CBlockIndex* pindexPrev, unsigned int& nExtraNonce);
    static bool ScanHash(const CBlockHeader* pblock, uint32_t& nNonce, uint32_t& countNonce, uint32_t min, uint32_t max, const uint32_t th, uint256* phash);
    static bool ProcessBlockFound(const uint32_t th, const CBlock* pblock, std::any context);
private:
    static void GenerateBlock(uint32_t th, uint32_t it, uint32_t min, uint32_t max, std::any context, const CScript& coinbase_script);
    static UniValue GenerateBlocks(bool fGenerate, int nGenProcLimit, const CTxDestination destination, std::any context);
public:
    static bool Minings(bool fGenerate, int nGenProcLimit, const CTxDestination destination, std::any context);
};
#endif // ENABLE_WALLET
}
#endif

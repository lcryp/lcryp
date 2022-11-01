#include <node/miner.h>
#include <chain.h>
#include <chainparams.h>
#include <coins.h>
#include <span.h>
#include <validationinterface.h>
#include <consensus/amount.h>
#include <consensus/consensus.h>
#include <consensus/merkle.h>
#include <consensus/tx_verify.h>
#include <consensus/validation.h>
#include <deploymentstatus.h>
#include <policy/feerate.h>
#include <key_io.h>
#include <policy/policy.h>
#include <pow.h>
#include <primitives/transaction.h>
#include <timedata.h>
#include <util/moneystr.h>
#include <util/system.h>
#include <validation.h>
#include <algorithm>
#include <netbase.h>
#include <rpc/protocol.h>
#include <rpc/server_util.h>
#include <net.h>
#include <utility>
#include <iostream>
#include <thread>
#include <util/threadnames.h>
#include <univalue.h>
#include <node/context.h>
#include <rpc/request.h>
#include <shutdown.h>
#include <fstream>
#include <any>
using node::NodeContext;
namespace node {
int64_t UpdateTime(CBlockHeader* pblock, const Consensus::Params& consensusParams, const CBlockIndex* pindexPrev)
{
    int64_t nOldTime = pblock->nTime;
    int64_t nNewTime{std::max<int64_t>(pindexPrev->GetMedianTimePast() + 1, TicksSinceEpoch<std::chrono::seconds>(GetAdjustedTime()))};
    if (nOldTime < nNewTime) {
        pblock->nTime = nNewTime;
    }
    if (consensusParams.fPowAllowMinDifficultyBlocks) {
        pblock->nBits = GetNextWorkRequired(pindexPrev, pblock, consensusParams);
    }
    return nNewTime - nOldTime;
}
void RegenerateCommitments(CBlock& block, ChainstateManager& chainman)
{
    CMutableTransaction tx{*block.vtx.at(0)};
    tx.vout.erase(tx.vout.begin() + GetWitnessCommitmentIndex(block));
    block.vtx.at(0) = MakeTransactionRef(tx);
    const CBlockIndex* prev_block = WITH_LOCK(::cs_main, return chainman.m_blockman.LookupBlockIndex(block.hashPrevBlock));
    chainman.GenerateCoinbaseCommitment(block, prev_block);
    block.hashMerkleRoot = BlockMerkleRoot(block);
}
BlockAssembler::Options::Options()
{
    blockMinFeeRate = CFeeRate(DEFAULT_BLOCK_MIN_TX_FEE);
    nBlockMaxWeight = DEFAULT_BLOCK_MAX_WEIGHT;
}
BlockAssembler::BlockAssembler(Chainstate& chainstate, const CTxMemPool* mempool, const Options& options)
    : chainparams{chainstate.m_chainman.GetParams()},
      m_mempool(mempool),
      m_chainstate(chainstate)
{
    blockMinFeeRate = options.blockMinFeeRate;
    nBlockMaxWeight = std::max<size_t>(4000, std::min<size_t>(MAX_BLOCK_WEIGHT - 4000, options.nBlockMaxWeight));
}
static BlockAssembler::Options DefaultOptions()
{
    BlockAssembler::Options options;
    options.nBlockMaxWeight = gArgs.GetIntArg("-blockmaxweight", DEFAULT_BLOCK_MAX_WEIGHT);
    if (gArgs.IsArgSet("-blockmintxfee")) {
        std::optional<CAmount> parsed = ParseMoney(gArgs.GetArg("-blockmintxfee", ""));
        options.blockMinFeeRate = CFeeRate{parsed.value_or(DEFAULT_BLOCK_MIN_TX_FEE)};
    } else {
        options.blockMinFeeRate = CFeeRate{DEFAULT_BLOCK_MIN_TX_FEE};
    }
    return options;
}
BlockAssembler::BlockAssembler(Chainstate& chainstate, const CTxMemPool* mempool)
    : BlockAssembler(chainstate, mempool, DefaultOptions()) {}
void BlockAssembler::resetBlock()
{
    inBlock.clear();
    nBlockWeight = 4000;
    nBlockSigOpsCost = 400;
    nBlockTx = 0;
    nFees = 0;
}
std::unique_ptr<CBlockTemplate> BlockAssembler::CreateNewBlock(const CScript& scriptPubKeyIn)
{
    int64_t nTimeStart = GetTimeMicros();
    resetBlock();
    pblocktemplate.reset(new CBlockTemplate());
    if (!pblocktemplate.get()) {
        return nullptr;
    }
    CBlock* const pblock = &pblocktemplate->block;
    pblock->vtx.emplace_back();
    pblocktemplate->vTxFees.push_back(-1);
    pblocktemplate->vTxSigOpsCost.push_back(-1);
    LOCK(::cs_main);
    CBlockIndex* pindexPrev = m_chainstate.m_chain.Tip();
    assert(pindexPrev != nullptr);
    nHeight = pindexPrev->nHeight + 1;
    pblock->nVersion = m_chainstate.m_chainman.m_versionbitscache.ComputeBlockVersion(pindexPrev, chainparams.GetConsensus());
    if (chainparams.MineBlocksOnDemand()) {
        pblock->nVersion = gArgs.GetIntArg("-blockversion", pblock->nVersion);
    }
    pblock->nTime = TicksSinceEpoch<std::chrono::seconds>(GetAdjustedTime());
    m_lock_time_cutoff = pindexPrev->GetMedianTimePast();
    int nPackagesSelected = 0;
    int nDescendantsUpdated = 0;
    if (m_mempool) {
        LOCK(m_mempool->cs);
        addPackageTxs(*m_mempool, nPackagesSelected, nDescendantsUpdated);
    }
    int64_t nTime1 = GetTimeMicros();
    m_last_block_num_txs = nBlockTx;
    m_last_block_weight = nBlockWeight;
    CMutableTransaction coinbaseTx;
    coinbaseTx.vin.resize(1);
    coinbaseTx.vin[0].prevout.SetNull();
    coinbaseTx.vout.resize(1);
    coinbaseTx.vout[0].scriptPubKey = scriptPubKeyIn;
    coinbaseTx.vout[0].nValue = nFees + GetBlockSubsidy(nHeight, chainparams.GetConsensus());
    coinbaseTx.vin[0].scriptSig = CScript() << nHeight << OP_0;
    pblock->vtx[0] = MakeTransactionRef(std::move(coinbaseTx));
    pblocktemplate->vchCoinbaseCommitment = m_chainstate.m_chainman.GenerateCoinbaseCommitment(*pblock, pindexPrev);
    pblocktemplate->vTxFees[0] = -nFees;
    LogPrintf("CreateNewBlock(): block weight: %u txs: %u fees: %ld sigops %d\n", GetBlockWeight(*pblock), nBlockTx, nFees, nBlockSigOpsCost);
    pblock->hashPrevBlock  = pindexPrev->GetBlockHash();
    UpdateTime(pblock, chainparams.GetConsensus(), pindexPrev);
    pblock->nBits = GetNextWorkRequired(pindexPrev, pblock, chainparams.GetConsensus());
    pblock->nNonce = 0;
    pblocktemplate->vTxSigOpsCost[0] = WITNESS_SCALE_FACTOR * GetLegacySigOpCount(*pblock->vtx[0]);
    BlockValidationState state;
    if (!TestBlockValidity(state, chainparams, m_chainstate, *pblock, pindexPrev, GetAdjustedTime, false, false)) {
        LogPrintf("%s: TestBlockValidity failed: %s\n", __func__, state.ToString());
        return std::move(pblocktemplate);
    }
    int64_t nTime2 = GetTimeMicros();
    LogPrint(BCLog::BENCH, "CreateNewBlock() packages: %.2fms (%d packages, %d updated descendants), validity: %.2fms (total %.2fms)\n", 0.001 * (nTime1 - nTimeStart), nPackagesSelected, nDescendantsUpdated, 0.001 * (nTime2 - nTime1), 0.001 * (nTime2 - nTimeStart));
    return std::move(pblocktemplate);
}
void BlockAssembler::onlyUnconfirmed(CTxMemPool::setEntries& testSet)
{
    for (CTxMemPool::setEntries::iterator iit = testSet.begin(); iit != testSet.end(); ) {
        if (inBlock.count(*iit)) {
            testSet.erase(iit++);
        } else {
            iit++;
        }
    }
}
bool BlockAssembler::TestPackage(uint64_t packageSize, int64_t packageSigOpsCost) const
{
    if (nBlockWeight + WITNESS_SCALE_FACTOR * packageSize >= nBlockMaxWeight) {
        return false;
    }
    if (nBlockSigOpsCost + packageSigOpsCost >= MAX_BLOCK_SIGOPS_COST) {
        return false;
    }
    return true;
}
bool BlockAssembler::TestPackageTransactions(const CTxMemPool::setEntries& package) const
{
    for (CTxMemPool::txiter it : package) {
        if (!IsFinalTx(it->GetTx(), nHeight, m_lock_time_cutoff)) {
            return false;
        }
    }
    return true;
}
void BlockAssembler::AddToBlock(CTxMemPool::txiter iter)
{
    pblocktemplate->block.vtx.emplace_back(iter->GetSharedTx());
    pblocktemplate->vTxFees.push_back(iter->GetFee());
    pblocktemplate->vTxSigOpsCost.push_back(iter->GetSigOpCost());
    nBlockWeight += iter->GetTxWeight();
    ++nBlockTx;
    nBlockSigOpsCost += iter->GetSigOpCost();
    nFees += iter->GetFee();
    inBlock.insert(iter);
    bool fPrintPriority = gArgs.GetBoolArg("-printpriority", DEFAULT_PRINTPRIORITY);
    if (fPrintPriority) {
        LogPrintf("fee rate %s txid %s\n",
                  CFeeRate(iter->GetModifiedFee(), iter->GetTxSize()).ToString(),
                  iter->GetTx().GetHash().ToString());
    }
}
static int UpdatePackagesForAdded(const CTxMemPool& mempool,
                                  const CTxMemPool::setEntries& alreadyAdded,
                                  indexed_modified_transaction_set& mapModifiedTx) EXCLUSIVE_LOCKS_REQUIRED(mempool.cs)
{
    AssertLockHeld(mempool.cs);
    int nDescendantsUpdated = 0;
    for (CTxMemPool::txiter it : alreadyAdded) {
        CTxMemPool::setEntries descendants;
        mempool.CalculateDescendants(it, descendants);
        for (CTxMemPool::txiter desc : descendants) {
            if (alreadyAdded.count(desc)) {
                continue;
            }
            ++nDescendantsUpdated;
            modtxiter mit = mapModifiedTx.find(desc);
            if (mit == mapModifiedTx.end()) {
                CTxMemPoolModifiedEntry modEntry(desc);
                modEntry.nSizeWithAncestors -= it->GetTxSize();
                modEntry.nModFeesWithAncestors -= it->GetModifiedFee();
                modEntry.nSigOpCostWithAncestors -= it->GetSigOpCost();
                mapModifiedTx.insert(modEntry);
            } else {
                mapModifiedTx.modify(mit, update_for_parent_inclusion(it));
            }
        }
    }
    return nDescendantsUpdated;
}
void BlockAssembler::SortForBlock(const CTxMemPool::setEntries& package, std::vector<CTxMemPool::txiter>& sortedEntries)
{
    sortedEntries.clear();
    sortedEntries.insert(sortedEntries.begin(), package.begin(), package.end());
    std::sort(sortedEntries.begin(), sortedEntries.end(), CompareTxIterByAncestorCount());
}
void BlockAssembler::addPackageTxs(const CTxMemPool& mempool, int& nPackagesSelected, int& nDescendantsUpdated)
{
    AssertLockHeld(mempool.cs);
    indexed_modified_transaction_set mapModifiedTx;
    CTxMemPool::setEntries failedTx;
    CTxMemPool::indexed_transaction_set::index<ancestor_score>::type::iterator mi = mempool.mapTx.get<ancestor_score>().begin();
    CTxMemPool::txiter iter;
    const int64_t MAX_CONSECUTIVE_FAILURES = 1000;
    int64_t nConsecutiveFailed = 0;
    while (mi != mempool.mapTx.get<ancestor_score>().end() || !mapModifiedTx.empty()) {
        if (mi != mempool.mapTx.get<ancestor_score>().end()) {
            auto it = mempool.mapTx.project<0>(mi);
            assert(it != mempool.mapTx.end());
            if (mapModifiedTx.count(it) || inBlock.count(it) || failedTx.count(it)) {
                ++mi;
                continue;
            }
        }
        bool fUsingModified = false;
        modtxscoreiter modit = mapModifiedTx.get<ancestor_score>().begin();
        if (mi == mempool.mapTx.get<ancestor_score>().end()) {
            iter = modit->iter;
            fUsingModified = true;
        } else {
            iter = mempool.mapTx.project<0>(mi);
            if (modit != mapModifiedTx.get<ancestor_score>().end() &&
                    CompareTxMemPoolEntryByAncestorFee()(*modit, CTxMemPoolModifiedEntry(iter))) {
                iter = modit->iter;
                fUsingModified = true;
            } else {
                ++mi;
            }
        }
        assert(!inBlock.count(iter));
        uint64_t packageSize = iter->GetSizeWithAncestors();
        CAmount packageFees = iter->GetModFeesWithAncestors();
        int64_t packageSigOpsCost = iter->GetSigOpCostWithAncestors();
        if (fUsingModified) {
            packageSize = modit->nSizeWithAncestors;
            packageFees = modit->nModFeesWithAncestors;
            packageSigOpsCost = modit->nSigOpCostWithAncestors;
        }
        if (packageFees < blockMinFeeRate.GetFee(packageSize)) {
            return;
        }
        if (!TestPackage(packageSize, packageSigOpsCost)) {
            if (fUsingModified) {
                mapModifiedTx.get<ancestor_score>().erase(modit);
                failedTx.insert(iter);
            }
            ++nConsecutiveFailed;
            if (nConsecutiveFailed > MAX_CONSECUTIVE_FAILURES && nBlockWeight >
                    nBlockMaxWeight - 4000) {
                break;
            }
            continue;
        }
        CTxMemPool::setEntries ancestors;
        uint64_t nNoLimit = std::numeric_limits<uint64_t>::max();
        std::string dummy;
        mempool.CalculateMemPoolAncestors(*iter, ancestors, nNoLimit, nNoLimit, nNoLimit, nNoLimit, dummy, false);
        onlyUnconfirmed(ancestors);
        ancestors.insert(iter);
        if (!TestPackageTransactions(ancestors)) {
            if (fUsingModified) {
                mapModifiedTx.get<ancestor_score>().erase(modit);
                failedTx.insert(iter);
            }
            continue;
        }
        nConsecutiveFailed = 0;
        std::vector<CTxMemPool::txiter> sortedEntries;
        SortForBlock(ancestors, sortedEntries);
        for (size_t i = 0; i < sortedEntries.size(); ++i) {
            AddToBlock(sortedEntries[i]);
            mapModifiedTx.erase(sortedEntries[i]);
        }
        ++nPackagesSelected;
        nDescendantsUpdated += UpdatePackagesForAdded(mempool, ancestors, mapModifiedTx);
    }
}
//
// Internal miner
//
#ifdef ENABLE_WALLET
std::mutex g_lock;
std::mutex g_lock_print;
bool m_flag_mining_busy = false;
uint32_t CounterBlok = 0;
std::vector<bool> m_flag_active_threads;
std::vector<double> m_flag_progress_threads;
std::vector<uint256> m_hashPrevBlock;
uint32_t BlockMining::random(uint32_t min, uint32_t max) {
    return min + rand() % (max - min);
}
bool BlockMining::IncrementExtraNonce(const uint32_t th, CBlock* pblock, const CBlockIndex* pindexPrev, unsigned int& nExtraNonce)
{
    try {
        if (m_hashPrevBlock[th] != pblock->hashPrevBlock) {
            nExtraNonce = 0;
            m_hashPrevBlock[th] = pblock->hashPrevBlock;
        }
        ++nExtraNonce;
        unsigned int nHeight = pindexPrev->nHeight + 1;
        CMutableTransaction txCoinbase(*pblock->vtx[0]);
        txCoinbase.vin[0].scriptSig = (CScript() << nHeight << CScriptNum(nExtraNonce));
        assert(txCoinbase.vin[0].scriptSig.size() <= 100);
        pblock->vtx[0] = MakeTransactionRef(std::move(txCoinbase));
        pblock->hashMerkleRoot = BlockMerkleRoot(*pblock);
        return true;
    }
    catch (const std::runtime_error& e)
    {
        {
            std::lock_guard lock(g_lock);
            m_flag_mining_busy = false;
            m_flag_active_threads[th] = true;
            LogPrintf("Runtime mining error: %s [%s:%s:%u]\n", e.what(), __FILE__, __func__, __LINE__);
        }
        return false;
    }
}
bool BlockMining::ScanHash(const CBlockHeader* pblock, uint32_t& nNonce, uint32_t& countNonce, uint32_t min, uint32_t max, const uint32_t th, uint256* phash)
{
    try {
        CHash256 hasher;
        CDataStream ss(SER_NETWORK, PROTOCOL_VERSION);
        ss << *pblock;
        assert(ss.size() == 80);
        std::string desc;
        hasher.WriteA((unsigned char*)&ss[0], 76);
        while (true) {
            nNonce++;
            countNonce++;
            if ((countNonce / LIMIT_CALC_PROFRESS) * LIMIT_CALC_PROFRESS == countNonce)
            {
                double one_pr = (double)(max - min) / (double)100;
                double progress = 100;
                if ((double)one_pr != 0)
                    progress = (double)countNonce / (double)one_pr;
                {
                    std::lock_guard lock(g_lock);
                    m_flag_progress_threads[th] = progress;
                }
            }
            CHash256(hasher).WriteA((unsigned char*)&nNonce, 4).FinalizeA((unsigned char*)phash);
            if (((uint16_t*)phash)[15] == 0)
                return true;
            if ((nNonce & 0xfff) == 0)
                return false;
        }
    }
    catch (const std::runtime_error& e)
    {
        {
            std::lock_guard lock(g_lock);
            m_flag_mining_busy = false;
            m_flag_active_threads[th] = true;
            LogPrintf("Runtime mining error: %s [%s:%s:%u]\n", e.what(), __FILE__, __func__, __LINE__);
        }
        return false;
    }
}
bool BlockMining::ProcessBlockFound(const uint32_t th, const CBlock* pblock, std::any context)
{
    try {
        {
            LogPrintf("Mining ProcessBlockFound: %s\n", pblock->ToString());
        }
        NodeContext& node = EnsureAnyNodeContext(context);
        ChainstateManager& chainman = EnsureChainman(node);
        Chainstate& active_chainstate = chainman.ActiveChainstate();
        CChain& active_chain = active_chainstate.m_chain;
        CMutableTransaction txCoinbase(*pblock->vtx[0]);
        {
            LogPrintf("Mining Generated: %s\n", FormatMoney(txCoinbase.vout[0].nValue));
        }
        {
            LOCK(cs_main);
            CBlockIndex* pindexPrev = active_chain.Tip();
            if (pblock->hashPrevBlock != pindexPrev->GetBlockHash())
            {
                {
                    LogPrintf("Mining Generated block is stale [%s:%s:%u]\n", __FILE__, __func__, __LINE__);
                }
                return false;
            }
            if (CheckProofOfWork(pblock->GetHash(), pblock->nBits, chainman.GetConsensus())) {
                if (!chainman.GetParams().IsTestChain()) {
                    const CConnman& connman = EnsureConnman(node);
                    if (connman.GetNodeCount(ConnectionDirection::Both) == 0) {
                        {
                            LogPrintf("Thread #%u. Is not connected! [%s:%s:%u]\n", th, __FILE__, __func__, __LINE__);
                        }
                        return false;
                    }
                    if (active_chainstate.IsInitialBlockDownload() && pindexPrev->nHeight > 1) {
                        {
                            LogPrintf("Thread #%u. Is in initial sync and waiting for blocks... [%s:%s:%u]\n", th, __FILE__, __func__, __LINE__);
                        }
                        return false;
                    }
                }
                std::shared_ptr<const CBlock> shared_pblock = std::make_shared<const CBlock>(*pblock);
                if (!chainman.ProcessNewBlock(shared_pblock, true, true, nullptr)) {
                    {
                        LogPrintf("Mining ProcessNewBlock Faild [%s:%s:%u]\n", __FILE__, __func__, __LINE__);
                    }
                    return false;
                }
                return true;
            }
            return false;
        }
    }
    catch (const std::runtime_error& e)
    {
        {
            m_flag_mining_busy = false;
            m_flag_active_threads[th] = true;
            LogPrintf("Runtime mining error: %s [%s:%s:%u]\n", e.what(), __FILE__, __func__, __LINE__);
        }
        return false;
    }
}
void BlockMining::GenerateBlock(uint32_t th, uint32_t it, uint32_t min, uint32_t max, std::any context, const CScript& coinbase_script)
{
    {
        std::lock_guard lock(g_lock);
        m_flag_active_threads[th] = false;
        LogPrintf("Thread #%u Iter #%u. Start [min=%u...max=%u]. [%s:%s:%u]\n", th, it, min, max, __FILE__, __func__, __LINE__);
    }
    try {
        unsigned int nExtraNonce = 0;
        int64_t nStart = GetTime();
        while (m_flag_mining_busy) {
            NodeContext& node = EnsureAnyNodeContext(context);
            ChainstateManager& chainman = EnsureChainman(node);
            Chainstate& active_chainstate = chainman.ActiveChainstate();
            CChain& active_chain = active_chainstate.m_chain;
            const CTxMemPool& mempool = EnsureMemPool(node);
            const CChainParams& chainparams = chainman.GetParams();
            const CConnman& connman = EnsureConnman(node);
            unsigned int nTransactionsUpdatedLast = mempool.GetTransactionsUpdated();
            CBlockIndex* pindexPrev = active_chain.Tip();
            if (!chainman.GetParams().IsTestChain()) {
                if (connman.GetNodeCount(ConnectionDirection::Both) == 0) {
                    {
                        std::lock_guard lock(g_lock);
			             m_flag_active_threads[th] = true;
                        LogPrintf("Thread #%u Iter #%u. Is not connected! [%s:%s:%u]\n", th, it, __FILE__, __func__, __LINE__);
                    }
                    Sleep(1000);
                    return;
                }
                if (active_chainstate.IsInitialBlockDownload() && pindexPrev->nHeight > 1) {
                    {
                        std::lock_guard lock(g_lock);
			             m_flag_active_threads[th] = true;
                        LogPrintf("Thread #%u Iter #%u. Is in initial sync and waiting for blocks... [%s:%s:%u]\n", th, it, __FILE__, __func__, __LINE__);
                    }
                    Sleep(1000);
                    return;
                }
            }
            {
                std::lock_guard lock(g_lock);
                LogPrintf("Thread #%u Iter #%u. New SubIteration ExtraNonce=%d.\n", th, it, nExtraNonce);
            }         
            std::unique_ptr<CBlockTemplate> pblocktemplate(BlockAssembler{ chainman.ActiveChainstate(), &mempool }.CreateNewBlock(coinbase_script));
            if (!pblocktemplate.get())
            {
                {
                    std::lock_guard lock(g_lock);
			         m_flag_active_threads[th] = true;
                    LogPrintf("Thread #%u Iter #%u. Couldn't create new block [%s:%s:%u]\n", th, it, __FILE__, __func__, __LINE__);
                }
                Sleep(1000);
                return;
            }
            CBlock* pblock = &pblocktemplate->block;
            if (!IncrementExtraNonce(th, pblock, pindexPrev, nExtraNonce))
            {
                {
                    std::lock_guard lock(g_lock);
			         m_flag_active_threads[th] = true;
                    LogPrintf("Thread #%u Iter #%u. IncrementExtraNonce [%s:%s:%u]\n", th, it, __FILE__, __func__, __LINE__);
                }
                Sleep(1000);
                return;
            }
            arith_uint256 hashTarget = arith_uint256().SetCompact(pblock->nBits);
            uint256 hash;
            uint32_t nNonce = random(min, max);
            uint32_t countNonce = 0;
            while (m_flag_mining_busy) {
                if (ScanHash(pblock, nNonce, countNonce, min, max, th, &hash)) {
                    if (UintToArith256(hash) <= hashTarget) {
                        pblock->nNonce = nNonce;
                        assert(hash == pblock->GetHash());
                        {

                            bool pbf = false;
                            {
                                std::lock_guard lock(g_lock);
                                LOCK(cs_main);
                                pbf = ProcessBlockFound(th, pblock, context);
                            }
                                if (pbf)
                                {
                                    {
                                        ++CounterBlok;
                                        m_flag_active_threads[th] = true;
                                        LogPrintf("Thread #%u Iter #%u. Blok #%u OK!\n", th, it, CounterBlok);
                                    }
                                    return;
                                }
                                else
                                {
                                    {
                                        m_flag_active_threads[th] = true;
                                        LogPrintf("Thread #%u Iter #%u. Blok FAILD!\n", th, it);
                                    }
                                    return;
                                }

                        }
                    }
                }
                if (!m_flag_mining_busy) {
                    LogPrintf("Thread #%u Iter #%u. Emergency exit A\n", th, it);
                    m_flag_active_threads[th] = true;
                    return;
                }
                {
                    std::lock_guard lock(g_lock);
                    if (m_flag_active_threads[th]) {
                        LogPrintf("Thread #%u Iter #%u. Emergency exit B\n", th, it);
                        return;
                    }
                }
                if (countNonce >= max - min) {
                    {
                        std::lock_guard lock(g_lock);
                        LogPrintf("Thread #%u Iter #%u. Break he search has ended because all allowed options have been exhausted\n", th, it);
                    }
                    break;
                }
                if (ShutdownRequested()) {
                    {
                        std::lock_guard lock(g_lock);
                        m_flag_active_threads[th] = true;
                        LogPrintf("Thread #%u Iter #%u. Break through termination\n", th, it);
                    }
                    return;
                }
                if ((nNonce & 0xfffff) == 0)
                {
                if (mempool.GetTransactionsUpdated() != nTransactionsUpdatedLast && GetTime() - nStart > 60) {
                    {
                        std::lock_guard lock(g_lock);
                        m_flag_active_threads[th] = true;
                        LogPrintf("Thread #%u Iter #%u. Break due to the fact that there is a new transaction\n", th, it);
                    }
                    return;
                }
                {
                    LOCK(cs_main);
                    if (pindexPrev != active_chain.Tip()) {
                        {
                            std::lock_guard lock(g_lock);
                            m_flag_active_threads[th] = true;
                            LogPrintf("Thread #%u Iter #%u. Break due to the fact that the block has already been found\n", th, it);
                        }
                        return;
                    }
                }
                if (UpdateTime(pblock, chainparams.GetConsensus(), pindexPrev) < 0) {
                    {
                        std::lock_guard lock(g_lock);
                        m_flag_active_threads[th] = true;
                        LogPrintf("Thread #%u Iter #%u. Break due to the fact that the transaction has already been found\n", th, it);
                    }
                    return;
                }
                if (chainparams.GetConsensus().fPowAllowMinDifficultyBlocks) {
                    hashTarget.SetCompact(pblock->nBits);
                    {
                        std::lock_guard lock(g_lock);
                        m_flag_active_threads[th] = true;
                        LogPrintf("Thread #%u Iter #%u. Break So that we can use the correct tense\n", th, it);
                    }
                    return;
                }
			     }
                if (nNonce > max) {
                    nNonce = min;
                }
            }
        }
    }
    catch (const std::runtime_error& e)
    {
        {
            std::lock_guard lock(g_lock);
            m_flag_mining_busy = false;
            LogPrintf("Thread #%u Iter #%u. Runtime mining error: %s [%s:%s:%u]\n", th, it, e.what(), __FILE__, __func__, __LINE__);
        }
    }
    {
        std::lock_guard lock(g_lock);
        m_flag_active_threads[th] = true;
    }
}
UniValue BlockMining::GenerateBlocks(bool fGenerate, int nGenProcLimit, const CTxDestination destination, std::any context)
{
    try {
        if (!IsValidDestination(destination)) {
            m_flag_mining_busy = false;
            {
                std::lock_guard lock(g_lock);
                m_flag_mining_busy = false;
                LogPrintf("Mining Error: Invalid address [%s:%s:%u]\n", __FILE__, __func__, __LINE__);
            }
            return GenerError("Invalid address");
        }
        m_flag_active_threads.clear();
        assert(m_flag_active_threads.empty());
        m_flag_progress_threads.clear();
        assert(m_flag_progress_threads.empty());
        m_hashPrevBlock.clear();
        assert(m_hashPrevBlock.empty());
        for (int n = 0; n < nGenProcLimit; ++n) {
            m_flag_active_threads.push_back(true);
            m_hashPrevBlock.push_back(uint256());
            m_flag_progress_threads.push_back(0);
        }
        uint32_t it = 0;
        uint32_t temp_i = 0;
        CounterBlok = 0;
        while (m_flag_mining_busy) {
            /*NodeContext& node = EnsureAnyNodeContext(context);
            ChainstateManager& chainman = EnsureChainman(node);
            Chainstate& active_chainstate = chainman.ActiveChainstate();
            CChain& active_chain = active_chainstate.m_chain;
            const CConnman& connman = EnsureConnman(node);
            CBlockIndex* pindexPrev = active_chain.Tip();
            if (!chainman.GetParams().IsTestChain()) {
                if (connman.GetNodeCount(ConnectionDirection::Both) == 0) {
                    {
                        std::lock_guard lock(g_lock);
                        LogPrintf("Is not connected! [%s:%s:%u]\n", __FILE__, __func__, __LINE__);
                    }
                    Sleep(1000);
                    continue;
                }
                if (active_chainstate.IsInitialBlockDownload() && pindexPrev->nHeight > 1) {
                    {
                        std::lock_guard lock(g_lock);
                        LogPrintf("Is in initial sync and waiting for blocks... [%s:%s:%u]\n", __FILE__, __func__, __LINE__);
                    }
                    Sleep(1000);
                    continue;
                }
                Sleep(1000);
                if (connman.GetNodeCount(ConnectionDirection::Both) == 0) {
                    {
                        std::lock_guard lock(g_lock);
                        LogPrintf("Is not connected! [%s:%s:%u]\n", __FILE__, __func__, __LINE__);
                    }
                    Sleep(1000);
                    continue;
                }
                if (active_chainstate.IsInitialBlockDownload() && pindexPrev->nHeight > 1) {
                    {
                        std::lock_guard lock(g_lock);
                        LogPrintf("Is in initial sync and waiting for blocks... [%s:%s:%u]\n", __FILE__, __func__, __LINE__);
                    }
                    Sleep(1000);
                    continue;
                }
            }*/
            while (true) {
                Sleep(1000);
                int count_term = 0;
                double progress = 0;
                {
                    for (int n = 0; n < nGenProcLimit; ++n)
                    {
                        progress += m_flag_progress_threads[n];
                        if (m_flag_active_threads[n]) {
                            ++count_term;
                        }
                    }
                    progress /= nGenProcLimit;
                    if ((temp_i / LIMIT_SEC_PROFRESS) * LIMIT_SEC_PROFRESS == temp_i && temp_i > 0) {
                        temp_i = 0;
                        {
                            std::lock_guard lock(g_lock);
                            LogPrintf("Progress Mining = %u%%\n", progress);
                        }
                    }
                    ++temp_i;
                }
                if (count_term == nGenProcLimit)
                {
                    {
                        std::lock_guard lock(g_lock);
                        LogPrintf("Progress Mining = %u%%\n", progress);
                    }
                    break;
                }
            }
            if (!m_flag_mining_busy) break;
            {
                std::lock_guard lock(g_lock);
                LogPrintf("Mining iter #%u \n", it);
            }
            std::vector<std::thread> m_worker_threads;
            m_worker_threads.clear();
            assert(m_worker_threads.empty());
            uint32_t _start = 0;
            uint32_t _final = 4294967295;
            uint32_t _step = _final / nGenProcLimit;
            uint32_t _min = 0;
            uint32_t _max = _step;
            CScript coinbase_script = GetScriptForDestination(destination);
            UniValue blockHashes(UniValue::VARR);
            uint256 block_hash;
            {
                std::lock_guard lock(g_lock);
                for (int n = 0; n < nGenProcLimit; ++n)
                {
                    m_worker_threads.emplace_back([n, it, _min, _max, context, coinbase_script]() {
                        util::ThreadRename(strprintf("Miner.%i", n));
                        GenerateBlock(n, it, _min, _max, context, coinbase_script);
                        });
                    _min = _max;
                    _max += _step;
                    if (n == nGenProcLimit - 1)
                    {
                        _max = _final;
                    }
                }
                for (auto& thread : m_worker_threads) {
                    thread.detach();
                }
                if (!block_hash.IsNull()) {
                    blockHashes.push_back(block_hash.GetHex());
                }
            }
            ++it;
        }
        {
            std::lock_guard lock(g_lock);
            LogPrintf("Mining Final [%s:%s:%u]\n", __FILE__, __func__, __LINE__);
        }
    }
    catch (const std::runtime_error& e)
    {
        {
            std::lock_guard lock(g_lock);
            m_flag_mining_busy = false;
            LogPrintf("Runtime mining error: %s [%s:%s:%u]\n", e.what(), __FILE__, __func__, __LINE__);
        }
        return GenerError(e.what());
    }
}
bool BlockMining::Minings(bool fGenerate, int nGenProcLimit, const CTxDestination destination, std::any context)
{
    if (nGenProcLimit < 1) { fGenerate = false; }
    if (!fGenerate) {
        m_flag_mining_busy = false;
        {
            std::lock_guard lock(g_lock);
            LogPrintf("Mining OFF \n");
        }
    }
    else {
        try {
            if (m_flag_mining_busy)
            {
                return false;
            }
            LogPrintf("Mining ON \n");
            m_flag_mining_busy = true;
            std::vector<std::thread> m_g_thread;
            m_g_thread.clear();
            assert(m_g_thread.empty());
            for (int n = 0; n < 1; ++n)
            {
                m_g_thread.emplace_back([fGenerate, nGenProcLimit, destination, context]() {
                    util::ThreadRename(strprintf("MinerController"));
                    GenerateBlocks(fGenerate, nGenProcLimit, destination, context);
                    });
            }
            for (auto& thread : m_g_thread) {
                thread.detach();
            }
        }
        catch (const std::runtime_error& e)
        {
            {
                std::lock_guard lock(g_lock);
                m_flag_mining_busy = false;
                LogPrintf("Runtime mining error: %s [%s:%s:%u]\n", e.what(), __FILE__, __func__, __LINE__);
            }
            return false;
        }
    }
    return true;
}
#endif // ENABLE_WALLET
}

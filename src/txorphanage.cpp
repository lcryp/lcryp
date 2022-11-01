#include <txorphanage.h>
#include <consensus/validation.h>
#include <logging.h>
#include <policy/policy.h>
#include <cassert>
static constexpr int64_t ORPHAN_TX_EXPIRE_TIME = 20 * 60;
static constexpr int64_t ORPHAN_TX_EXPIRE_INTERVAL = 5 * 60;
RecursiveMutex g_cs_orphans;
bool TxOrphanage::AddTx(const CTransactionRef& tx, NodeId peer)
{
    AssertLockHeld(g_cs_orphans);
    const uint256& hash = tx->GetHash();
    if (m_orphans.count(hash))
        return false;
    unsigned int sz = GetTransactionWeight(*tx);
    if (sz > MAX_STANDARD_TX_WEIGHT)
    {
        LogPrint(BCLog::MEMPOOL, "ignoring large orphan tx (size: %u, hash: %s)\n", sz, hash.ToString());
        return false;
    }
    auto ret = m_orphans.emplace(hash, OrphanTx{tx, peer, GetTime() + ORPHAN_TX_EXPIRE_TIME, m_orphan_list.size()});
    assert(ret.second);
    m_orphan_list.push_back(ret.first);
    m_wtxid_to_orphan_it.emplace(tx->GetWitnessHash(), ret.first);
    for (const CTxIn& txin : tx->vin) {
        m_outpoint_to_orphan_it[txin.prevout].insert(ret.first);
    }
    LogPrint(BCLog::MEMPOOL, "stored orphan tx %s (mapsz %u outsz %u)\n", hash.ToString(),
             m_orphans.size(), m_outpoint_to_orphan_it.size());
    return true;
}
int TxOrphanage::EraseTx(const uint256& txid)
{
    AssertLockHeld(g_cs_orphans);
    std::map<uint256, OrphanTx>::iterator it = m_orphans.find(txid);
    if (it == m_orphans.end())
        return 0;
    for (const CTxIn& txin : it->second.tx->vin)
    {
        auto itPrev = m_outpoint_to_orphan_it.find(txin.prevout);
        if (itPrev == m_outpoint_to_orphan_it.end())
            continue;
        itPrev->second.erase(it);
        if (itPrev->second.empty())
            m_outpoint_to_orphan_it.erase(itPrev);
    }
    size_t old_pos = it->second.list_pos;
    assert(m_orphan_list[old_pos] == it);
    if (old_pos + 1 != m_orphan_list.size()) {
        auto it_last = m_orphan_list.back();
        m_orphan_list[old_pos] = it_last;
        it_last->second.list_pos = old_pos;
    }
    m_orphan_list.pop_back();
    m_wtxid_to_orphan_it.erase(it->second.tx->GetWitnessHash());
    m_orphans.erase(it);
    return 1;
}
void TxOrphanage::EraseForPeer(NodeId peer)
{
    AssertLockHeld(g_cs_orphans);
    int nErased = 0;
    std::map<uint256, OrphanTx>::iterator iter = m_orphans.begin();
    while (iter != m_orphans.end())
    {
        std::map<uint256, OrphanTx>::iterator maybeErase = iter++;
        if (maybeErase->second.fromPeer == peer)
        {
            nErased += EraseTx(maybeErase->second.tx->GetHash());
        }
    }
    if (nErased > 0) LogPrint(BCLog::MEMPOOL, "Erased %d orphan tx from peer=%d\n", nErased, peer);
}
void TxOrphanage::LimitOrphans(unsigned int max_orphans)
{
    AssertLockHeld(g_cs_orphans);
    unsigned int nEvicted = 0;
    static int64_t nNextSweep;
    int64_t nNow = GetTime();
    if (nNextSweep <= nNow) {
        int nErased = 0;
        int64_t nMinExpTime = nNow + ORPHAN_TX_EXPIRE_TIME - ORPHAN_TX_EXPIRE_INTERVAL;
        std::map<uint256, OrphanTx>::iterator iter = m_orphans.begin();
        while (iter != m_orphans.end())
        {
            std::map<uint256, OrphanTx>::iterator maybeErase = iter++;
            if (maybeErase->second.nTimeExpire <= nNow) {
                nErased += EraseTx(maybeErase->second.tx->GetHash());
            } else {
                nMinExpTime = std::min(maybeErase->second.nTimeExpire, nMinExpTime);
            }
        }
        nNextSweep = nMinExpTime + ORPHAN_TX_EXPIRE_INTERVAL;
        if (nErased > 0) LogPrint(BCLog::MEMPOOL, "Erased %d orphan tx due to expiration\n", nErased);
    }
    FastRandomContext rng;
    while (m_orphans.size() > max_orphans)
    {
        size_t randompos = rng.randrange(m_orphan_list.size());
        EraseTx(m_orphan_list[randompos]->first);
        ++nEvicted;
    }
    if (nEvicted > 0) LogPrint(BCLog::MEMPOOL, "orphanage overflow, removed %u tx\n", nEvicted);
}
void TxOrphanage::AddChildrenToWorkSet(const CTransaction& tx, std::set<uint256>& orphan_work_set) const
{
    AssertLockHeld(g_cs_orphans);
    for (unsigned int i = 0; i < tx.vout.size(); i++) {
        const auto it_by_prev = m_outpoint_to_orphan_it.find(COutPoint(tx.GetHash(), i));
        if (it_by_prev != m_outpoint_to_orphan_it.end()) {
            for (const auto& elem : it_by_prev->second) {
                orphan_work_set.insert(elem->first);
            }
        }
    }
}
bool TxOrphanage::HaveTx(const GenTxid& gtxid) const
{
    LOCK(g_cs_orphans);
    if (gtxid.IsWtxid()) {
        return m_wtxid_to_orphan_it.count(gtxid.GetHash());
    } else {
        return m_orphans.count(gtxid.GetHash());
    }
}
std::pair<CTransactionRef, NodeId> TxOrphanage::GetTx(const uint256& txid) const
{
    AssertLockHeld(g_cs_orphans);
    const auto it = m_orphans.find(txid);
    if (it == m_orphans.end()) return {nullptr, -1};
    return {it->second.tx, it->second.fromPeer};
}
void TxOrphanage::EraseForBlock(const CBlock& block)
{
    LOCK(g_cs_orphans);
    std::vector<uint256> vOrphanErase;
    for (const CTransactionRef& ptx : block.vtx) {
        const CTransaction& tx = *ptx;
        for (const auto& txin : tx.vin) {
            auto itByPrev = m_outpoint_to_orphan_it.find(txin.prevout);
            if (itByPrev == m_outpoint_to_orphan_it.end()) continue;
            for (auto mi = itByPrev->second.begin(); mi != itByPrev->second.end(); ++mi) {
                const CTransaction& orphanTx = *(*mi)->second.tx;
                const uint256& orphanHash = orphanTx.GetHash();
                vOrphanErase.push_back(orphanHash);
            }
        }
    }
    if (vOrphanErase.size()) {
        int nErased = 0;
        for (const uint256& orphanHash : vOrphanErase) {
            nErased += EraseTx(orphanHash);
        }
        LogPrint(BCLog::MEMPOOL, "Erased %d orphan tx included or conflicted by block\n", nErased);
    }
}

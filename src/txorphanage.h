#ifndef LCRYP_TXORPHANAGE_H
#define LCRYP_TXORPHANAGE_H
#include <net.h>
#include <primitives/block.h>
#include <primitives/transaction.h>
#include <sync.h>
extern RecursiveMutex g_cs_orphans;
class TxOrphanage {
public:
    bool AddTx(const CTransactionRef& tx, NodeId peer) EXCLUSIVE_LOCKS_REQUIRED(g_cs_orphans);
    bool HaveTx(const GenTxid& gtxid) const LOCKS_EXCLUDED(::g_cs_orphans);
    std::pair<CTransactionRef, NodeId> GetTx(const uint256& txid) const EXCLUSIVE_LOCKS_REQUIRED(g_cs_orphans);
    int EraseTx(const uint256& txid) EXCLUSIVE_LOCKS_REQUIRED(g_cs_orphans);
    void EraseForPeer(NodeId peer) EXCLUSIVE_LOCKS_REQUIRED(g_cs_orphans);
    void EraseForBlock(const CBlock& block) LOCKS_EXCLUDED(::g_cs_orphans);
    void LimitOrphans(unsigned int max_orphans) EXCLUSIVE_LOCKS_REQUIRED(g_cs_orphans);
    void AddChildrenToWorkSet(const CTransaction& tx, std::set<uint256>& orphan_work_set) const EXCLUSIVE_LOCKS_REQUIRED(g_cs_orphans);
    size_t Size() LOCKS_EXCLUDED(::g_cs_orphans)
    {
        LOCK(::g_cs_orphans);
        return m_orphans.size();
    }
protected:
    struct OrphanTx {
        CTransactionRef tx;
        NodeId fromPeer;
        int64_t nTimeExpire;
        size_t list_pos;
    };
    std::map<uint256, OrphanTx> m_orphans GUARDED_BY(g_cs_orphans);
    using OrphanMap = decltype(m_orphans);
    struct IteratorComparator
    {
        template<typename I>
        bool operator()(const I& a, const I& b) const
        {
            return &(*a) < &(*b);
        }
    };
    std::map<COutPoint, std::set<OrphanMap::iterator, IteratorComparator>> m_outpoint_to_orphan_it GUARDED_BY(g_cs_orphans);
    std::vector<OrphanMap::iterator> m_orphan_list GUARDED_BY(g_cs_orphans);
    std::map<uint256, OrphanMap::iterator> m_wtxid_to_orphan_it GUARDED_BY(g_cs_orphans);
};
#endif

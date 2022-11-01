#ifndef LCRYP_NET_PROCESSING_H
#define LCRYP_NET_PROCESSING_H
#include <net.h>
#include <validationinterface.h>
class AddrMan;
class CChainParams;
class CTxMemPool;
class ChainstateManager;
static const unsigned int DEFAULT_MAX_ORPHAN_TRANSACTIONS = 100;
static const unsigned int DEFAULT_BLOCK_RECONSTRUCTION_EXTRA_TXN = 100;
static const bool DEFAULT_PEERBLOOMFILTERS = false;
static const bool DEFAULT_PEERBLOCKFILTERS = false;
static const int DISCOURAGEMENT_THRESHOLD{100};
struct CNodeStateStats {
    int nSyncHeight = -1;
    int nCommonHeight = -1;
    int m_starting_height = -1;
    std::chrono::microseconds m_ping_wait;
    std::vector<int> vHeightInFlight;
    bool m_relay_txs;
    CAmount m_fee_filter_received;
    uint64_t m_addr_processed = 0;
    uint64_t m_addr_rate_limited = 0;
    bool m_addr_relay_enabled{false};
    ServiceFlags their_services;
    int64_t presync_height{-1};
};
class PeerManager : public CValidationInterface, public NetEventsInterface
{
public:
    static std::unique_ptr<PeerManager> make(CConnman& connman, AddrMan& addrman,
                                             BanMan* banman, ChainstateManager& chainman,
                                             CTxMemPool& pool, bool ignore_incoming_txs);
    virtual ~PeerManager() { }
    virtual std::optional<std::string> FetchBlock(NodeId peer_id, const CBlockIndex& block_index) = 0;
    virtual void StartScheduledTasks(CScheduler& scheduler) = 0;
    virtual bool GetNodeStateStats(NodeId nodeid, CNodeStateStats& stats) const = 0;
    virtual bool IgnoresIncomingTxs() = 0;
    virtual void RelayTransaction(const uint256& txid, const uint256& wtxid) = 0;
    virtual void SendPings() = 0;
    virtual void SetBestHeight(int height) = 0;
    virtual void UnitTestMisbehaving(NodeId peer_id, int howmuch) = 0;
    virtual void CheckForStaleTipAndEvictPeers() = 0;
    virtual void ProcessMessage(CNode& pfrom, const std::string& msg_type, CDataStream& vRecv,
                                const std::chrono::microseconds time_received, const std::atomic<bool>& interruptMsgProc) EXCLUSIVE_LOCKS_REQUIRED(g_msgproc_mutex) = 0;
    virtual void UpdateLastBlockAnnounceTime(NodeId node, int64_t time_in_seconds) = 0;
};
#endif

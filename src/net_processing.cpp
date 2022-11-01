#include <net_processing.h>
#include <addrman.h>
#include <banman.h>
#include <blockencodings.h>
#include <blockfilter.h>
#include <chainparams.h>
#include <consensus/amount.h>
#include <consensus/validation.h>
#include <deploymentstatus.h>
#include <hash.h>
#include <headerssync.h>
#include <index/blockfilterindex.h>
#include <merkleblock.h>
#include <netbase.h>
#include <netmessagemaker.h>
#include <node/blockstorage.h>
#include <policy/fees.h>
#include <policy/policy.h>
#include <policy/settings.h>
#include <primitives/block.h>
#include <primitives/transaction.h>
#include <random.h>
#include <reverse_iterator.h>
#include <scheduler.h>
#include <streams.h>
#include <sync.h>
#include <timedata.h>
#include <tinyformat.h>
#include <txmempool.h>
#include <txorphanage.h>
#include <txrequest.h>
#include <util/check.h>
#include <util/strencodings.h>
#include <util/system.h>
#include <util/trace.h>
#include <validation.h>
#include <algorithm>
#include <atomic>
#include <chrono>
#include <future>
#include <memory>
#include <optional>
#include <typeinfo>
using node::ReadBlockFromDisk;
using node::ReadRawBlockFromDisk;
using node::fImporting;
using node::fPruneMode;
using node::fReindex;
static constexpr auto RELAY_TX_CACHE_TIME = 15min;
static constexpr auto UNCONDITIONAL_RELAY_DELAY = 2min;
static constexpr auto HEADERS_DOWNLOAD_TIMEOUT_BASE = 15min;
static constexpr auto HEADERS_DOWNLOAD_TIMEOUT_PER_HEADER = 1ms;
static constexpr auto HEADERS_RESPONSE_TIME{2min};
static constexpr int32_t MAX_OUTBOUND_PEERS_TO_PROTECT_FROM_DISCONNECT = 4;
static constexpr auto CHAIN_SYNC_TIMEOUT{20min};
static constexpr auto STALE_CHECK_INTERVAL{30min};
static constexpr auto EXTRA_PEER_CHECK_INTERVAL{45s};
static constexpr auto MINIMUM_CONNECT_TIME{30s};
static constexpr uint64_t RANDOMIZER_ID_ADDRESS_RELAY = 0x3cac0035b5866b90ULL;
static constexpr int STALE_RELAY_AGE_LIMIT = 1 * 30 * 24 * 60 * 60;
static constexpr int HISTORICAL_BLOCK_AGE = 1 * 7 * 24 * 60 * 60;
static constexpr auto PING_INTERVAL{2min};
static const unsigned int MAX_LOCATOR_SZ = 101;
static const unsigned int MAX_INV_SZ = 50000;
static constexpr int32_t MAX_PEER_TX_REQUEST_IN_FLIGHT = 100;
static constexpr int32_t MAX_PEER_TX_ANNOUNCEMENTS = 5000;
static constexpr auto TXID_RELAY_DELAY{2s};
static constexpr auto NONPREF_PEER_TX_DELAY{2s};
static constexpr auto OVERLOADED_PEER_TX_DELAY{2s};
static constexpr auto GETDATA_TX_INTERVAL{60s};
static const unsigned int MAX_GETDATA_SZ = 1000;
static const int MAX_BLOCKS_IN_TRANSIT_PER_PEER = 16;
static constexpr auto BLOCK_STALLING_TIMEOUT{2s};
static const unsigned int MAX_HEADERS_RESULTS = 2000;
static const int MAX_CMPCTBLOCK_DEPTH = 5;
static const int MAX_BLOCKTXN_DEPTH = 10;
static const unsigned int BLOCK_DOWNLOAD_WINDOW = 1024;
static constexpr double BLOCK_DOWNLOAD_TIMEOUT_BASE = 1;
static constexpr double BLOCK_DOWNLOAD_TIMEOUT_PER_PEER = 0.5;
static const unsigned int MAX_BLOCKS_TO_ANNOUNCE = 8;
static const int MAX_UNCONNECTING_HEADERS = 10;
static const unsigned int NODE_NETWORK_LIMITED_MIN_BLOCKS = 288;
static constexpr auto AVG_LOCAL_ADDRESS_BROADCAST_INTERVAL{24h};
static constexpr auto AVG_ADDRESS_BROADCAST_INTERVAL{30s};
static constexpr auto ROTATE_ADDR_RELAY_DEST_INTERVAL{24h};
static constexpr auto INBOUND_INVENTORY_BROADCAST_INTERVAL{5s};
static constexpr auto OUTBOUND_INVENTORY_BROADCAST_INTERVAL{2s};
static constexpr unsigned int INVENTORY_BROADCAST_PER_SECOND = 7;
static constexpr unsigned int INVENTORY_BROADCAST_MAX = INVENTORY_BROADCAST_PER_SECOND * count_seconds(INBOUND_INVENTORY_BROADCAST_INTERVAL);
static constexpr unsigned int INVENTORY_MAX_RECENT_RELAY = 3500;
static_assert(INVENTORY_MAX_RECENT_RELAY >= INVENTORY_BROADCAST_PER_SECOND * UNCONDITIONAL_RELAY_DELAY / std::chrono::seconds{1}, "INVENTORY_RELAY_MAX too low");
static constexpr auto AVG_FEEFILTER_BROADCAST_INTERVAL{10min};
static constexpr auto MAX_FEEFILTER_CHANGE_DELAY{5min};
static constexpr uint32_t MAX_GETCFILTERS_SIZE = 1000;
static constexpr uint32_t MAX_GETCFHEADERS_SIZE = 2000;
static constexpr size_t MAX_PCT_ADDR_TO_SEND = 23;
static constexpr size_t MAX_ADDR_TO_SEND{1000};
static constexpr double MAX_ADDR_RATE_PER_SECOND{0.1};
static constexpr size_t MAX_ADDR_PROCESSING_TOKEN_BUCKET{MAX_ADDR_TO_SEND};
static constexpr uint64_t CMPCTBLOCKS_VERSION{2};
namespace {
struct QueuedBlock {
    const CBlockIndex* pindex;
    std::unique_ptr<PartiallyDownloadedBlock> partialBlock;
};
struct Peer {
    const NodeId m_id{0};
    const ServiceFlags m_our_services;
    std::atomic<ServiceFlags> m_their_services{NODE_NONE};
    Mutex m_misbehavior_mutex;
    int m_misbehavior_score GUARDED_BY(m_misbehavior_mutex){0};
    bool m_should_discourage GUARDED_BY(m_misbehavior_mutex){false};
    Mutex m_block_inv_mutex;
    std::vector<uint256> m_blocks_for_inv_relay GUARDED_BY(m_block_inv_mutex);
    std::vector<uint256> m_blocks_for_headers_relay GUARDED_BY(m_block_inv_mutex);
    uint256 m_continuation_block GUARDED_BY(m_block_inv_mutex) {};
    std::atomic<int> m_starting_height{-1};
    std::atomic<uint64_t> m_ping_nonce_sent{0};
    std::atomic<std::chrono::microseconds> m_ping_start{0us};
    std::atomic<bool> m_ping_queued{false};
    std::atomic<bool> m_wtxid_relay{false};
    CAmount m_fee_filter_sent GUARDED_BY(NetEventsInterface::g_msgproc_mutex){0};
    std::chrono::microseconds m_next_send_feefilter GUARDED_BY(NetEventsInterface::g_msgproc_mutex){0};
    struct TxRelay {
        mutable RecursiveMutex m_bloom_filter_mutex;
        bool m_relay_txs GUARDED_BY(m_bloom_filter_mutex){false};
        std::unique_ptr<CBloomFilter> m_bloom_filter PT_GUARDED_BY(m_bloom_filter_mutex) GUARDED_BY(m_bloom_filter_mutex){nullptr};
        mutable RecursiveMutex m_tx_inventory_mutex;
        CRollingBloomFilter m_tx_inventory_known_filter GUARDED_BY(m_tx_inventory_mutex){50000, 0.000001};
        std::set<uint256> m_tx_inventory_to_send GUARDED_BY(m_tx_inventory_mutex);
        bool m_send_mempool GUARDED_BY(m_tx_inventory_mutex){false};
        std::atomic<std::chrono::seconds> m_last_mempool_req{0s};
        std::chrono::microseconds m_next_inv_send_time GUARDED_BY(NetEventsInterface::g_msgproc_mutex){0};
        std::atomic<CAmount> m_fee_filter_received{0};
    };
    TxRelay* SetTxRelay() EXCLUSIVE_LOCKS_REQUIRED(!m_tx_relay_mutex)
    {
        LOCK(m_tx_relay_mutex);
        Assume(!m_tx_relay);
        m_tx_relay = std::make_unique<Peer::TxRelay>();
        return m_tx_relay.get();
    };
    TxRelay* GetTxRelay() EXCLUSIVE_LOCKS_REQUIRED(!m_tx_relay_mutex)
    {
        return WITH_LOCK(m_tx_relay_mutex, return m_tx_relay.get());
    };
    std::vector<CAddress> m_addrs_to_send GUARDED_BY(NetEventsInterface::g_msgproc_mutex);
    std::unique_ptr<CRollingBloomFilter> m_addr_known GUARDED_BY(NetEventsInterface::g_msgproc_mutex);
    std::atomic_bool m_addr_relay_enabled{false};
    bool m_getaddr_sent GUARDED_BY(NetEventsInterface::g_msgproc_mutex){false};
    mutable Mutex m_addr_send_times_mutex;
    std::chrono::microseconds m_next_addr_send GUARDED_BY(m_addr_send_times_mutex){0};
    std::chrono::microseconds m_next_local_addr_send GUARDED_BY(m_addr_send_times_mutex){0};
    std::atomic_bool m_wants_addrv2{false};
    bool m_getaddr_recvd GUARDED_BY(NetEventsInterface::g_msgproc_mutex){false};
    double m_addr_token_bucket GUARDED_BY(NetEventsInterface::g_msgproc_mutex){1.0};
    std::chrono::microseconds m_addr_token_timestamp GUARDED_BY(NetEventsInterface::g_msgproc_mutex){GetTime<std::chrono::microseconds>()};
    std::atomic<uint64_t> m_addr_rate_limited{0};
    std::atomic<uint64_t> m_addr_processed{0};
    std::set<uint256> m_orphan_work_set GUARDED_BY(g_cs_orphans);
    bool m_inv_triggered_getheaders_before_sync GUARDED_BY(NetEventsInterface::g_msgproc_mutex){false};
    Mutex m_getdata_requests_mutex;
    std::deque<CInv> m_getdata_requests GUARDED_BY(m_getdata_requests_mutex);
    NodeClock::time_point m_last_getheaders_timestamp GUARDED_BY(NetEventsInterface::g_msgproc_mutex){};
    Mutex m_headers_sync_mutex;
    std::unique_ptr<HeadersSyncState> m_headers_sync PT_GUARDED_BY(m_headers_sync_mutex) GUARDED_BY(m_headers_sync_mutex) {};
    std::atomic<bool> m_sent_sendheaders{false};
    explicit Peer(NodeId id, ServiceFlags our_services)
        : m_id{id}
        , m_our_services{our_services}
    {}
private:
    Mutex m_tx_relay_mutex;
    std::unique_ptr<TxRelay> m_tx_relay GUARDED_BY(m_tx_relay_mutex);
};
using PeerRef = std::shared_ptr<Peer>;
struct CNodeState {
    const CBlockIndex* pindexBestKnownBlock{nullptr};
    uint256 hashLastUnknownBlock{};
    const CBlockIndex* pindexLastCommonBlock{nullptr};
    const CBlockIndex* pindexBestHeaderSent{nullptr};
    int nUnconnectingHeaders{0};
    bool fSyncStarted{false};
    std::chrono::microseconds m_headers_sync_timeout{0us};
    std::chrono::microseconds m_stalling_since{0us};
    std::list<QueuedBlock> vBlocksInFlight;
    std::chrono::microseconds m_downloading_since{0us};
    int nBlocksInFlight{0};
    bool fPreferredDownload{false};
    bool fPreferHeaders{false};
    bool m_requested_hb_cmpctblocks{false};
    bool m_provides_cmpctblocks{false};
    struct ChainSyncTimeoutState {
        std::chrono::seconds m_timeout{0s};
        const CBlockIndex* m_work_header{nullptr};
        bool m_sent_getheaders{false};
        bool m_protect{false};
    };
    ChainSyncTimeoutState m_chain_sync;
    int64_t m_last_block_announcement{0};
    const bool m_is_inbound;
    CRollingBloomFilter m_recently_announced_invs = CRollingBloomFilter{INVENTORY_MAX_RECENT_RELAY, 0.000001};
    CNodeState(bool is_inbound) : m_is_inbound(is_inbound) {}
};
class PeerManagerImpl final : public PeerManager
{
public:
    PeerManagerImpl(CConnman& connman, AddrMan& addrman,
                    BanMan* banman, ChainstateManager& chainman,
                    CTxMemPool& pool, bool ignore_incoming_txs);
    void BlockConnected(const std::shared_ptr<const CBlock>& pblock, const CBlockIndex* pindexConnected) override
        EXCLUSIVE_LOCKS_REQUIRED(!m_recent_confirmed_transactions_mutex);
    void BlockDisconnected(const std::shared_ptr<const CBlock> &block, const CBlockIndex* pindex) override
        EXCLUSIVE_LOCKS_REQUIRED(!m_recent_confirmed_transactions_mutex);
    void UpdatedBlockTip(const CBlockIndex *pindexNew, const CBlockIndex *pindexFork, bool fInitialDownload) override
        EXCLUSIVE_LOCKS_REQUIRED(!m_peer_mutex);
    void BlockChecked(const CBlock& block, const BlockValidationState& state) override
        EXCLUSIVE_LOCKS_REQUIRED(!m_peer_mutex);
    void NewPoWValidBlock(const CBlockIndex *pindex, const std::shared_ptr<const CBlock>& pblock) override
        EXCLUSIVE_LOCKS_REQUIRED(!m_most_recent_block_mutex);
    void InitializeNode(CNode& node, ServiceFlags our_services) override EXCLUSIVE_LOCKS_REQUIRED(!m_peer_mutex);
    void FinalizeNode(const CNode& node) override EXCLUSIVE_LOCKS_REQUIRED(!m_peer_mutex, !m_headers_presync_mutex);
    bool ProcessMessages(CNode* pfrom, std::atomic<bool>& interrupt) override
        EXCLUSIVE_LOCKS_REQUIRED(!m_peer_mutex, !m_recent_confirmed_transactions_mutex, !m_most_recent_block_mutex, !m_headers_presync_mutex, g_msgproc_mutex);
    bool SendMessages(CNode* pto) override
        EXCLUSIVE_LOCKS_REQUIRED(!m_peer_mutex, !m_recent_confirmed_transactions_mutex, !m_most_recent_block_mutex, g_msgproc_mutex);
    void StartScheduledTasks(CScheduler& scheduler) override;
    void CheckForStaleTipAndEvictPeers() override;
    std::optional<std::string> FetchBlock(NodeId peer_id, const CBlockIndex& block_index) override
        EXCLUSIVE_LOCKS_REQUIRED(!m_peer_mutex);
    bool GetNodeStateStats(NodeId nodeid, CNodeStateStats& stats) const override EXCLUSIVE_LOCKS_REQUIRED(!m_peer_mutex);
    bool IgnoresIncomingTxs() override { return m_ignore_incoming_txs; }
    void SendPings() override EXCLUSIVE_LOCKS_REQUIRED(!m_peer_mutex);
    void RelayTransaction(const uint256& txid, const uint256& wtxid) override EXCLUSIVE_LOCKS_REQUIRED(!m_peer_mutex);
    void SetBestHeight(int height) override { m_best_height = height; };
    void UnitTestMisbehaving(NodeId peer_id, int howmuch) override EXCLUSIVE_LOCKS_REQUIRED(!m_peer_mutex) { Misbehaving(*Assert(GetPeerRef(peer_id)), howmuch, ""); };
    void ProcessMessage(CNode& pfrom, const std::string& msg_type, CDataStream& vRecv,
                        const std::chrono::microseconds time_received, const std::atomic<bool>& interruptMsgProc) override
        EXCLUSIVE_LOCKS_REQUIRED(!m_peer_mutex, !m_recent_confirmed_transactions_mutex, !m_most_recent_block_mutex, !m_headers_presync_mutex, g_msgproc_mutex);
    void UpdateLastBlockAnnounceTime(NodeId node, int64_t time_in_seconds) override;
private:
    void ConsiderEviction(CNode& pto, Peer& peer, std::chrono::seconds time_in_seconds) EXCLUSIVE_LOCKS_REQUIRED(cs_main, g_msgproc_mutex);
    void EvictExtraOutboundPeers(std::chrono::seconds now) EXCLUSIVE_LOCKS_REQUIRED(cs_main);
    void ReattemptInitialBroadcast(CScheduler& scheduler) EXCLUSIVE_LOCKS_REQUIRED(!m_peer_mutex);
    PeerRef GetPeerRef(NodeId id) const EXCLUSIVE_LOCKS_REQUIRED(!m_peer_mutex);
    PeerRef RemovePeer(NodeId id) EXCLUSIVE_LOCKS_REQUIRED(!m_peer_mutex);
    void Misbehaving(Peer& peer, int howmuch, const std::string& message);
    bool MaybePunishNodeForBlock(NodeId nodeid, const BlockValidationState& state,
                                 bool via_compact_block, const std::string& message = "")
        EXCLUSIVE_LOCKS_REQUIRED(!m_peer_mutex);
    bool MaybePunishNodeForTx(NodeId nodeid, const TxValidationState& state, const std::string& message = "")
        EXCLUSIVE_LOCKS_REQUIRED(!m_peer_mutex);
    bool MaybeDiscourageAndDisconnect(CNode& pnode, Peer& peer);
    void ProcessOrphanTx(std::set<uint256>& orphan_work_set) EXCLUSIVE_LOCKS_REQUIRED(cs_main, g_cs_orphans)
        EXCLUSIVE_LOCKS_REQUIRED(!m_peer_mutex);
    void ProcessHeadersMessage(CNode& pfrom, Peer& peer,
                               std::vector<CBlockHeader>&& headers,
                               bool via_compact_block)
        EXCLUSIVE_LOCKS_REQUIRED(!m_peer_mutex, !m_headers_presync_mutex, g_msgproc_mutex);
    bool CheckHeadersPoW(const std::vector<CBlockHeader>& headers, const Consensus::Params& consensusParams, Peer& peer);
    arith_uint256 GetAntiDoSWorkThreshold();
    void HandleFewUnconnectingHeaders(CNode& pfrom, Peer& peer, const std::vector<CBlockHeader>& headers) EXCLUSIVE_LOCKS_REQUIRED(g_msgproc_mutex);
    bool CheckHeadersAreContinuous(const std::vector<CBlockHeader>& headers) const;
    bool IsContinuationOfLowWorkHeadersSync(Peer& peer, CNode& pfrom,
            std::vector<CBlockHeader>& headers)
        EXCLUSIVE_LOCKS_REQUIRED(peer.m_headers_sync_mutex, !m_headers_presync_mutex, g_msgproc_mutex);
    bool TryLowWorkHeadersSync(Peer& peer, CNode& pfrom,
                                  const CBlockIndex* chain_start_header,
                                  std::vector<CBlockHeader>& headers)
        EXCLUSIVE_LOCKS_REQUIRED(!peer.m_headers_sync_mutex, !m_peer_mutex, !m_headers_presync_mutex, g_msgproc_mutex);
    bool IsAncestorOfBestHeaderOrTip(const CBlockIndex* header) EXCLUSIVE_LOCKS_REQUIRED(cs_main);
    bool MaybeSendGetHeaders(CNode& pfrom, const CBlockLocator& locator, Peer& peer) EXCLUSIVE_LOCKS_REQUIRED(g_msgproc_mutex);
    void HeadersDirectFetchBlocks(CNode& pfrom, const Peer& peer, const CBlockIndex* pindexLast);
    void UpdatePeerStateForReceivedHeaders(CNode& pfrom, const CBlockIndex *pindexLast, bool received_new_header, bool may_have_more_headers);
    void SendBlockTransactions(CNode& pfrom, Peer& peer, const CBlock& block, const BlockTransactionsRequest& req);
    void AddTxAnnouncement(const CNode& node, const GenTxid& gtxid, std::chrono::microseconds current_time)
        EXCLUSIVE_LOCKS_REQUIRED(::cs_main);
    void PushNodeVersion(CNode& pnode, const Peer& peer);
    void MaybeSendPing(CNode& node_to, Peer& peer, std::chrono::microseconds now);
    void MaybeSendAddr(CNode& node, Peer& peer, std::chrono::microseconds current_time) EXCLUSIVE_LOCKS_REQUIRED(g_msgproc_mutex);
    void MaybeSendSendHeaders(CNode& node, Peer& peer) EXCLUSIVE_LOCKS_REQUIRED(g_msgproc_mutex);
    void RelayAddress(NodeId originator, const CAddress& addr, bool fReachable) EXCLUSIVE_LOCKS_REQUIRED(!m_peer_mutex, g_msgproc_mutex);
    void MaybeSendFeefilter(CNode& node, Peer& peer, std::chrono::microseconds current_time) EXCLUSIVE_LOCKS_REQUIRED(g_msgproc_mutex);
    const CChainParams& m_chainparams;
    CConnman& m_connman;
    AddrMan& m_addrman;
    BanMan* const m_banman;
    ChainstateManager& m_chainman;
    CTxMemPool& m_mempool;
    TxRequestTracker m_txrequest GUARDED_BY(::cs_main);
    std::atomic<int> m_best_height{-1};
    std::chrono::seconds m_stale_tip_check_time GUARDED_BY(cs_main){0s};
    const bool m_ignore_incoming_txs;
    bool RejectIncomingTxs(const CNode& peer) const;
    bool m_initial_sync_finished GUARDED_BY(cs_main){false};
    mutable Mutex m_peer_mutex;
    std::map<NodeId, PeerRef> m_peer_map GUARDED_BY(m_peer_mutex);
    std::map<NodeId, CNodeState> m_node_states GUARDED_BY(cs_main);
    const CNodeState* State(NodeId pnode) const EXCLUSIVE_LOCKS_REQUIRED(cs_main);
    CNodeState* State(NodeId pnode) EXCLUSIVE_LOCKS_REQUIRED(cs_main);
    uint32_t GetFetchFlags(const Peer& peer) const;
    std::atomic<std::chrono::microseconds> m_next_inv_to_inbounds{0us};
    int nSyncStarted GUARDED_BY(cs_main) = 0;
    uint256 m_last_block_inv_triggering_headers_sync GUARDED_BY(g_msgproc_mutex){};
    std::map<uint256, std::pair<NodeId, bool>> mapBlockSource GUARDED_BY(cs_main);
    std::atomic<int> m_wtxid_relay_peers{0};
    int m_outbound_peers_with_protect_from_disconnect GUARDED_BY(cs_main) = 0;
    int m_num_preferred_download_peers GUARDED_BY(cs_main){0};
    bool AlreadyHaveTx(const GenTxid& gtxid)
        EXCLUSIVE_LOCKS_REQUIRED(cs_main, !m_recent_confirmed_transactions_mutex);
    CRollingBloomFilter m_recent_rejects GUARDED_BY(::cs_main){120'000, 0.000'001};
    uint256 hashRecentRejectsChainTip GUARDED_BY(cs_main);
    Mutex m_recent_confirmed_transactions_mutex;
    CRollingBloomFilter m_recent_confirmed_transactions GUARDED_BY(m_recent_confirmed_transactions_mutex){48'000, 0.000'001};
    std::chrono::microseconds NextInvToInbounds(std::chrono::microseconds now,
                                                std::chrono::seconds average_interval);
    Mutex m_most_recent_block_mutex;
    std::shared_ptr<const CBlock> m_most_recent_block GUARDED_BY(m_most_recent_block_mutex);
    std::shared_ptr<const CBlockHeaderAndShortTxIDs> m_most_recent_compact_block GUARDED_BY(m_most_recent_block_mutex);
    uint256 m_most_recent_block_hash GUARDED_BY(m_most_recent_block_mutex);
    Mutex m_headers_presync_mutex;
    using HeadersPresyncStats = std::pair<arith_uint256, std::optional<std::pair<int64_t, uint32_t>>>;
    std::map<NodeId, HeadersPresyncStats> m_headers_presync_stats GUARDED_BY(m_headers_presync_mutex) {};
    NodeId m_headers_presync_bestpeer GUARDED_BY(m_headers_presync_mutex) {-1};
    std::atomic_bool m_headers_presync_should_signal{false};
    int m_highest_fast_announce GUARDED_BY(::cs_main){0};
    bool IsBlockRequested(const uint256& hash) EXCLUSIVE_LOCKS_REQUIRED(cs_main);
    void RemoveBlockRequest(const uint256& hash) EXCLUSIVE_LOCKS_REQUIRED(cs_main);
    bool BlockRequested(NodeId nodeid, const CBlockIndex& block, std::list<QueuedBlock>::iterator** pit = nullptr) EXCLUSIVE_LOCKS_REQUIRED(cs_main);
    bool TipMayBeStale() EXCLUSIVE_LOCKS_REQUIRED(cs_main);
    void FindNextBlocksToDownload(const Peer& peer, unsigned int count, std::vector<const CBlockIndex*>& vBlocks, NodeId& nodeStaller) EXCLUSIVE_LOCKS_REQUIRED(cs_main);
    std::map<uint256, std::pair<NodeId, std::list<QueuedBlock>::iterator> > mapBlocksInFlight GUARDED_BY(cs_main);
    std::atomic<std::chrono::seconds> m_last_tip_update{0s};
    CTransactionRef FindTxForGetData(const CNode& peer, const GenTxid& gtxid, const std::chrono::seconds mempool_req, const std::chrono::seconds now) LOCKS_EXCLUDED(cs_main);
    void ProcessGetData(CNode& pfrom, Peer& peer, const std::atomic<bool>& interruptMsgProc)
        EXCLUSIVE_LOCKS_REQUIRED(!m_most_recent_block_mutex, peer.m_getdata_requests_mutex) LOCKS_EXCLUDED(::cs_main);
    void ProcessBlock(CNode& node, const std::shared_ptr<const CBlock>& block, bool force_processing, bool min_pow_checked);
    typedef std::map<uint256, CTransactionRef> MapRelay;
    MapRelay mapRelay GUARDED_BY(cs_main);
    std::deque<std::pair<std::chrono::microseconds, MapRelay::iterator>> g_relay_expiration GUARDED_BY(cs_main);
    void MaybeSetPeerAsAnnouncingHeaderAndIDs(NodeId nodeid) EXCLUSIVE_LOCKS_REQUIRED(cs_main);
    std::list<NodeId> lNodesAnnouncingHeaderAndIDs GUARDED_BY(cs_main);
    int m_peers_downloading_from GUARDED_BY(cs_main) = 0;
    TxOrphanage m_orphanage;
    void AddToCompactExtraTransactions(const CTransactionRef& tx) EXCLUSIVE_LOCKS_REQUIRED(g_cs_orphans);
    std::vector<std::pair<uint256, CTransactionRef>> vExtraTxnForCompact GUARDED_BY(g_cs_orphans);
    size_t vExtraTxnForCompactIt GUARDED_BY(g_cs_orphans) = 0;
    void ProcessBlockAvailability(NodeId nodeid) EXCLUSIVE_LOCKS_REQUIRED(cs_main);
    void UpdateBlockAvailability(NodeId nodeid, const uint256& hash) EXCLUSIVE_LOCKS_REQUIRED(cs_main);
    bool CanDirectFetch() EXCLUSIVE_LOCKS_REQUIRED(cs_main);
    bool BlockRequestAllowed(const CBlockIndex* pindex) EXCLUSIVE_LOCKS_REQUIRED(cs_main);
    bool AlreadyHaveBlock(const uint256& block_hash) EXCLUSIVE_LOCKS_REQUIRED(cs_main);
    void ProcessGetBlockData(CNode& pfrom, Peer& peer, const CInv& inv)
        EXCLUSIVE_LOCKS_REQUIRED(!m_most_recent_block_mutex);
    bool PrepareBlockFilterRequest(CNode& node, Peer& peer,
                                   BlockFilterType filter_type, uint32_t start_height,
                                   const uint256& stop_hash, uint32_t max_height_diff,
                                   const CBlockIndex*& stop_index,
                                   BlockFilterIndex*& filter_index);
    void ProcessGetCFilters(CNode& node, Peer& peer, CDataStream& vRecv);
    void ProcessGetCFHeaders(CNode& node, Peer& peer, CDataStream& vRecv);
    void ProcessGetCFCheckPt(CNode& node, Peer& peer, CDataStream& vRecv);
    bool SetupAddressRelay(const CNode& node, Peer& peer) EXCLUSIVE_LOCKS_REQUIRED(g_msgproc_mutex);
    void AddAddressKnown(Peer& peer, const CAddress& addr) EXCLUSIVE_LOCKS_REQUIRED(g_msgproc_mutex);
    void PushAddress(Peer& peer, const CAddress& addr, FastRandomContext& insecure_rand) EXCLUSIVE_LOCKS_REQUIRED(g_msgproc_mutex);
};
const CNodeState* PeerManagerImpl::State(NodeId pnode) const EXCLUSIVE_LOCKS_REQUIRED(cs_main)
{
    std::map<NodeId, CNodeState>::const_iterator it = m_node_states.find(pnode);
    if (it == m_node_states.end())
        return nullptr;
    return &it->second;
}
CNodeState* PeerManagerImpl::State(NodeId pnode) EXCLUSIVE_LOCKS_REQUIRED(cs_main)
{
    return const_cast<CNodeState*>(std::as_const(*this).State(pnode));
}
static bool IsAddrCompatible(const Peer& peer, const CAddress& addr)
{
    return peer.m_wants_addrv2 || addr.IsAddrV1Compatible();
}
void PeerManagerImpl::AddAddressKnown(Peer& peer, const CAddress& addr)
{
    assert(peer.m_addr_known);
    peer.m_addr_known->insert(addr.GetKey());
}
void PeerManagerImpl::PushAddress(Peer& peer, const CAddress& addr, FastRandomContext& insecure_rand)
{
    assert(peer.m_addr_known);
    if (addr.IsValid() && !peer.m_addr_known->contains(addr.GetKey()) && IsAddrCompatible(peer, addr)) {
        if (peer.m_addrs_to_send.size() >= MAX_ADDR_TO_SEND) {
            peer.m_addrs_to_send[insecure_rand.randrange(peer.m_addrs_to_send.size())] = addr;
        } else {
            peer.m_addrs_to_send.push_back(addr);
        }
    }
}
static void AddKnownTx(Peer& peer, const uint256& hash)
{
    auto tx_relay = peer.GetTxRelay();
    if (!tx_relay) return;
    LOCK(tx_relay->m_tx_inventory_mutex);
    tx_relay->m_tx_inventory_known_filter.insert(hash);
}
static bool CanServeBlocks(const Peer& peer)
{
    return peer.m_their_services & (NODE_NETWORK|NODE_NETWORK_LIMITED);
}
static bool IsLimitedPeer(const Peer& peer)
{
    return (!(peer.m_their_services & NODE_NETWORK) &&
             (peer.m_their_services & NODE_NETWORK_LIMITED));
}
static bool CanServeWitnesses(const Peer& peer)
{
    return peer.m_their_services & NODE_WITNESS;
}
std::chrono::microseconds PeerManagerImpl::NextInvToInbounds(std::chrono::microseconds now,
                                                             std::chrono::seconds average_interval)
{
    if (m_next_inv_to_inbounds.load() < now) {
        m_next_inv_to_inbounds = GetExponentialRand(now, average_interval);
    }
    return m_next_inv_to_inbounds;
}
bool PeerManagerImpl::IsBlockRequested(const uint256& hash)
{
    return mapBlocksInFlight.find(hash) != mapBlocksInFlight.end();
}
void PeerManagerImpl::RemoveBlockRequest(const uint256& hash)
{
    auto it = mapBlocksInFlight.find(hash);
    if (it == mapBlocksInFlight.end()) {
        return;
    }
    auto [node_id, list_it] = it->second;
    CNodeState *state = State(node_id);
    assert(state != nullptr);
    if (state->vBlocksInFlight.begin() == list_it) {
        state->m_downloading_since = std::max(state->m_downloading_since, GetTime<std::chrono::microseconds>());
    }
    state->vBlocksInFlight.erase(list_it);
    state->nBlocksInFlight--;
    if (state->nBlocksInFlight == 0) {
        m_peers_downloading_from--;
    }
    state->m_stalling_since = 0us;
    mapBlocksInFlight.erase(it);
}
bool PeerManagerImpl::BlockRequested(NodeId nodeid, const CBlockIndex& block, std::list<QueuedBlock>::iterator** pit)
{
    const uint256& hash{block.GetBlockHash()};
    CNodeState *state = State(nodeid);
    assert(state != nullptr);
    std::map<uint256, std::pair<NodeId, std::list<QueuedBlock>::iterator> >::iterator itInFlight = mapBlocksInFlight.find(hash);
    if (itInFlight != mapBlocksInFlight.end() && itInFlight->second.first == nodeid) {
        if (pit) {
            *pit = &itInFlight->second.second;
        }
        return false;
    }
    RemoveBlockRequest(hash);
    std::list<QueuedBlock>::iterator it = state->vBlocksInFlight.insert(state->vBlocksInFlight.end(),
            {&block, std::unique_ptr<PartiallyDownloadedBlock>(pit ? new PartiallyDownloadedBlock(&m_mempool) : nullptr)});
    state->nBlocksInFlight++;
    if (state->nBlocksInFlight == 1) {
        state->m_downloading_since = GetTime<std::chrono::microseconds>();
        m_peers_downloading_from++;
    }
    itInFlight = mapBlocksInFlight.insert(std::make_pair(hash, std::make_pair(nodeid, it))).first;
    if (pit) {
        *pit = &itInFlight->second.second;
    }
    return true;
}
void PeerManagerImpl::MaybeSetPeerAsAnnouncingHeaderAndIDs(NodeId nodeid)
{
    AssertLockHeld(cs_main);
    if (m_ignore_incoming_txs) return;
    CNodeState* nodestate = State(nodeid);
    if (!nodestate || !nodestate->m_provides_cmpctblocks) {
        return;
    }
    int num_outbound_hb_peers = 0;
    for (std::list<NodeId>::iterator it = lNodesAnnouncingHeaderAndIDs.begin(); it != lNodesAnnouncingHeaderAndIDs.end(); it++) {
        if (*it == nodeid) {
            lNodesAnnouncingHeaderAndIDs.erase(it);
            lNodesAnnouncingHeaderAndIDs.push_back(nodeid);
            return;
        }
        CNodeState *state = State(*it);
        if (state != nullptr && !state->m_is_inbound) ++num_outbound_hb_peers;
    }
    if (nodestate->m_is_inbound) {
        if (lNodesAnnouncingHeaderAndIDs.size() >= 3 && num_outbound_hb_peers == 1) {
            CNodeState *remove_node = State(lNodesAnnouncingHeaderAndIDs.front());
            if (remove_node != nullptr && !remove_node->m_is_inbound) {
                std::swap(lNodesAnnouncingHeaderAndIDs.front(), *std::next(lNodesAnnouncingHeaderAndIDs.begin()));
            }
        }
    }
    m_connman.ForNode(nodeid, [this](CNode* pfrom) EXCLUSIVE_LOCKS_REQUIRED(::cs_main) {
        AssertLockHeld(::cs_main);
        if (lNodesAnnouncingHeaderAndIDs.size() >= 3) {
            m_connman.ForNode(lNodesAnnouncingHeaderAndIDs.front(), [this](CNode* pnodeStop){
                m_connman.PushMessage(pnodeStop, CNetMsgMaker(pnodeStop->GetCommonVersion()).Make(NetMsgType::SENDCMPCT,  false,  CMPCTBLOCKS_VERSION));
                pnodeStop->m_bip152_highbandwidth_to = false;
                return true;
            });
            lNodesAnnouncingHeaderAndIDs.pop_front();
        }
        m_connman.PushMessage(pfrom, CNetMsgMaker(pfrom->GetCommonVersion()).Make(NetMsgType::SENDCMPCT,  true,  CMPCTBLOCKS_VERSION));
        pfrom->m_bip152_highbandwidth_to = true;
        lNodesAnnouncingHeaderAndIDs.push_back(pfrom->GetId());
        return true;
    });
}
bool PeerManagerImpl::TipMayBeStale()
{
    AssertLockHeld(cs_main);
    const Consensus::Params& consensusParams = m_chainparams.GetConsensus();
    if (m_last_tip_update.load() == 0s) {
        m_last_tip_update = GetTime<std::chrono::seconds>();
    }
    return m_last_tip_update.load() < GetTime<std::chrono::seconds>() - std::chrono::seconds{consensusParams.nPowTargetSpacing * 3} && mapBlocksInFlight.empty();
}
bool PeerManagerImpl::CanDirectFetch()
{
    return m_chainman.ActiveChain().Tip()->Time() > GetAdjustedTime() - m_chainparams.GetConsensus().PowTargetSpacing() * 20;
}
static bool PeerHasHeader(CNodeState *state, const CBlockIndex *pindex) EXCLUSIVE_LOCKS_REQUIRED(cs_main)
{
    if (state->pindexBestKnownBlock && pindex == state->pindexBestKnownBlock->GetAncestor(pindex->nHeight))
        return true;
    if (state->pindexBestHeaderSent && pindex == state->pindexBestHeaderSent->GetAncestor(pindex->nHeight))
        return true;
    return false;
}
void PeerManagerImpl::ProcessBlockAvailability(NodeId nodeid) {
    CNodeState *state = State(nodeid);
    assert(state != nullptr);
    if (!state->hashLastUnknownBlock.IsNull()) {
        const CBlockIndex* pindex = m_chainman.m_blockman.LookupBlockIndex(state->hashLastUnknownBlock);
        if (pindex && pindex->nChainWork > 0) {
            if (state->pindexBestKnownBlock == nullptr || pindex->nChainWork >= state->pindexBestKnownBlock->nChainWork) {
                state->pindexBestKnownBlock = pindex;
            }
            state->hashLastUnknownBlock.SetNull();
        }
    }
}
void PeerManagerImpl::UpdateBlockAvailability(NodeId nodeid, const uint256 &hash) {
    CNodeState *state = State(nodeid);
    assert(state != nullptr);
    ProcessBlockAvailability(nodeid);
    const CBlockIndex* pindex = m_chainman.m_blockman.LookupBlockIndex(hash);
    if (pindex && pindex->nChainWork > 0) {
        if (state->pindexBestKnownBlock == nullptr || pindex->nChainWork >= state->pindexBestKnownBlock->nChainWork) {
            state->pindexBestKnownBlock = pindex;
        }
    } else {
        state->hashLastUnknownBlock = hash;
    }
}
void PeerManagerImpl::FindNextBlocksToDownload(const Peer& peer, unsigned int count, std::vector<const CBlockIndex*>& vBlocks, NodeId& nodeStaller)
{
    if (count == 0)
        return;
    vBlocks.reserve(vBlocks.size() + count);
    CNodeState *state = State(peer.m_id);
    assert(state != nullptr);
    ProcessBlockAvailability(peer.m_id);
    if (state->pindexBestKnownBlock == nullptr || state->pindexBestKnownBlock->nChainWork < m_chainman.ActiveChain().Tip()->nChainWork || state->pindexBestKnownBlock->nChainWork < nMinimumChainWork) {
        return;
    }
    if (state->pindexLastCommonBlock == nullptr) {
        state->pindexLastCommonBlock = m_chainman.ActiveChain()[std::min(state->pindexBestKnownBlock->nHeight, m_chainman.ActiveChain().Height())];
    }
    state->pindexLastCommonBlock = LastCommonAncestor(state->pindexLastCommonBlock, state->pindexBestKnownBlock);
    if (state->pindexLastCommonBlock == state->pindexBestKnownBlock)
        return;
    std::vector<const CBlockIndex*> vToFetch;
    const CBlockIndex *pindexWalk = state->pindexLastCommonBlock;
    int nWindowEnd = state->pindexLastCommonBlock->nHeight + BLOCK_DOWNLOAD_WINDOW;
    int nMaxHeight = std::min<int>(state->pindexBestKnownBlock->nHeight, nWindowEnd + 1);
    NodeId waitingfor = -1;
    while (pindexWalk->nHeight < nMaxHeight) {
        int nToFetch = std::min(nMaxHeight - pindexWalk->nHeight, std::max<int>(count - vBlocks.size(), 128));
        vToFetch.resize(nToFetch);
        pindexWalk = state->pindexBestKnownBlock->GetAncestor(pindexWalk->nHeight + nToFetch);
        vToFetch[nToFetch - 1] = pindexWalk;
        for (unsigned int i = nToFetch - 1; i > 0; i--) {
            vToFetch[i - 1] = vToFetch[i]->pprev;
        }
        for (const CBlockIndex* pindex : vToFetch) {
            if (!pindex->IsValid(BLOCK_VALID_TREE)) {
                return;
            }
            if (!CanServeWitnesses(peer) && DeploymentActiveAt(*pindex, m_chainman, Consensus::DEPLOYMENT_SEGWIT)) {
                return;
            }
            if (pindex->nStatus & BLOCK_HAVE_DATA || m_chainman.ActiveChain().Contains(pindex)) {
                if (pindex->HaveTxsDownloaded())
                    state->pindexLastCommonBlock = pindex;
            } else if (!IsBlockRequested(pindex->GetBlockHash())) {
                if (pindex->nHeight > nWindowEnd) {
                    if (vBlocks.size() == 0 && waitingfor != peer.m_id) {
                        nodeStaller = waitingfor;
                    }
                    return;
                }
                vBlocks.push_back(pindex);
                if (vBlocks.size() == count) {
                    return;
                }
            } else if (waitingfor == -1) {
                waitingfor = mapBlocksInFlight[pindex->GetBlockHash()].first;
            }
        }
    }
}
}
void PeerManagerImpl::PushNodeVersion(CNode& pnode, const Peer& peer)
{
    uint64_t my_services{peer.m_our_services};
    const int64_t nTime{count_seconds(GetTime<std::chrono::seconds>())};
    uint64_t nonce = pnode.GetLocalNonce();
    const int nNodeStartingHeight{m_best_height};
    NodeId nodeid = pnode.GetId();
    CAddress addr = pnode.addr;
    CService addr_you = addr.IsRoutable() && !IsProxy(addr) && addr.IsAddrV1Compatible() ? addr : CService();
    uint64_t your_services{addr.nServices};
    const bool tx_relay = !m_ignore_incoming_txs && !pnode.IsBlockOnlyConn() && !pnode.IsFeelerConn();
    m_connman.PushMessage(&pnode, CNetMsgMaker(INIT_PROTO_VERSION).Make(NetMsgType::VERSION, PROTOCOL_VERSION, my_services, nTime,
            your_services, addr_you,
            my_services, CService(),
            nonce, strSubVersion, nNodeStartingHeight, tx_relay));
    if (fLogIPs) {
        LogPrint(BCLog::NET, "send version message: version %d, blocks=%d, them=%s, txrelay=%d, peer=%d\n", PROTOCOL_VERSION, nNodeStartingHeight, addr_you.ToString(), tx_relay, nodeid);
    } else {
        LogPrint(BCLog::NET, "send version message: version %d, blocks=%d, txrelay=%d, peer=%d\n", PROTOCOL_VERSION, nNodeStartingHeight, tx_relay, nodeid);
    }
}
void PeerManagerImpl::AddTxAnnouncement(const CNode& node, const GenTxid& gtxid, std::chrono::microseconds current_time)
{
    AssertLockHeld(::cs_main);
    NodeId nodeid = node.GetId();
    if (!node.HasPermission(NetPermissionFlags::Relay) && m_txrequest.Count(nodeid) >= MAX_PEER_TX_ANNOUNCEMENTS) {
        return;
    }
    const CNodeState* state = State(nodeid);
    auto delay{0us};
    const bool preferred = state->fPreferredDownload;
    if (!preferred) delay += NONPREF_PEER_TX_DELAY;
    if (!gtxid.IsWtxid() && m_wtxid_relay_peers > 0) delay += TXID_RELAY_DELAY;
    const bool overloaded = !node.HasPermission(NetPermissionFlags::Relay) &&
        m_txrequest.CountInFlight(nodeid) >= MAX_PEER_TX_REQUEST_IN_FLIGHT;
    if (overloaded) delay += OVERLOADED_PEER_TX_DELAY;
    m_txrequest.ReceivedInv(nodeid, gtxid, preferred, current_time + delay);
}
void PeerManagerImpl::UpdateLastBlockAnnounceTime(NodeId node, int64_t time_in_seconds)
{
    LOCK(cs_main);
    CNodeState *state = State(node);
    if (state) state->m_last_block_announcement = time_in_seconds;
}
void PeerManagerImpl::InitializeNode(CNode& node, ServiceFlags our_services)
{
    NodeId nodeid = node.GetId();
    {
        LOCK(cs_main);
        m_node_states.emplace_hint(m_node_states.end(), std::piecewise_construct, std::forward_as_tuple(nodeid), std::forward_as_tuple(node.IsInboundConn()));
        assert(m_txrequest.Count(nodeid) == 0);
    }
    PeerRef peer = std::make_shared<Peer>(nodeid, our_services);
    {
        LOCK(m_peer_mutex);
        m_peer_map.emplace_hint(m_peer_map.end(), nodeid, peer);
    }
    if (!node.IsInboundConn()) {
        PushNodeVersion(node, *peer);
    }
}
void PeerManagerImpl::ReattemptInitialBroadcast(CScheduler& scheduler)
{
    std::set<uint256> unbroadcast_txids = m_mempool.GetUnbroadcastTxs();
    for (const auto& txid : unbroadcast_txids) {
        CTransactionRef tx = m_mempool.get(txid);
        if (tx != nullptr) {
            RelayTransaction(txid, tx->GetWitnessHash());
        } else {
            m_mempool.RemoveUnbroadcastTx(txid, true);
        }
    }
    const std::chrono::milliseconds delta = 10min + GetRandMillis(5min);
    scheduler.scheduleFromNow([&] { ReattemptInitialBroadcast(scheduler); }, delta);
}
void PeerManagerImpl::FinalizeNode(const CNode& node)
{
    NodeId nodeid = node.GetId();
    int misbehavior{0};
    {
    LOCK(cs_main);
    {
        PeerRef peer = RemovePeer(nodeid);
        assert(peer != nullptr);
        misbehavior = WITH_LOCK(peer->m_misbehavior_mutex, return peer->m_misbehavior_score);
        m_wtxid_relay_peers -= peer->m_wtxid_relay;
        assert(m_wtxid_relay_peers >= 0);
    }
    CNodeState *state = State(nodeid);
    assert(state != nullptr);
    if (state->fSyncStarted)
        nSyncStarted--;
    for (const QueuedBlock& entry : state->vBlocksInFlight) {
        mapBlocksInFlight.erase(entry.pindex->GetBlockHash());
    }
    WITH_LOCK(g_cs_orphans, m_orphanage.EraseForPeer(nodeid));
    m_txrequest.DisconnectedPeer(nodeid);
    m_num_preferred_download_peers -= state->fPreferredDownload;
    m_peers_downloading_from -= (state->nBlocksInFlight != 0);
    assert(m_peers_downloading_from >= 0);
    m_outbound_peers_with_protect_from_disconnect -= state->m_chain_sync.m_protect;
    assert(m_outbound_peers_with_protect_from_disconnect >= 0);
    m_node_states.erase(nodeid);
    if (m_node_states.empty()) {
        assert(mapBlocksInFlight.empty());
        assert(m_num_preferred_download_peers == 0);
        assert(m_peers_downloading_from == 0);
        assert(m_outbound_peers_with_protect_from_disconnect == 0);
        assert(m_wtxid_relay_peers == 0);
        assert(m_txrequest.Size() == 0);
        assert(m_orphanage.Size() == 0);
    }
    }
    if (node.fSuccessfullyConnected && misbehavior == 0 &&
        !node.IsBlockOnlyConn() && !node.IsInboundConn()) {
        m_addrman.Connected(node.addr);
    }
    {
        LOCK(m_headers_presync_mutex);
        m_headers_presync_stats.erase(nodeid);
    }
    LogPrint(BCLog::NET, "Cleared nodestate for peer=%d\n", nodeid);
}
PeerRef PeerManagerImpl::GetPeerRef(NodeId id) const
{
    LOCK(m_peer_mutex);
    auto it = m_peer_map.find(id);
    return it != m_peer_map.end() ? it->second : nullptr;
}
PeerRef PeerManagerImpl::RemovePeer(NodeId id)
{
    PeerRef ret;
    LOCK(m_peer_mutex);
    auto it = m_peer_map.find(id);
    if (it != m_peer_map.end()) {
        ret = std::move(it->second);
        m_peer_map.erase(it);
    }
    return ret;
}
bool PeerManagerImpl::GetNodeStateStats(NodeId nodeid, CNodeStateStats& stats) const
{
    {
        LOCK(cs_main);
        const CNodeState* state = State(nodeid);
        if (state == nullptr)
            return false;
        stats.nSyncHeight = state->pindexBestKnownBlock ? state->pindexBestKnownBlock->nHeight : -1;
        stats.nCommonHeight = state->pindexLastCommonBlock ? state->pindexLastCommonBlock->nHeight : -1;
        for (const QueuedBlock& queue : state->vBlocksInFlight) {
            if (queue.pindex)
                stats.vHeightInFlight.push_back(queue.pindex->nHeight);
        }
    }
    PeerRef peer = GetPeerRef(nodeid);
    if (peer == nullptr) return false;
    stats.their_services = peer->m_their_services;
    stats.m_starting_height = peer->m_starting_height;
    auto ping_wait{0us};
    if ((0 != peer->m_ping_nonce_sent) && (0 != peer->m_ping_start.load().count())) {
        ping_wait = GetTime<std::chrono::microseconds>() - peer->m_ping_start.load();
    }
    if (auto tx_relay = peer->GetTxRelay(); tx_relay != nullptr) {
        stats.m_relay_txs = WITH_LOCK(tx_relay->m_bloom_filter_mutex, return tx_relay->m_relay_txs);
        stats.m_fee_filter_received = tx_relay->m_fee_filter_received.load();
    } else {
        stats.m_relay_txs = false;
        stats.m_fee_filter_received = 0;
    }
    stats.m_ping_wait = ping_wait;
    stats.m_addr_processed = peer->m_addr_processed.load();
    stats.m_addr_rate_limited = peer->m_addr_rate_limited.load();
    stats.m_addr_relay_enabled = peer->m_addr_relay_enabled.load();
    {
        LOCK(peer->m_headers_sync_mutex);
        if (peer->m_headers_sync) {
            stats.presync_height = peer->m_headers_sync->GetPresyncHeight();
        }
    }
    return true;
}
void PeerManagerImpl::AddToCompactExtraTransactions(const CTransactionRef& tx)
{
    size_t max_extra_txn = gArgs.GetIntArg("-blockreconstructionextratxn", DEFAULT_BLOCK_RECONSTRUCTION_EXTRA_TXN);
    if (max_extra_txn <= 0)
        return;
    if (!vExtraTxnForCompact.size())
        vExtraTxnForCompact.resize(max_extra_txn);
    vExtraTxnForCompact[vExtraTxnForCompactIt] = std::make_pair(tx->GetWitnessHash(), tx);
    vExtraTxnForCompactIt = (vExtraTxnForCompactIt + 1) % max_extra_txn;
}
void PeerManagerImpl::Misbehaving(Peer& peer, int howmuch, const std::string& message)
{
    assert(howmuch > 0);
    LOCK(peer.m_misbehavior_mutex);
    const int score_before{peer.m_misbehavior_score};
    peer.m_misbehavior_score += howmuch;
    const int score_now{peer.m_misbehavior_score};
    const std::string message_prefixed = message.empty() ? "" : (": " + message);
    std::string warning;
    if (score_now >= DISCOURAGEMENT_THRESHOLD && score_before < DISCOURAGEMENT_THRESHOLD) {
        warning = " DISCOURAGE THRESHOLD EXCEEDED";
        peer.m_should_discourage = true;
    }
    LogPrint(BCLog::NET, "Misbehaving: peer=%d (%d -> %d)%s%s\n",
             peer.m_id, score_before, score_now, warning, message_prefixed);
}
bool PeerManagerImpl::MaybePunishNodeForBlock(NodeId nodeid, const BlockValidationState& state,
                                              bool via_compact_block, const std::string& message)
{
    PeerRef peer{GetPeerRef(nodeid)};
    switch (state.GetResult()) {
    case BlockValidationResult::BLOCK_RESULT_UNSET:
        break;
    case BlockValidationResult::BLOCK_HEADER_LOW_WORK:
        break;
    case BlockValidationResult::BLOCK_CONSENSUS:
    case BlockValidationResult::BLOCK_MUTATED:
        if (!via_compact_block) {
            if (peer) Misbehaving(*peer, 100, message);
            return true;
        }
        break;
    case BlockValidationResult::BLOCK_CACHED_INVALID:
        {
            LOCK(cs_main);
            CNodeState *node_state = State(nodeid);
            if (node_state == nullptr) {
                break;
            }
            if (!via_compact_block && !node_state->m_is_inbound) {
                if (peer) Misbehaving(*peer, 100, message);
                return true;
            }
            break;
        }
    case BlockValidationResult::BLOCK_INVALID_HEADER:
    case BlockValidationResult::BLOCK_CHECKPOINT:
    case BlockValidationResult::BLOCK_INVALID_PREV:
        if (peer) Misbehaving(*peer, 100, message);
        return true;
    case BlockValidationResult::BLOCK_MISSING_PREV:
        if (peer) Misbehaving(*peer, 10, message);
        return true;
    case BlockValidationResult::BLOCK_RECENT_CONSENSUS_CHANGE:
    case BlockValidationResult::BLOCK_TIME_FUTURE:
        break;
    }
    if (message != "") {
        LogPrint(BCLog::NET, "peer=%d: %s\n", nodeid, message);
    }
    return false;
}
bool PeerManagerImpl::MaybePunishNodeForTx(NodeId nodeid, const TxValidationState& state, const std::string& message)
{
    PeerRef peer{GetPeerRef(nodeid)};
    switch (state.GetResult()) {
    case TxValidationResult::TX_RESULT_UNSET:
        break;
    case TxValidationResult::TX_CONSENSUS:
        if (peer) Misbehaving(*peer, 100, message);
        return true;
    case TxValidationResult::TX_RECENT_CONSENSUS_CHANGE:
    case TxValidationResult::TX_INPUTS_NOT_STANDARD:
    case TxValidationResult::TX_NOT_STANDARD:
    case TxValidationResult::TX_MISSING_INPUTS:
    case TxValidationResult::TX_PREMATURE_SPEND:
    case TxValidationResult::TX_WITNESS_MUTATED:
    case TxValidationResult::TX_WITNESS_STRIPPED:
    case TxValidationResult::TX_CONFLICT:
    case TxValidationResult::TX_MEMPOOL_POLICY:
    case TxValidationResult::TX_NO_MEMPOOL:
        break;
    }
    if (message != "") {
        LogPrint(BCLog::NET, "peer=%d: %s\n", nodeid, message);
    }
    return false;
}
bool PeerManagerImpl::BlockRequestAllowed(const CBlockIndex* pindex)
{
    AssertLockHeld(cs_main);
    if (m_chainman.ActiveChain().Contains(pindex)) return true;
    return pindex->IsValid(BLOCK_VALID_SCRIPTS) && (m_chainman.m_best_header != nullptr) &&
           (m_chainman.m_best_header->GetBlockTime() - pindex->GetBlockTime() < STALE_RELAY_AGE_LIMIT) &&
           (GetBlockProofEquivalentTime(*m_chainman.m_best_header, *pindex, *m_chainman.m_best_header, m_chainparams.GetConsensus()) < STALE_RELAY_AGE_LIMIT);
}
std::optional<std::string> PeerManagerImpl::FetchBlock(NodeId peer_id, const CBlockIndex& block_index)
{
    if (fImporting) return "Importing...";
    if (fReindex) return "Reindexing...";
    PeerRef peer = GetPeerRef(peer_id);
    if (peer == nullptr) return "Peer does not exist";
    if (!CanServeWitnesses(*peer)) return "Pre-SegWit peer";
    LOCK(cs_main);
    if (!BlockRequested(peer_id, block_index)) return "Already requested from this peer";
    const uint256& hash{block_index.GetBlockHash()};
    std::vector<CInv> invs{CInv(MSG_BLOCK | MSG_WITNESS_FLAG, hash)};
    bool success = m_connman.ForNode(peer_id, [this, &invs](CNode* node) {
        const CNetMsgMaker msgMaker(node->GetCommonVersion());
        this->m_connman.PushMessage(node, msgMaker.Make(NetMsgType::GETDATA, invs));
        return true;
    });
    if (!success) return "Peer not fully connected";
    LogPrint(BCLog::NET, "Requesting block %s from peer=%d\n",
                 hash.ToString(), peer_id);
    return std::nullopt;
}
std::unique_ptr<PeerManager> PeerManager::make(CConnman& connman, AddrMan& addrman,
                                               BanMan* banman, ChainstateManager& chainman,
                                               CTxMemPool& pool, bool ignore_incoming_txs)
{
    return std::make_unique<PeerManagerImpl>(connman, addrman, banman, chainman, pool, ignore_incoming_txs);
}
PeerManagerImpl::PeerManagerImpl(CConnman& connman, AddrMan& addrman,
                                 BanMan* banman, ChainstateManager& chainman,
                                 CTxMemPool& pool, bool ignore_incoming_txs)
    : m_chainparams(chainman.GetParams()),
      m_connman(connman),
      m_addrman(addrman),
      m_banman(banman),
      m_chainman(chainman),
      m_mempool(pool),
      m_ignore_incoming_txs(ignore_incoming_txs)
{
}
void PeerManagerImpl::StartScheduledTasks(CScheduler& scheduler)
{
    static_assert(EXTRA_PEER_CHECK_INTERVAL < STALE_CHECK_INTERVAL, "peer eviction timer should be less than stale tip check timer");
    scheduler.scheduleEvery([this] { this->CheckForStaleTipAndEvictPeers(); }, std::chrono::seconds{EXTRA_PEER_CHECK_INTERVAL});
    const std::chrono::milliseconds delta = 10min + GetRandMillis(5min);
    scheduler.scheduleFromNow([&] { ReattemptInitialBroadcast(scheduler); }, delta);
}
void PeerManagerImpl::BlockConnected(const std::shared_ptr<const CBlock>& pblock, const CBlockIndex* pindex)
{
    m_orphanage.EraseForBlock(*pblock);
    m_last_tip_update = GetTime<std::chrono::seconds>();
    {
        LOCK(m_recent_confirmed_transactions_mutex);
        for (const auto& ptx : pblock->vtx) {
            m_recent_confirmed_transactions.insert(ptx->GetHash());
            if (ptx->GetHash() != ptx->GetWitnessHash()) {
                m_recent_confirmed_transactions.insert(ptx->GetWitnessHash());
            }
        }
    }
    {
        LOCK(cs_main);
        for (const auto& ptx : pblock->vtx) {
            m_txrequest.ForgetTxHash(ptx->GetHash());
            m_txrequest.ForgetTxHash(ptx->GetWitnessHash());
        }
    }
}
void PeerManagerImpl::BlockDisconnected(const std::shared_ptr<const CBlock> &block, const CBlockIndex* pindex)
{
    LOCK(m_recent_confirmed_transactions_mutex);
    m_recent_confirmed_transactions.reset();
}
void PeerManagerImpl::NewPoWValidBlock(const CBlockIndex *pindex, const std::shared_ptr<const CBlock>& pblock)
{
    auto pcmpctblock = std::make_shared<const CBlockHeaderAndShortTxIDs>(*pblock);
    const CNetMsgMaker msgMaker(PROTOCOL_VERSION);
    LOCK(cs_main);
    if (pindex->nHeight <= m_highest_fast_announce)
        return;
    m_highest_fast_announce = pindex->nHeight;
    if (!DeploymentActiveAt(*pindex, m_chainman, Consensus::DEPLOYMENT_SEGWIT)) return;
    uint256 hashBlock(pblock->GetHash());
    const std::shared_future<CSerializedNetMsg> lazy_ser{
        std::async(std::launch::deferred, [&] { return msgMaker.Make(NetMsgType::CMPCTBLOCK, *pcmpctblock); })};
    {
        LOCK(m_most_recent_block_mutex);
        m_most_recent_block_hash = hashBlock;
        m_most_recent_block = pblock;
        m_most_recent_compact_block = pcmpctblock;
    }
    m_connman.ForEachNode([this, pindex, &lazy_ser, &hashBlock](CNode* pnode) EXCLUSIVE_LOCKS_REQUIRED(::cs_main) {
        AssertLockHeld(::cs_main);
        if (pnode->GetCommonVersion() < INVALID_CB_NO_BAN_VERSION || pnode->fDisconnect)
            return;
        ProcessBlockAvailability(pnode->GetId());
        CNodeState &state = *State(pnode->GetId());
        if (state.m_requested_hb_cmpctblocks && !PeerHasHeader(&state, pindex) && PeerHasHeader(&state, pindex->pprev)) {
            LogPrint(BCLog::NET, "%s sending header-and-ids %s to peer=%d\n", "PeerManager::NewPoWValidBlock",
                    hashBlock.ToString(), pnode->GetId());
            const CSerializedNetMsg& ser_cmpctblock{lazy_ser.get()};
            m_connman.PushMessage(pnode, ser_cmpctblock.Copy());
            state.pindexBestHeaderSent = pindex;
        }
    });
}
void PeerManagerImpl::UpdatedBlockTip(const CBlockIndex *pindexNew, const CBlockIndex *pindexFork, bool fInitialDownload)
{
    SetBestHeight(pindexNew->nHeight);
    SetServiceFlagsIBDCache(!fInitialDownload);
    if (fInitialDownload) return;
    std::vector<uint256> vHashes;
    const CBlockIndex *pindexToAnnounce = pindexNew;
    while (pindexToAnnounce != pindexFork) {
        vHashes.push_back(pindexToAnnounce->GetBlockHash());
        pindexToAnnounce = pindexToAnnounce->pprev;
        if (vHashes.size() == MAX_BLOCKS_TO_ANNOUNCE) {
            break;
        }
    }
    {
        LOCK(m_peer_mutex);
        for (auto& it : m_peer_map) {
            Peer& peer = *it.second;
            LOCK(peer.m_block_inv_mutex);
            for (const uint256& hash : reverse_iterate(vHashes)) {
                peer.m_blocks_for_headers_relay.push_back(hash);
            }
        }
    }
    m_connman.WakeMessageHandler();
}
void PeerManagerImpl::BlockChecked(const CBlock& block, const BlockValidationState& state)
{
    LOCK(cs_main);
    const uint256 hash(block.GetHash());
    std::map<uint256, std::pair<NodeId, bool>>::iterator it = mapBlockSource.find(hash);
    if (state.IsInvalid() &&
        it != mapBlockSource.end() &&
        State(it->second.first)) {
            MaybePunishNodeForBlock(  it->second.first, state,   !it->second.second);
    }
    else if (state.IsValid() &&
             !m_chainman.ActiveChainstate().IsInitialBlockDownload() &&
             mapBlocksInFlight.count(hash) == mapBlocksInFlight.size()) {
        if (it != mapBlockSource.end()) {
            MaybeSetPeerAsAnnouncingHeaderAndIDs(it->second.first);
        }
    }
    if (it != mapBlockSource.end())
        mapBlockSource.erase(it);
}
bool PeerManagerImpl::AlreadyHaveTx(const GenTxid& gtxid)
{
    if (m_chainman.ActiveChain().Tip()->GetBlockHash() != hashRecentRejectsChainTip) {
        hashRecentRejectsChainTip = m_chainman.ActiveChain().Tip()->GetBlockHash();
        m_recent_rejects.reset();
    }
    const uint256& hash = gtxid.GetHash();
    if (m_orphanage.HaveTx(gtxid)) return true;
    {
        LOCK(m_recent_confirmed_transactions_mutex);
        if (m_recent_confirmed_transactions.contains(hash)) return true;
    }
    return m_recent_rejects.contains(hash) || m_mempool.exists(gtxid);
}
bool PeerManagerImpl::AlreadyHaveBlock(const uint256& block_hash)
{
    return m_chainman.m_blockman.LookupBlockIndex(block_hash) != nullptr;
}
void PeerManagerImpl::SendPings()
{
    LOCK(m_peer_mutex);
    for(auto& it : m_peer_map) it.second->m_ping_queued = true;
}
void PeerManagerImpl::RelayTransaction(const uint256& txid, const uint256& wtxid)
{
    LOCK(m_peer_mutex);
    for(auto& it : m_peer_map) {
        Peer& peer = *it.second;
        auto tx_relay = peer.GetTxRelay();
        if (!tx_relay) continue;
        const uint256& hash{peer.m_wtxid_relay ? wtxid : txid};
        LOCK(tx_relay->m_tx_inventory_mutex);
        if (!tx_relay->m_tx_inventory_known_filter.contains(hash)) {
            tx_relay->m_tx_inventory_to_send.insert(hash);
        }
    };
}
void PeerManagerImpl::RelayAddress(NodeId originator,
                                   const CAddress& addr,
                                   bool fReachable)
{
    if (!fReachable && !addr.IsRelayable()) return;
    const uint64_t hash_addr{CServiceHash(0, 0)(addr)};
    const auto current_time{GetTime<std::chrono::seconds>()};
    const uint64_t time_addr{(static_cast<uint64_t>(count_seconds(current_time)) + hash_addr) / count_seconds(ROTATE_ADDR_RELAY_DEST_INTERVAL)};
    const CSipHasher hasher{m_connman.GetDeterministicRandomizer(RANDOMIZER_ID_ADDRESS_RELAY)
                                .Write(hash_addr)
                                .Write(time_addr)};
    FastRandomContext insecure_rand;
    unsigned int nRelayNodes = (fReachable || (hasher.Finalize() & 1)) ? 2 : 1;
    std::array<std::pair<uint64_t, Peer*>, 2> best{{{0, nullptr}, {0, nullptr}}};
    assert(nRelayNodes <= best.size());
    LOCK(m_peer_mutex);
    for (auto& [id, peer] : m_peer_map) {
        if (peer->m_addr_relay_enabled && id != originator && IsAddrCompatible(*peer, addr)) {
            uint64_t hashKey = CSipHasher(hasher).Write(id).Finalize();
            for (unsigned int i = 0; i < nRelayNodes; i++) {
                 if (hashKey > best[i].first) {
                     std::copy(best.begin() + i, best.begin() + nRelayNodes - 1, best.begin() + i + 1);
                     best[i] = std::make_pair(hashKey, peer.get());
                     break;
                 }
            }
        }
    };
    for (unsigned int i = 0; i < nRelayNodes && best[i].first != 0; i++) {
        PushAddress(*best[i].second, addr, insecure_rand);
    }
}
void PeerManagerImpl::ProcessGetBlockData(CNode& pfrom, Peer& peer, const CInv& inv)
{
    std::shared_ptr<const CBlock> a_recent_block;
    std::shared_ptr<const CBlockHeaderAndShortTxIDs> a_recent_compact_block;
    {
        LOCK(m_most_recent_block_mutex);
        a_recent_block = m_most_recent_block;
        a_recent_compact_block = m_most_recent_compact_block;
    }
    bool need_activate_chain = false;
    {
        LOCK(cs_main);
        const CBlockIndex* pindex = m_chainman.m_blockman.LookupBlockIndex(inv.hash);
        if (pindex) {
            if (pindex->HaveTxsDownloaded() && !pindex->IsValid(BLOCK_VALID_SCRIPTS) &&
                    pindex->IsValid(BLOCK_VALID_TREE)) {
                need_activate_chain = true;
            }
        }
    }
    if (need_activate_chain) {
        BlockValidationState state;
        if (!m_chainman.ActiveChainstate().ActivateBestChain(state, a_recent_block)) {
            LogPrint(BCLog::NET, "failed to activate chain (%s)\n", state.ToString());
        }
    }
    LOCK(cs_main);
    const CBlockIndex* pindex = m_chainman.m_blockman.LookupBlockIndex(inv.hash);
    if (!pindex) {
        return;
    }
    if (!BlockRequestAllowed(pindex)) {
        LogPrint(BCLog::NET, "%s: ignoring request from peer=%i for old block that isn't in the main chain\n", __func__, pfrom.GetId());
        return;
    }
    const CNetMsgMaker msgMaker(pfrom.GetCommonVersion());
    if (m_connman.OutboundTargetReached(true) &&
        (((m_chainman.m_best_header != nullptr) && (m_chainman.m_best_header->GetBlockTime() - pindex->GetBlockTime() > HISTORICAL_BLOCK_AGE)) || inv.IsMsgFilteredBlk()) &&
        !pfrom.HasPermission(NetPermissionFlags::Download)
    ) {
        LogPrint(BCLog::NET, "historical block serving limit reached, disconnect peer=%d\n", pfrom.GetId());
        pfrom.fDisconnect = true;
        return;
    }
    if (!pfrom.HasPermission(NetPermissionFlags::NoBan) && (
            (((peer.m_our_services & NODE_NETWORK_LIMITED) == NODE_NETWORK_LIMITED) && ((peer.m_our_services & NODE_NETWORK) != NODE_NETWORK) && (m_chainman.ActiveChain().Tip()->nHeight - pindex->nHeight > (int)NODE_NETWORK_LIMITED_MIN_BLOCKS + 2  ) )
       )) {
        LogPrint(BCLog::NET, "Ignore block request below NODE_NETWORK_LIMITED threshold, disconnect peer=%d\n", pfrom.GetId());
        pfrom.fDisconnect = true;
        return;
    }
    if (!(pindex->nStatus & BLOCK_HAVE_DATA)) {
        return;
    }
    std::shared_ptr<const CBlock> pblock;
    if (a_recent_block && a_recent_block->GetHash() == pindex->GetBlockHash()) {
        pblock = a_recent_block;
    } else if (inv.IsMsgWitnessBlk()) {
        std::vector<uint8_t> block_data;
        if (!ReadRawBlockFromDisk(block_data, pindex->GetBlockPos(), m_chainparams.MessageStart())) {
            assert(!"cannot load block from disk");
        }
        m_connman.PushMessage(&pfrom, msgMaker.Make(NetMsgType::BLOCK, Span{block_data}));
    } else {
        std::shared_ptr<CBlock> pblockRead = std::make_shared<CBlock>();
        if (!ReadBlockFromDisk(*pblockRead, pindex, m_chainparams.GetConsensus())) {
            assert(!"cannot load block from disk");
        }
        pblock = pblockRead;
    }
    if (pblock) {
        if (inv.IsMsgBlk()) {
            m_connman.PushMessage(&pfrom, msgMaker.Make(SERIALIZE_TRANSACTION_NO_WITNESS, NetMsgType::BLOCK, *pblock));
        } else if (inv.IsMsgWitnessBlk()) {
            m_connman.PushMessage(&pfrom, msgMaker.Make(NetMsgType::BLOCK, *pblock));
        } else if (inv.IsMsgFilteredBlk()) {
            bool sendMerkleBlock = false;
            CMerkleBlock merkleBlock;
            if (auto tx_relay = peer.GetTxRelay(); tx_relay != nullptr) {
                LOCK(tx_relay->m_bloom_filter_mutex);
                if (tx_relay->m_bloom_filter) {
                    sendMerkleBlock = true;
                    merkleBlock = CMerkleBlock(*pblock, *tx_relay->m_bloom_filter);
                }
            }
            if (sendMerkleBlock) {
                m_connman.PushMessage(&pfrom, msgMaker.Make(NetMsgType::MERKLEBLOCK, merkleBlock));
                typedef std::pair<unsigned int, uint256> PairType;
                for (PairType& pair : merkleBlock.vMatchedTxn)
                    m_connman.PushMessage(&pfrom, msgMaker.Make(SERIALIZE_TRANSACTION_NO_WITNESS, NetMsgType::TX, *pblock->vtx[pair.first]));
            }
        } else if (inv.IsMsgCmpctBlk()) {
            if (CanDirectFetch() && pindex->nHeight >= m_chainman.ActiveChain().Height() - MAX_CMPCTBLOCK_DEPTH) {
                if (a_recent_compact_block && a_recent_compact_block->header.GetHash() == pindex->GetBlockHash()) {
                    m_connman.PushMessage(&pfrom, msgMaker.Make(NetMsgType::CMPCTBLOCK, *a_recent_compact_block));
                } else {
                    CBlockHeaderAndShortTxIDs cmpctblock{*pblock};
                    m_connman.PushMessage(&pfrom, msgMaker.Make(NetMsgType::CMPCTBLOCK, cmpctblock));
                }
            } else {
                m_connman.PushMessage(&pfrom, msgMaker.Make(NetMsgType::BLOCK, *pblock));
            }
        }
    }
    {
        LOCK(peer.m_block_inv_mutex);
        if (inv.hash == peer.m_continuation_block) {
            std::vector<CInv> vInv;
            vInv.push_back(CInv(MSG_BLOCK, m_chainman.ActiveChain().Tip()->GetBlockHash()));
            m_connman.PushMessage(&pfrom, msgMaker.Make(NetMsgType::INV, vInv));
            peer.m_continuation_block.SetNull();
        }
    }
}
CTransactionRef PeerManagerImpl::FindTxForGetData(const CNode& peer, const GenTxid& gtxid, const std::chrono::seconds mempool_req, const std::chrono::seconds now)
{
    auto txinfo = m_mempool.info(gtxid);
    if (txinfo.tx) {
        if ((mempool_req.count() && txinfo.m_time <= mempool_req) || txinfo.m_time <= now - UNCONDITIONAL_RELAY_DELAY) {
            return std::move(txinfo.tx);
        }
    }
    {
        LOCK(cs_main);
        if (State(peer.GetId())->m_recently_announced_invs.contains(gtxid.GetHash())) {
            if (txinfo.tx) return std::move(txinfo.tx);
            auto mi = mapRelay.find(gtxid.GetHash());
            if (mi != mapRelay.end()) return mi->second;
        }
    }
    return {};
}
void PeerManagerImpl::ProcessGetData(CNode& pfrom, Peer& peer, const std::atomic<bool>& interruptMsgProc)
{
    AssertLockNotHeld(cs_main);
    auto tx_relay = peer.GetTxRelay();
    std::deque<CInv>::iterator it = peer.m_getdata_requests.begin();
    std::vector<CInv> vNotFound;
    const CNetMsgMaker msgMaker(pfrom.GetCommonVersion());
    const auto now{GetTime<std::chrono::seconds>()};
    const auto mempool_req = tx_relay != nullptr ? tx_relay->m_last_mempool_req.load() : std::chrono::seconds::min();
    while (it != peer.m_getdata_requests.end() && it->IsGenTxMsg()) {
        if (interruptMsgProc) return;
        if (pfrom.fPauseSend) break;
        const CInv &inv = *it++;
        if (tx_relay == nullptr) {
            continue;
        }
        CTransactionRef tx = FindTxForGetData(pfrom, ToGenTxid(inv), mempool_req, now);
        if (tx) {
            int nSendFlags = (inv.IsMsgTx() ? SERIALIZE_TRANSACTION_NO_WITNESS : 0);
            m_connman.PushMessage(&pfrom, msgMaker.Make(nSendFlags, NetMsgType::TX, *tx));
            m_mempool.RemoveUnbroadcastTx(tx->GetHash());
            std::vector<uint256> parent_ids_to_add;
            {
                LOCK(m_mempool.cs);
                auto txiter = m_mempool.GetIter(tx->GetHash());
                if (txiter) {
                    const CTxMemPoolEntry::Parents& parents = (*txiter)->GetMemPoolParentsConst();
                    parent_ids_to_add.reserve(parents.size());
                    for (const CTxMemPoolEntry& parent : parents) {
                        if (parent.GetTime() > now - UNCONDITIONAL_RELAY_DELAY) {
                            parent_ids_to_add.push_back(parent.GetTx().GetHash());
                        }
                    }
                }
            }
            for (const uint256& parent_txid : parent_ids_to_add) {
                if (WITH_LOCK(tx_relay->m_tx_inventory_mutex, return !tx_relay->m_tx_inventory_known_filter.contains(parent_txid))) {
                    LOCK(cs_main);
                    State(pfrom.GetId())->m_recently_announced_invs.insert(parent_txid);
                }
            }
        } else {
            vNotFound.push_back(inv);
        }
    }
    if (it != peer.m_getdata_requests.end() && !pfrom.fPauseSend) {
        const CInv &inv = *it++;
        if (inv.IsGenBlkMsg()) {
            ProcessGetBlockData(pfrom, peer, inv);
        }
    }
    peer.m_getdata_requests.erase(peer.m_getdata_requests.begin(), it);
    if (!vNotFound.empty()) {
        m_connman.PushMessage(&pfrom, msgMaker.Make(NetMsgType::NOTFOUND, vNotFound));
    }
}
uint32_t PeerManagerImpl::GetFetchFlags(const Peer& peer) const
{
    uint32_t nFetchFlags = 0;
    if (CanServeWitnesses(peer)) {
        nFetchFlags |= MSG_WITNESS_FLAG;
    }
    return nFetchFlags;
}
void PeerManagerImpl::SendBlockTransactions(CNode& pfrom, Peer& peer, const CBlock& block, const BlockTransactionsRequest& req)
{
    BlockTransactions resp(req);
    for (size_t i = 0; i < req.indexes.size(); i++) {
        if (req.indexes[i] >= block.vtx.size()) {
            Misbehaving(peer, 100, "getblocktxn with out-of-bounds tx indices");
            return;
        }
        resp.txn[i] = block.vtx[req.indexes[i]];
    }
    const CNetMsgMaker msgMaker(pfrom.GetCommonVersion());
    m_connman.PushMessage(&pfrom, msgMaker.Make(NetMsgType::BLOCKTXN, resp));
}
bool PeerManagerImpl::CheckHeadersPoW(const std::vector<CBlockHeader>& headers, const Consensus::Params& consensusParams, Peer& peer)
{
    if (!HasValidProofOfWork(headers, consensusParams)) {
        Misbehaving(peer, 100, "header with invalid proof of work");
        return false;
    }
    if (!CheckHeadersAreContinuous(headers)) {
        Misbehaving(peer, 20, "non-continuous headers sequence");
        return false;
    }
    return true;
}
arith_uint256 PeerManagerImpl::GetAntiDoSWorkThreshold()
{
    arith_uint256 near_chaintip_work = 0;
    LOCK(cs_main);
    if (m_chainman.ActiveChain().Tip() != nullptr) {
        const CBlockIndex *tip = m_chainman.ActiveChain().Tip();
        near_chaintip_work = tip->nChainWork - std::min<arith_uint256>(144*GetBlockProof(*tip), tip->nChainWork);
    }
    return std::max(near_chaintip_work, arith_uint256(nMinimumChainWork));
}
void PeerManagerImpl::HandleFewUnconnectingHeaders(CNode& pfrom, Peer& peer,
        const std::vector<CBlockHeader>& headers)
{
    const CNetMsgMaker msgMaker(pfrom.GetCommonVersion());
    LOCK(cs_main);
    CNodeState *nodestate = State(pfrom.GetId());
    nodestate->nUnconnectingHeaders++;
    if (MaybeSendGetHeaders(pfrom, GetLocator(m_chainman.m_best_header), peer)) {
        LogPrint(BCLog::NET, "received header %s: missing prev block %s, sending getheaders (%d) to end (peer=%d, nUnconnectingHeaders=%d)\n",
            headers[0].GetHash().ToString(),
            headers[0].hashPrevBlock.ToString(),
            m_chainman.m_best_header->nHeight,
            pfrom.GetId(), nodestate->nUnconnectingHeaders);
    }
    UpdateBlockAvailability(pfrom.GetId(), headers.back().GetHash());
    if (nodestate->nUnconnectingHeaders % MAX_UNCONNECTING_HEADERS == 0) {
        Misbehaving(peer, 20, strprintf("%d non-connecting headers", nodestate->nUnconnectingHeaders));
    }
}
bool PeerManagerImpl::CheckHeadersAreContinuous(const std::vector<CBlockHeader>& headers) const
{
    uint256 hashLastBlock;
    for (const CBlockHeader& header : headers) {
        if (!hashLastBlock.IsNull() && header.hashPrevBlock != hashLastBlock) {
            return false;
        }
        hashLastBlock = header.GetHash();
    }
    return true;
}
bool PeerManagerImpl::IsContinuationOfLowWorkHeadersSync(Peer& peer, CNode& pfrom, std::vector<CBlockHeader>& headers)
{
    if (peer.m_headers_sync) {
        auto result = peer.m_headers_sync->ProcessNextHeaders(headers, headers.size() == MAX_HEADERS_RESULTS);
        if (result.request_more) {
            auto locator = peer.m_headers_sync->NextHeadersRequestLocator();
            Assume(!locator.vHave.empty());
            if (!locator.vHave.empty()) {
                bool sent_getheaders = MaybeSendGetHeaders(pfrom, locator, peer);
                if (sent_getheaders) {
                    LogPrint(BCLog::NET, "more getheaders (from %s) to peer=%d\n",
                            locator.vHave.front().ToString(), pfrom.GetId());
                } else {
                    LogPrint(BCLog::NET, "error sending next getheaders (from %s) to continue sync with peer=%d\n",
                            locator.vHave.front().ToString(), pfrom.GetId());
                }
            }
        }
        if (peer.m_headers_sync->GetState() == HeadersSyncState::State::FINAL) {
            peer.m_headers_sync.reset(nullptr);
            LOCK(m_headers_presync_mutex);
            m_headers_presync_stats.erase(pfrom.GetId());
        } else {
            HeadersPresyncStats stats;
            stats.first = peer.m_headers_sync->GetPresyncWork();
            if (peer.m_headers_sync->GetState() == HeadersSyncState::State::PRESYNC) {
                stats.second = {peer.m_headers_sync->GetPresyncHeight(),
                                peer.m_headers_sync->GetPresyncTime()};
            }
            LOCK(m_headers_presync_mutex);
            m_headers_presync_stats[pfrom.GetId()] = stats;
            auto best_it = m_headers_presync_stats.find(m_headers_presync_bestpeer);
            bool best_updated = false;
            if (best_it == m_headers_presync_stats.end()) {
                NodeId peer_best{-1};
                const HeadersPresyncStats* stat_best{nullptr};
                for (const auto& [peer, stat] : m_headers_presync_stats) {
                    if (!stat_best || stat > *stat_best) {
                        peer_best = peer;
                        stat_best = &stat;
                    }
                }
                m_headers_presync_bestpeer = peer_best;
                best_updated = (peer_best == pfrom.GetId());
            } else if (best_it->first == pfrom.GetId() || stats > best_it->second) {
                m_headers_presync_bestpeer = pfrom.GetId();
                best_updated = true;
            }
            if (best_updated && stats.second.has_value()) {
                m_headers_presync_should_signal = true;
            }
        }
        if (result.success) {
            headers.swap(result.pow_validated_headers);
        }
        return result.success;
    }
    return false;
}
bool PeerManagerImpl::TryLowWorkHeadersSync(Peer& peer, CNode& pfrom, const CBlockIndex* chain_start_header, std::vector<CBlockHeader>& headers)
{
    arith_uint256 total_work = chain_start_header->nChainWork + CalculateHeadersWork(headers);
    arith_uint256 minimum_chain_work = GetAntiDoSWorkThreshold();
    if (total_work < minimum_chain_work) {
        if (headers.size() == MAX_HEADERS_RESULTS) {
            LOCK(peer.m_headers_sync_mutex);
            peer.m_headers_sync.reset(new HeadersSyncState(peer.m_id, m_chainparams.GetConsensus(),
                chain_start_header, minimum_chain_work));
            return IsContinuationOfLowWorkHeadersSync(peer, pfrom, headers);
        } else {
            LogPrint(BCLog::NET, "Ignoring low-work chain (height=%u) from peer=%d\n", chain_start_header->nHeight + headers.size(), pfrom.GetId());
            headers = {};
            return true;
        }
    }
    return false;
}
bool PeerManagerImpl::IsAncestorOfBestHeaderOrTip(const CBlockIndex* header)
{
    if (header == nullptr) {
        return false;
    } else if (m_chainman.m_best_header != nullptr && header == m_chainman.m_best_header->GetAncestor(header->nHeight)) {
        return true;
    } else if (m_chainman.ActiveChain().Contains(header)) {
        return true;
    }
    return false;
}
bool PeerManagerImpl::MaybeSendGetHeaders(CNode& pfrom, const CBlockLocator& locator, Peer& peer)
{
    const CNetMsgMaker msgMaker(pfrom.GetCommonVersion());
    const auto current_time = NodeClock::now();
    if (current_time - peer.m_last_getheaders_timestamp > HEADERS_RESPONSE_TIME) {
        m_connman.PushMessage(&pfrom, msgMaker.Make(NetMsgType::GETHEADERS, locator, uint256()));
        peer.m_last_getheaders_timestamp = current_time;
        return true;
    }
    return false;
}
void PeerManagerImpl::HeadersDirectFetchBlocks(CNode& pfrom, const Peer& peer, const CBlockIndex* pindexLast)
{
    const CNetMsgMaker msgMaker(pfrom.GetCommonVersion());
    LOCK(cs_main);
    CNodeState *nodestate = State(pfrom.GetId());
    if (CanDirectFetch() && pindexLast->IsValid(BLOCK_VALID_TREE) && m_chainman.ActiveChain().Tip()->nChainWork <= pindexLast->nChainWork) {
        std::vector<const CBlockIndex*> vToFetch;
        const CBlockIndex *pindexWalk = pindexLast;
        while (pindexWalk && !m_chainman.ActiveChain().Contains(pindexWalk) && vToFetch.size() <= MAX_BLOCKS_IN_TRANSIT_PER_PEER) {
            if (!(pindexWalk->nStatus & BLOCK_HAVE_DATA) &&
                    !IsBlockRequested(pindexWalk->GetBlockHash()) &&
                    (!DeploymentActiveAt(*pindexWalk, m_chainman, Consensus::DEPLOYMENT_SEGWIT) || CanServeWitnesses(peer))) {
                vToFetch.push_back(pindexWalk);
            }
            pindexWalk = pindexWalk->pprev;
        }
        if (!m_chainman.ActiveChain().Contains(pindexWalk)) {
            LogPrint(BCLog::NET, "Large reorg, won't direct fetch to %s (%d)\n",
                    pindexLast->GetBlockHash().ToString(),
                    pindexLast->nHeight);
        } else {
            std::vector<CInv> vGetData;
            for (const CBlockIndex *pindex : reverse_iterate(vToFetch)) {
                if (nodestate->nBlocksInFlight >= MAX_BLOCKS_IN_TRANSIT_PER_PEER) {
                    break;
                }
                uint32_t nFetchFlags = GetFetchFlags(peer);
                vGetData.push_back(CInv(MSG_BLOCK | nFetchFlags, pindex->GetBlockHash()));
                BlockRequested(pfrom.GetId(), *pindex);
                LogPrint(BCLog::NET, "Requesting block %s from  peer=%d\n",
                        pindex->GetBlockHash().ToString(), pfrom.GetId());
            }
            if (vGetData.size() > 1) {
                LogPrint(BCLog::NET, "Downloading blocks toward %s (%d) via headers direct fetch\n",
                        pindexLast->GetBlockHash().ToString(), pindexLast->nHeight);
            }
            if (vGetData.size() > 0) {
                if (!m_ignore_incoming_txs &&
                        nodestate->m_provides_cmpctblocks &&
                        vGetData.size() == 1 &&
                        mapBlocksInFlight.size() == 1 &&
                        pindexLast->pprev->IsValid(BLOCK_VALID_CHAIN)) {
                    vGetData[0] = CInv(MSG_CMPCT_BLOCK, vGetData[0].hash);
                }
                m_connman.PushMessage(&pfrom, msgMaker.Make(NetMsgType::GETDATA, vGetData));
            }
        }
    }
}
void PeerManagerImpl::UpdatePeerStateForReceivedHeaders(CNode& pfrom,
        const CBlockIndex *pindexLast, bool received_new_header, bool may_have_more_headers)
{
    LOCK(cs_main);
    CNodeState *nodestate = State(pfrom.GetId());
    if (nodestate->nUnconnectingHeaders > 0) {
        LogPrint(BCLog::NET, "peer=%d: resetting nUnconnectingHeaders (%d -> 0)\n", pfrom.GetId(), nodestate->nUnconnectingHeaders);
    }
    nodestate->nUnconnectingHeaders = 0;
    assert(pindexLast);
    UpdateBlockAvailability(pfrom.GetId(), pindexLast->GetBlockHash());
    if (received_new_header && pindexLast->nChainWork > m_chainman.ActiveChain().Tip()->nChainWork) {
        nodestate->m_last_block_announcement = GetTime();
    }
    if (m_chainman.ActiveChainstate().IsInitialBlockDownload() && !may_have_more_headers) {
        if (nodestate->pindexBestKnownBlock && nodestate->pindexBestKnownBlock->nChainWork < nMinimumChainWork) {
            if (pfrom.IsOutboundOrBlockRelayConn()) {
                LogPrintf("Disconnecting outbound peer %d -- headers chain has insufficient work\n", pfrom.GetId());
                pfrom.fDisconnect = true;
            }
        }
    }
    if (!pfrom.fDisconnect && pfrom.IsFullOutboundConn() && nodestate->pindexBestKnownBlock != nullptr) {
        if (m_outbound_peers_with_protect_from_disconnect < MAX_OUTBOUND_PEERS_TO_PROTECT_FROM_DISCONNECT && nodestate->pindexBestKnownBlock->nChainWork >= m_chainman.ActiveChain().Tip()->nChainWork && !nodestate->m_chain_sync.m_protect) {
            LogPrint(BCLog::NET, "Protecting outbound peer=%d from eviction\n", pfrom.GetId());
            nodestate->m_chain_sync.m_protect = true;
            ++m_outbound_peers_with_protect_from_disconnect;
        }
    }
}
void PeerManagerImpl::ProcessHeadersMessage(CNode& pfrom, Peer& peer,
                                            std::vector<CBlockHeader>&& headers,
                                            bool via_compact_block)
{
    size_t nCount = headers.size();
    if (nCount == 0) {
        LOCK(peer.m_headers_sync_mutex);
        if (peer.m_headers_sync) {
            peer.m_headers_sync.reset(nullptr);
            LOCK(m_headers_presync_mutex);
            m_headers_presync_stats.erase(pfrom.GetId());
        }
        return;
    }
    if (!CheckHeadersPoW(headers, m_chainparams.GetConsensus(), peer)) {
        return;
    }
    const CBlockIndex *pindexLast = nullptr;
    bool already_validated_work = false;
    bool have_headers_sync = false;
    {
        LOCK(peer.m_headers_sync_mutex);
        already_validated_work = IsContinuationOfLowWorkHeadersSync(peer, pfrom, headers);
        if (headers.empty()) {
            return;
        }
        have_headers_sync = !!peer.m_headers_sync;
    }
    const CBlockIndex *chain_start_header{WITH_LOCK(::cs_main, return m_chainman.m_blockman.LookupBlockIndex(headers[0].hashPrevBlock))};
    bool headers_connect_blockindex{chain_start_header != nullptr};
    if (!headers_connect_blockindex) {
        if (nCount <= MAX_BLOCKS_TO_ANNOUNCE) {
            HandleFewUnconnectingHeaders(pfrom, peer, headers);
        } else {
            Misbehaving(peer, 10, "invalid header received");
        }
        return;
    }
    const CBlockIndex *last_received_header{nullptr};
    {
        LOCK(cs_main);
        last_received_header = m_chainman.m_blockman.LookupBlockIndex(headers.back().GetHash());
        if (IsAncestorOfBestHeaderOrTip(last_received_header)) {
            already_validated_work = true;
        }
    }
    if (pfrom.HasPermission(NetPermissionFlags::NoBan)) {
        already_validated_work = true;
    }
    if (!already_validated_work && TryLowWorkHeadersSync(peer, pfrom,
                chain_start_header, headers)) {
        Assume(headers.empty());
        return;
    }
    bool received_new_header{last_received_header == nullptr};
    BlockValidationState state;
    if (!m_chainman.ProcessNewBlockHeaders(headers,  true, state, &pindexLast)) {
        if (state.IsInvalid()) {
            MaybePunishNodeForBlock(pfrom.GetId(), state, via_compact_block, "invalid header received");
            return;
        }
    }
    Assume(pindexLast);
    if (nCount == MAX_HEADERS_RESULTS && !have_headers_sync) {
        if (MaybeSendGetHeaders(pfrom, GetLocator(pindexLast), peer)) {
            LogPrint(BCLog::NET, "more getheaders (%d) to end to peer=%d (startheight:%d)\n",
                    pindexLast->nHeight, pfrom.GetId(), peer.m_starting_height);
        }
    }
    UpdatePeerStateForReceivedHeaders(pfrom, pindexLast, received_new_header, nCount == MAX_HEADERS_RESULTS);
    HeadersDirectFetchBlocks(pfrom, peer, pindexLast);
    return;
}
void PeerManagerImpl::ProcessOrphanTx(std::set<uint256>& orphan_work_set)
{
    AssertLockHeld(cs_main);
    AssertLockHeld(g_cs_orphans);
    while (!orphan_work_set.empty()) {
        const uint256 orphanHash = *orphan_work_set.begin();
        orphan_work_set.erase(orphan_work_set.begin());
        const auto [porphanTx, from_peer] = m_orphanage.GetTx(orphanHash);
        if (porphanTx == nullptr) continue;
        const MempoolAcceptResult result = m_chainman.ProcessTransaction(porphanTx);
        const TxValidationState& state = result.m_state;
        if (result.m_result_type == MempoolAcceptResult::ResultType::VALID) {
            LogPrint(BCLog::MEMPOOL, "   accepted orphan tx %s\n", orphanHash.ToString());
            RelayTransaction(orphanHash, porphanTx->GetWitnessHash());
            m_orphanage.AddChildrenToWorkSet(*porphanTx, orphan_work_set);
            m_orphanage.EraseTx(orphanHash);
            for (const CTransactionRef& removedTx : result.m_replaced_transactions.value()) {
                AddToCompactExtraTransactions(removedTx);
            }
            break;
        } else if (state.GetResult() != TxValidationResult::TX_MISSING_INPUTS) {
            if (state.IsInvalid()) {
                LogPrint(BCLog::MEMPOOL, "   invalid orphan tx %s from peer=%d. %s\n",
                    orphanHash.ToString(),
                    from_peer,
                    state.ToString());
                MaybePunishNodeForTx(from_peer, state);
            }
            LogPrint(BCLog::MEMPOOL, "   removed orphan tx %s\n", orphanHash.ToString());
            if (state.GetResult() != TxValidationResult::TX_WITNESS_STRIPPED) {
                m_recent_rejects.insert(porphanTx->GetWitnessHash());
                if (state.GetResult() == TxValidationResult::TX_INPUTS_NOT_STANDARD && porphanTx->GetWitnessHash() != porphanTx->GetHash()) {
                    m_recent_rejects.insert(porphanTx->GetHash());
                }
            }
            m_orphanage.EraseTx(orphanHash);
            break;
        }
    }
}
bool PeerManagerImpl::PrepareBlockFilterRequest(CNode& node, Peer& peer,
                                                BlockFilterType filter_type, uint32_t start_height,
                                                const uint256& stop_hash, uint32_t max_height_diff,
                                                const CBlockIndex*& stop_index,
                                                BlockFilterIndex*& filter_index)
{
    const bool supported_filter_type =
        (filter_type == BlockFilterType::BASIC &&
         (peer.m_our_services & NODE_COMPACT_FILTERS));
    if (!supported_filter_type) {
        LogPrint(BCLog::NET, "peer %d requested unsupported block filter type: %d\n",
                 node.GetId(), static_cast<uint8_t>(filter_type));
        node.fDisconnect = true;
        return false;
    }
    {
        LOCK(cs_main);
        stop_index = m_chainman.m_blockman.LookupBlockIndex(stop_hash);
        if (!stop_index || !BlockRequestAllowed(stop_index)) {
            LogPrint(BCLog::NET, "peer %d requested invalid block hash: %s\n",
                     node.GetId(), stop_hash.ToString());
            node.fDisconnect = true;
            return false;
        }
    }
    uint32_t stop_height = stop_index->nHeight;
    if (start_height > stop_height) {
        LogPrint(BCLog::NET, "peer %d sent invalid getcfilters/getcfheaders with "
                 "start height %d and stop height %d\n",
                 node.GetId(), start_height, stop_height);
        node.fDisconnect = true;
        return false;
    }
    if (stop_height - start_height >= max_height_diff) {
        LogPrint(BCLog::NET, "peer %d requested too many cfilters/cfheaders: %d / %d\n",
                 node.GetId(), stop_height - start_height + 1, max_height_diff);
        node.fDisconnect = true;
        return false;
    }
    filter_index = GetBlockFilterIndex(filter_type);
    if (!filter_index) {
        LogPrint(BCLog::NET, "Filter index for supported type %s not found\n", BlockFilterTypeName(filter_type));
        return false;
    }
    return true;
}
void PeerManagerImpl::ProcessGetCFilters(CNode& node,Peer& peer, CDataStream& vRecv)
{
    uint8_t filter_type_ser;
    uint32_t start_height;
    uint256 stop_hash;
    vRecv >> filter_type_ser >> start_height >> stop_hash;
    const BlockFilterType filter_type = static_cast<BlockFilterType>(filter_type_ser);
    const CBlockIndex* stop_index;
    BlockFilterIndex* filter_index;
    if (!PrepareBlockFilterRequest(node, peer, filter_type, start_height, stop_hash,
                                   MAX_GETCFILTERS_SIZE, stop_index, filter_index)) {
        return;
    }
    std::vector<BlockFilter> filters;
    if (!filter_index->LookupFilterRange(start_height, stop_index, filters)) {
        LogPrint(BCLog::NET, "Failed to find block filter in index: filter_type=%s, start_height=%d, stop_hash=%s\n",
                     BlockFilterTypeName(filter_type), start_height, stop_hash.ToString());
        return;
    }
    for (const auto& filter : filters) {
        CSerializedNetMsg msg = CNetMsgMaker(node.GetCommonVersion())
            .Make(NetMsgType::CFILTER, filter);
        m_connman.PushMessage(&node, std::move(msg));
    }
}
void PeerManagerImpl::ProcessGetCFHeaders(CNode& node, Peer& peer, CDataStream& vRecv)
{
    uint8_t filter_type_ser;
    uint32_t start_height;
    uint256 stop_hash;
    vRecv >> filter_type_ser >> start_height >> stop_hash;
    const BlockFilterType filter_type = static_cast<BlockFilterType>(filter_type_ser);
    const CBlockIndex* stop_index;
    BlockFilterIndex* filter_index;
    if (!PrepareBlockFilterRequest(node, peer, filter_type, start_height, stop_hash,
                                   MAX_GETCFHEADERS_SIZE, stop_index, filter_index)) {
        return;
    }
    uint256 prev_header;
    if (start_height > 0) {
        const CBlockIndex* const prev_block =
            stop_index->GetAncestor(static_cast<int>(start_height - 1));
        if (!filter_index->LookupFilterHeader(prev_block, prev_header)) {
            LogPrint(BCLog::NET, "Failed to find block filter header in index: filter_type=%s, block_hash=%s\n",
                         BlockFilterTypeName(filter_type), prev_block->GetBlockHash().ToString());
            return;
        }
    }
    std::vector<uint256> filter_hashes;
    if (!filter_index->LookupFilterHashRange(start_height, stop_index, filter_hashes)) {
        LogPrint(BCLog::NET, "Failed to find block filter hashes in index: filter_type=%s, start_height=%d, stop_hash=%s\n",
                     BlockFilterTypeName(filter_type), start_height, stop_hash.ToString());
        return;
    }
    CSerializedNetMsg msg = CNetMsgMaker(node.GetCommonVersion())
        .Make(NetMsgType::CFHEADERS,
              filter_type_ser,
              stop_index->GetBlockHash(),
              prev_header,
              filter_hashes);
    m_connman.PushMessage(&node, std::move(msg));
}
void PeerManagerImpl::ProcessGetCFCheckPt(CNode& node, Peer& peer, CDataStream& vRecv)
{
    uint8_t filter_type_ser;
    uint256 stop_hash;
    vRecv >> filter_type_ser >> stop_hash;
    const BlockFilterType filter_type = static_cast<BlockFilterType>(filter_type_ser);
    const CBlockIndex* stop_index;
    BlockFilterIndex* filter_index;
    if (!PrepareBlockFilterRequest(node, peer, filter_type,  0, stop_hash,
                                    std::numeric_limits<uint32_t>::max(),
                                   stop_index, filter_index)) {
        return;
    }
    std::vector<uint256> headers(stop_index->nHeight / CFCHECKPT_INTERVAL);
    const CBlockIndex* block_index = stop_index;
    for (int i = headers.size() - 1; i >= 0; i--) {
        int height = (i + 1) * CFCHECKPT_INTERVAL;
        block_index = block_index->GetAncestor(height);
        if (!filter_index->LookupFilterHeader(block_index, headers[i])) {
            LogPrint(BCLog::NET, "Failed to find block filter header in index: filter_type=%s, block_hash=%s\n",
                         BlockFilterTypeName(filter_type), block_index->GetBlockHash().ToString());
            return;
        }
    }
    CSerializedNetMsg msg = CNetMsgMaker(node.GetCommonVersion())
        .Make(NetMsgType::CFCHECKPT,
              filter_type_ser,
              stop_index->GetBlockHash(),
              headers);
    m_connman.PushMessage(&node, std::move(msg));
}
void PeerManagerImpl::ProcessBlock(CNode& node, const std::shared_ptr<const CBlock>& block, bool force_processing, bool min_pow_checked)
{
    bool new_block{false};
    m_chainman.ProcessNewBlock(block, force_processing, min_pow_checked, &new_block);
    if (new_block) {
        node.m_last_block_time = GetTime<std::chrono::seconds>();
    } else {
        LOCK(cs_main);
        mapBlockSource.erase(block->GetHash());
    }
}
void PeerManagerImpl::ProcessMessage(CNode& pfrom, const std::string& msg_type, CDataStream& vRecv,
                                     const std::chrono::microseconds time_received,
                                     const std::atomic<bool>& interruptMsgProc)
{
    AssertLockHeld(g_msgproc_mutex);
    LogPrint(BCLog::NET, "received: %s (%u bytes) peer=%d\n", SanitizeString(msg_type), vRecv.size(), pfrom.GetId());
    PeerRef peer = GetPeerRef(pfrom.GetId());
    if (peer == nullptr) return;
    if (msg_type == NetMsgType::VERSION) {
        if (pfrom.nVersion != 0) {
            LogPrint(BCLog::NET, "redundant version message from peer=%d\n", pfrom.GetId());
            return;
        }
        int64_t nTime;
        CService addrMe;
        uint64_t nNonce = 1;
        ServiceFlags nServices;
        int nVersion;
        std::string cleanSubVer;
        int starting_height = -1;
        bool fRelay = true;
        vRecv >> nVersion >> Using<CustomUintFormatter<8>>(nServices) >> nTime;
        if (nTime < 0) {
            nTime = 0;
        }
        vRecv.ignore(8);
        vRecv >> addrMe;
        if (!pfrom.IsInboundConn())
        {
            m_addrman.SetServices(pfrom.addr, nServices);
        }
        if (pfrom.ExpectServicesFromConn() && !HasAllDesirableServiceFlags(nServices))
        {
            LogPrint(BCLog::NET, "peer=%d does not offer the expected services (%08x offered, %08x expected); disconnecting\n", pfrom.GetId(), nServices, GetDesirableServiceFlags(nServices));
            pfrom.fDisconnect = true;
            return;
        }
        if (nVersion < MIN_PEER_PROTO_VERSION) {
            LogPrint(BCLog::NET, "peer=%d using obsolete version %i; disconnecting\n", pfrom.GetId(), nVersion);
            pfrom.fDisconnect = true;
            return;
        }
        if (!vRecv.empty()) {
            vRecv.ignore(26);
            vRecv >> nNonce;
        }
        if (!vRecv.empty()) {
            std::string strSubVer;
            vRecv >> LIMITED_STRING(strSubVer, MAX_SUBVERSION_LENGTH);
            cleanSubVer = SanitizeString(strSubVer);
        }
        if (!vRecv.empty()) {
            vRecv >> starting_height;
        }
        if (!vRecv.empty())
            vRecv >> fRelay;
        if (pfrom.IsInboundConn() && !m_connman.CheckIncomingNonce(nNonce))
        {
            LogPrintf("connected to self at %s, disconnecting\n", pfrom.addr.ToString());
            pfrom.fDisconnect = true;
            return;
        }
        if (pfrom.IsInboundConn() && addrMe.IsRoutable())
        {
            SeenLocal(addrMe);
        }
        if (pfrom.IsInboundConn()) {
            PushNodeVersion(pfrom, *peer);
        }
        const int greatest_common_version = std::min(nVersion, PROTOCOL_VERSION);
        pfrom.SetCommonVersion(greatest_common_version);
        pfrom.nVersion = nVersion;
        const CNetMsgMaker msg_maker(greatest_common_version);
        if (greatest_common_version >= WTXID_RELAY_VERSION) {
            m_connman.PushMessage(&pfrom, msg_maker.Make(NetMsgType::WTXIDRELAY));
        }
        if (greatest_common_version >= 70016) {
            m_connman.PushMessage(&pfrom, msg_maker.Make(NetMsgType::SENDADDRV2));
        }
        m_connman.PushMessage(&pfrom, msg_maker.Make(NetMsgType::VERACK));
        pfrom.m_has_all_wanted_services = HasAllDesirableServiceFlags(nServices);
        peer->m_their_services = nServices;
        pfrom.SetAddrLocal(addrMe);
        {
            LOCK(pfrom.m_subver_mutex);
            pfrom.cleanSubVer = cleanSubVer;
        }
        peer->m_starting_height = starting_height;
        if (!pfrom.IsBlockOnlyConn() &&
            (fRelay || (peer->m_our_services & NODE_BLOOM))) {
            auto* const tx_relay = peer->SetTxRelay();
            {
                LOCK(tx_relay->m_bloom_filter_mutex);
                tx_relay->m_relay_txs = fRelay;
            }
            if (fRelay) pfrom.m_relays_txs = true;
        }
        {
            LOCK(cs_main);
            CNodeState* state = State(pfrom.GetId());
            state->fPreferredDownload = (!pfrom.IsInboundConn() || pfrom.HasPermission(NetPermissionFlags::NoBan)) && !pfrom.IsAddrFetchConn() && CanServeBlocks(*peer);
            m_num_preferred_download_peers += state->fPreferredDownload;
        }
        if (!pfrom.IsInboundConn() && SetupAddressRelay(pfrom, *peer)) {
            if (fListen && !m_chainman.ActiveChainstate().IsInitialBlockDownload())
            {
                CAddress addr{GetLocalAddress(pfrom.addr), peer->m_our_services, Now<NodeSeconds>()};
                FastRandomContext insecure_rand;
                if (addr.IsRoutable())
                {
                    LogPrint(BCLog::NET, "ProcessMessages: advertising address %s\n", addr.ToString());
                    PushAddress(*peer, addr, insecure_rand);
                } else if (IsPeerAddrLocalGood(&pfrom)) {
                    addr.SetIP(addrMe);
                    LogPrint(BCLog::NET, "ProcessMessages: advertising address %s\n", addr.ToString());
                    PushAddress(*peer, addr, insecure_rand);
                }
            }
            m_connman.PushMessage(&pfrom, CNetMsgMaker(greatest_common_version).Make(NetMsgType::GETADDR));
            peer->m_getaddr_sent = true;
            peer->m_addr_token_bucket += MAX_ADDR_TO_SEND;
        }
        if (!pfrom.IsInboundConn()) {
            m_addrman.Good(pfrom.addr);
        }
        std::string remoteAddr;
        if (fLogIPs)
            remoteAddr = ", peeraddr=" + pfrom.addr.ToString();
        LogPrint(BCLog::NET, "receive version message: %s: version %d, blocks=%d, us=%s, txrelay=%d, peer=%d%s\n",
                  cleanSubVer, pfrom.nVersion,
                  peer->m_starting_height, addrMe.ToString(), fRelay, pfrom.GetId(),
                  remoteAddr);
        int64_t nTimeOffset = nTime - GetTime();
        pfrom.nTimeOffset = nTimeOffset;
        if (!pfrom.IsInboundConn()) {
            AddTimeData(pfrom.addr, nTimeOffset);
        }
        if (greatest_common_version <= 70012) {
            CDataStream finalAlert(ParseHex("60010000000000000000000000ffffff7f00000000ffffff7ffeffff7f01ffffff7f00000000ffffff7f00ffffff7f002f555247454e543a20416c657274206b657920636f6d70726f6d697365642c2075706772616465207265717569726564004630440220653febd6410f470f6bae11cad19c48413becb1ac2c17f908fd0fd53bdc3abd5202206d0e9c96fe88d4a0f01ed9dedae2b6f9e00da94cad0fecaae66ecf689bf71b50"), SER_NETWORK, PROTOCOL_VERSION);
            m_connman.PushMessage(&pfrom, CNetMsgMaker(greatest_common_version).Make("alert", finalAlert));
        }
        if (pfrom.IsFeelerConn()) {
            LogPrint(BCLog::NET, "feeler connection completed peer=%d; disconnecting\n", pfrom.GetId());
            pfrom.fDisconnect = true;
        }
        return;
    }
    if (pfrom.nVersion == 0) {
        LogPrint(BCLog::NET, "non-version message before version handshake. Message \"%s\" from peer=%d\n", SanitizeString(msg_type), pfrom.GetId());
        return;
    }
    const CNetMsgMaker msgMaker(pfrom.GetCommonVersion());
    if (msg_type == NetMsgType::VERACK) {
        if (pfrom.fSuccessfullyConnected) {
            LogPrint(BCLog::NET, "ignoring redundant verack message from peer=%d\n", pfrom.GetId());
            return;
        }
        if (!pfrom.IsInboundConn()) {
            LogPrintf("New outbound peer connected: version: %d, blocks=%d, peer=%d%s (%s)\n",
                      pfrom.nVersion.load(), peer->m_starting_height,
                      pfrom.GetId(), (fLogIPs ? strprintf(", peeraddr=%s", pfrom.addr.ToString()) : ""),
                      pfrom.ConnectionTypeAsString());
        }
        if (pfrom.GetCommonVersion() >= SHORT_IDS_BLOCKS_VERSION) {
            m_connman.PushMessage(&pfrom, msgMaker.Make(NetMsgType::SENDCMPCT,  false,  CMPCTBLOCKS_VERSION));
        }
        pfrom.fSuccessfullyConnected = true;
        return;
    }
    if (msg_type == NetMsgType::SENDHEADERS) {
        LOCK(cs_main);
        State(pfrom.GetId())->fPreferHeaders = true;
        return;
    }
    if (msg_type == NetMsgType::SENDCMPCT) {
        bool sendcmpct_hb{false};
        uint64_t sendcmpct_version{0};
        vRecv >> sendcmpct_hb >> sendcmpct_version;
        if (sendcmpct_version != CMPCTBLOCKS_VERSION) return;
        LOCK(cs_main);
        CNodeState* nodestate = State(pfrom.GetId());
        nodestate->m_provides_cmpctblocks = true;
        nodestate->m_requested_hb_cmpctblocks = sendcmpct_hb;
        pfrom.m_bip152_highbandwidth_from = sendcmpct_hb;
        return;
    }
    if (msg_type == NetMsgType::WTXIDRELAY) {
        if (pfrom.fSuccessfullyConnected) {
            LogPrint(BCLog::NET, "wtxidrelay received after verack from peer=%d; disconnecting\n", pfrom.GetId());
            pfrom.fDisconnect = true;
            return;
        }
        if (pfrom.GetCommonVersion() >= WTXID_RELAY_VERSION) {
            if (!peer->m_wtxid_relay) {
                peer->m_wtxid_relay = true;
                m_wtxid_relay_peers++;
            } else {
                LogPrint(BCLog::NET, "ignoring duplicate wtxidrelay from peer=%d\n", pfrom.GetId());
            }
        } else {
            LogPrint(BCLog::NET, "ignoring wtxidrelay due to old common version=%d from peer=%d\n", pfrom.GetCommonVersion(), pfrom.GetId());
        }
        return;
    }
    if (msg_type == NetMsgType::SENDADDRV2) {
        if (pfrom.fSuccessfullyConnected) {
            LogPrint(BCLog::NET, "sendaddrv2 received after verack from peer=%d; disconnecting\n", pfrom.GetId());
            pfrom.fDisconnect = true;
            return;
        }
        peer->m_wants_addrv2 = true;
        return;
    }
    if (!pfrom.fSuccessfullyConnected) {
        LogPrint(BCLog::NET, "Unsupported message \"%s\" prior to verack from peer=%d\n", SanitizeString(msg_type), pfrom.GetId());
        return;
    }
    if (msg_type == NetMsgType::ADDR || msg_type == NetMsgType::ADDRV2) {
        int stream_version = vRecv.GetVersion();
        if (msg_type == NetMsgType::ADDRV2) {
            stream_version |= ADDRV2_FORMAT;
        }
        OverrideStream<CDataStream> s(&vRecv, vRecv.GetType(), stream_version);
        std::vector<CAddress> vAddr;
        s >> vAddr;
        if (!SetupAddressRelay(pfrom, *peer)) {
            LogPrint(BCLog::NET, "ignoring %s message from %s peer=%d\n", msg_type, pfrom.ConnectionTypeAsString(), pfrom.GetId());
            return;
        }
        if (vAddr.size() > MAX_ADDR_TO_SEND)
        {
            Misbehaving(*peer, 20, strprintf("%s message size = %u", msg_type, vAddr.size()));
            return;
        }
        std::vector<CAddress> vAddrOk;
        const auto current_a_time{Now<NodeSeconds>()};
        const auto current_time{GetTime<std::chrono::microseconds>()};
        if (peer->m_addr_token_bucket < MAX_ADDR_PROCESSING_TOKEN_BUCKET) {
            const auto time_diff = std::max(current_time - peer->m_addr_token_timestamp, 0us);
            const double increment = Ticks<SecondsDouble>(time_diff) * MAX_ADDR_RATE_PER_SECOND;
            peer->m_addr_token_bucket = std::min<double>(peer->m_addr_token_bucket + increment, MAX_ADDR_PROCESSING_TOKEN_BUCKET);
        }
        peer->m_addr_token_timestamp = current_time;
        const bool rate_limited = !pfrom.HasPermission(NetPermissionFlags::Addr);
        uint64_t num_proc = 0;
        uint64_t num_rate_limit = 0;
        Shuffle(vAddr.begin(), vAddr.end(), FastRandomContext());
        for (CAddress& addr : vAddr)
        {
            if (interruptMsgProc)
                return;
            if (peer->m_addr_token_bucket < 1.0) {
                if (rate_limited) {
                    ++num_rate_limit;
                    continue;
                }
            } else {
                peer->m_addr_token_bucket -= 1.0;
            }
            if (!MayHaveUsefulAddressDB(addr.nServices) && !HasAllDesirableServiceFlags(addr.nServices))
                continue;
            if (addr.nTime <= NodeSeconds{100000000s} || addr.nTime > current_a_time + 10min) {
                addr.nTime = current_a_time - 5 * 24h;
            }
            AddAddressKnown(*peer, addr);
            if (m_banman && (m_banman->IsDiscouraged(addr) || m_banman->IsBanned(addr))) {
                continue;
            }
            ++num_proc;
            bool fReachable = IsReachable(addr);
            if (addr.nTime > current_a_time - 10min && !peer->m_getaddr_sent && vAddr.size() <= 10 && addr.IsRoutable()) {
                RelayAddress(pfrom.GetId(), addr, fReachable);
            }
            if (fReachable)
                vAddrOk.push_back(addr);
        }
        peer->m_addr_processed += num_proc;
        peer->m_addr_rate_limited += num_rate_limit;
        LogPrint(BCLog::NET, "Received addr: %u addresses (%u processed, %u rate-limited) from peer=%d\n",
                 vAddr.size(), num_proc, num_rate_limit, pfrom.GetId());
        m_addrman.Add(vAddrOk, pfrom.addr, 2h);
        if (vAddr.size() < 1000) peer->m_getaddr_sent = false;
        if (pfrom.IsAddrFetchConn() && vAddr.size() > 1) {
            LogPrint(BCLog::NET, "addrfetch connection completed peer=%d; disconnecting\n", pfrom.GetId());
            pfrom.fDisconnect = true;
        }
        return;
    }
    if (msg_type == NetMsgType::INV) {
        std::vector<CInv> vInv;
        vRecv >> vInv;
        if (vInv.size() > MAX_INV_SZ)
        {
            Misbehaving(*peer, 20, strprintf("inv message size = %u", vInv.size()));
            return;
        }
        const bool reject_tx_invs{RejectIncomingTxs(pfrom)};
        LOCK(cs_main);
        const auto current_time{GetTime<std::chrono::microseconds>()};
        uint256* best_block{nullptr};
        for (CInv& inv : vInv) {
            if (interruptMsgProc) return;
            if (peer->m_wtxid_relay) {
                if (inv.IsMsgTx()) continue;
            } else {
                if (inv.IsMsgWtx()) continue;
            }
            if (inv.IsMsgBlk()) {
                const bool fAlreadyHave = AlreadyHaveBlock(inv.hash);
                LogPrint(BCLog::NET, "got inv: %s  %s peer=%d\n", inv.ToString(), fAlreadyHave ? "have" : "new", pfrom.GetId());
                UpdateBlockAvailability(pfrom.GetId(), inv.hash);
                if (!fAlreadyHave && !fImporting && !fReindex && !IsBlockRequested(inv.hash)) {
                    best_block = &inv.hash;
                }
            } else if (inv.IsGenTxMsg()) {
                if (reject_tx_invs) {
                    LogPrint(BCLog::NET, "transaction (%s) inv sent in violation of protocol, disconnecting peer=%d\n", inv.hash.ToString(), pfrom.GetId());
                    pfrom.fDisconnect = true;
                    return;
                }
                const GenTxid gtxid = ToGenTxid(inv);
                const bool fAlreadyHave = AlreadyHaveTx(gtxid);
                LogPrint(BCLog::NET, "got inv: %s  %s peer=%d\n", inv.ToString(), fAlreadyHave ? "have" : "new", pfrom.GetId());
                AddKnownTx(*peer, inv.hash);
                if (!fAlreadyHave && !m_chainman.ActiveChainstate().IsInitialBlockDownload()) {
                    AddTxAnnouncement(pfrom, gtxid, current_time);
                }
            } else {
                LogPrint(BCLog::NET, "Unknown inv type \"%s\" received from peer=%d\n", inv.ToString(), pfrom.GetId());
            }
        }
        if (best_block != nullptr) {
            CNodeState& state{*Assert(State(pfrom.GetId()))};
            if (state.fSyncStarted || (!peer->m_inv_triggered_getheaders_before_sync && *best_block != m_last_block_inv_triggering_headers_sync)) {
                if (MaybeSendGetHeaders(pfrom, GetLocator(m_chainman.m_best_header), *peer)) {
                    LogPrint(BCLog::NET, "getheaders (%d) %s to peer=%d\n",
                            m_chainman.m_best_header->nHeight, best_block->ToString(),
                            pfrom.GetId());
                }
                if (!state.fSyncStarted) {
                    peer->m_inv_triggered_getheaders_before_sync = true;
                    m_last_block_inv_triggering_headers_sync = *best_block;
                }
            }
        }
        return;
    }
    if (msg_type == NetMsgType::GETDATA) {
        std::vector<CInv> vInv;
        vRecv >> vInv;
        if (vInv.size() > MAX_INV_SZ)
        {
            Misbehaving(*peer, 20, strprintf("getdata message size = %u", vInv.size()));
            return;
        }
        LogPrint(BCLog::NET, "received getdata (%u invsz) peer=%d\n", vInv.size(), pfrom.GetId());
        if (vInv.size() > 0) {
            LogPrint(BCLog::NET, "received getdata for: %s peer=%d\n", vInv[0].ToString(), pfrom.GetId());
        }
        {
            LOCK(peer->m_getdata_requests_mutex);
            peer->m_getdata_requests.insert(peer->m_getdata_requests.end(), vInv.begin(), vInv.end());
            ProcessGetData(pfrom, *peer, interruptMsgProc);
        }
        return;
    }
    if (msg_type == NetMsgType::GETBLOCKS) {
        CBlockLocator locator;
        uint256 hashStop;
        vRecv >> locator >> hashStop;
        if (locator.vHave.size() > MAX_LOCATOR_SZ) {
            LogPrint(BCLog::NET, "getblocks locator size %lld > %d, disconnect peer=%d\n", locator.vHave.size(), MAX_LOCATOR_SZ, pfrom.GetId());
            pfrom.fDisconnect = true;
            return;
        }
        {
            std::shared_ptr<const CBlock> a_recent_block;
            {
                LOCK(m_most_recent_block_mutex);
                a_recent_block = m_most_recent_block;
            }
            BlockValidationState state;
            if (!m_chainman.ActiveChainstate().ActivateBestChain(state, a_recent_block)) {
                LogPrint(BCLog::NET, "failed to activate chain (%s)\n", state.ToString());
            }
        }
        LOCK(cs_main);
        const CBlockIndex* pindex = m_chainman.ActiveChainstate().FindForkInGlobalIndex(locator);
        if (pindex)
            pindex = m_chainman.ActiveChain().Next(pindex);
        int nLimit = 500;
        LogPrint(BCLog::NET, "getblocks %d to %s limit %d from peer=%d\n", (pindex ? pindex->nHeight : -1), hashStop.IsNull() ? "end" : hashStop.ToString(), nLimit, pfrom.GetId());
        for (; pindex; pindex = m_chainman.ActiveChain().Next(pindex))
        {
            if (pindex->GetBlockHash() == hashStop)
            {
                LogPrint(BCLog::NET, "  getblocks stopping at %d %s\n", pindex->nHeight, pindex->GetBlockHash().ToString());
                break;
            }
            const int nPrunedBlocksLikelyToHave = MIN_BLOCKS_TO_KEEP - 3600 / m_chainparams.GetConsensus().nPowTargetSpacing;
            if (fPruneMode && (!(pindex->nStatus & BLOCK_HAVE_DATA) || pindex->nHeight <= m_chainman.ActiveChain().Tip()->nHeight - nPrunedBlocksLikelyToHave))
            {
                LogPrint(BCLog::NET, " getblocks stopping, pruned or too old block at %d %s\n", pindex->nHeight, pindex->GetBlockHash().ToString());
                break;
            }
            WITH_LOCK(peer->m_block_inv_mutex, peer->m_blocks_for_inv_relay.push_back(pindex->GetBlockHash()));
            if (--nLimit <= 0) {
                LogPrint(BCLog::NET, "  getblocks stopping at limit %d %s\n", pindex->nHeight, pindex->GetBlockHash().ToString());
                WITH_LOCK(peer->m_block_inv_mutex, {peer->m_continuation_block = pindex->GetBlockHash();});
                break;
            }
        }
        return;
    }
    if (msg_type == NetMsgType::GETBLOCKTXN) {
        BlockTransactionsRequest req;
        vRecv >> req;
        std::shared_ptr<const CBlock> recent_block;
        {
            LOCK(m_most_recent_block_mutex);
            if (m_most_recent_block_hash == req.blockhash)
                recent_block = m_most_recent_block;
        }
        if (recent_block) {
            SendBlockTransactions(pfrom, *peer, *recent_block, req);
            return;
        }
        {
            LOCK(cs_main);
            const CBlockIndex* pindex = m_chainman.m_blockman.LookupBlockIndex(req.blockhash);
            if (!pindex || !(pindex->nStatus & BLOCK_HAVE_DATA)) {
                LogPrint(BCLog::NET, "Peer %d sent us a getblocktxn for a block we don't have\n", pfrom.GetId());
                return;
            }
            if (pindex->nHeight >= m_chainman.ActiveChain().Height() - MAX_BLOCKTXN_DEPTH) {
                CBlock block;
                bool ret = ReadBlockFromDisk(block, pindex, m_chainparams.GetConsensus());
                assert(ret);
                SendBlockTransactions(pfrom, *peer, block, req);
                return;
            }
        }
        LogPrint(BCLog::NET, "Peer %d sent us a getblocktxn for a block > %i deep\n", pfrom.GetId(), MAX_BLOCKTXN_DEPTH);
        CInv inv{MSG_WITNESS_BLOCK, req.blockhash};
        WITH_LOCK(peer->m_getdata_requests_mutex, peer->m_getdata_requests.push_back(inv));
        return;
    }
    if (msg_type == NetMsgType::GETHEADERS) {
        CBlockLocator locator;
        uint256 hashStop;
        vRecv >> locator >> hashStop;
        if (locator.vHave.size() > MAX_LOCATOR_SZ) {
            LogPrint(BCLog::NET, "getheaders locator size %lld > %d, disconnect peer=%d\n", locator.vHave.size(), MAX_LOCATOR_SZ, pfrom.GetId());
            pfrom.fDisconnect = true;
            return;
        }
        if (fImporting || fReindex) {
            LogPrint(BCLog::NET, "Ignoring getheaders from peer=%d while importing/reindexing\n", pfrom.GetId());
            return;
        }
        LOCK(cs_main);
        if (m_chainman.ActiveTip() == nullptr ||
                (m_chainman.ActiveTip()->nChainWork < nMinimumChainWork && !pfrom.HasPermission(NetPermissionFlags::Download))) {
            LogPrint(BCLog::NET, "Ignoring getheaders from peer=%d because active chain has too little work; sending empty response\n", pfrom.GetId());
            m_connman.PushMessage(&pfrom, msgMaker.Make(NetMsgType::HEADERS, std::vector<CBlock>()));
            return;
        }
        CNodeState *nodestate = State(pfrom.GetId());
        const CBlockIndex* pindex = nullptr;
        if (locator.IsNull())
        {
            pindex = m_chainman.m_blockman.LookupBlockIndex(hashStop);
            if (!pindex) {
                return;
            }
            if (!BlockRequestAllowed(pindex)) {
                LogPrint(BCLog::NET, "%s: ignoring request from peer=%i for old block header that isn't in the main chain\n", __func__, pfrom.GetId());
                return;
            }
        }
        else
        {
            pindex = m_chainman.ActiveChainstate().FindForkInGlobalIndex(locator);
            if (pindex)
                pindex = m_chainman.ActiveChain().Next(pindex);
        }
        std::vector<CBlock> vHeaders;
        int nLimit = MAX_HEADERS_RESULTS;
        LogPrint(BCLog::NET, "getheaders %d to %s from peer=%d\n", (pindex ? pindex->nHeight : -1), hashStop.IsNull() ? "end" : hashStop.ToString(), pfrom.GetId());
        for (; pindex; pindex = m_chainman.ActiveChain().Next(pindex))
        {
            vHeaders.push_back(pindex->GetBlockHeader());
            if (--nLimit <= 0 || pindex->GetBlockHash() == hashStop)
                break;
        }
        nodestate->pindexBestHeaderSent = pindex ? pindex : m_chainman.ActiveChain().Tip();
        m_connman.PushMessage(&pfrom, msgMaker.Make(NetMsgType::HEADERS, vHeaders));
        return;
    }
    if (msg_type == NetMsgType::TX) {
        if (RejectIncomingTxs(pfrom)) {
            LogPrint(BCLog::NET, "transaction sent in violation of protocol peer=%d\n", pfrom.GetId());
            pfrom.fDisconnect = true;
            return;
        }
        if (m_chainman.ActiveChainstate().IsInitialBlockDownload()) return;
        CTransactionRef ptx;
        vRecv >> ptx;
        const CTransaction& tx = *ptx;
        const uint256& txid = ptx->GetHash();
        const uint256& wtxid = ptx->GetWitnessHash();
        const uint256& hash = peer->m_wtxid_relay ? wtxid : txid;
        AddKnownTx(*peer, hash);
        if (peer->m_wtxid_relay && txid != wtxid) {
            AddKnownTx(*peer, txid);
        }
        LOCK2(cs_main, g_cs_orphans);
        m_txrequest.ReceivedResponse(pfrom.GetId(), txid);
        if (tx.HasWitness()) m_txrequest.ReceivedResponse(pfrom.GetId(), wtxid);
        if (AlreadyHaveTx(GenTxid::Wtxid(wtxid))) {
            if (pfrom.HasPermission(NetPermissionFlags::ForceRelay)) {
                if (!m_mempool.exists(GenTxid::Txid(tx.GetHash()))) {
                    LogPrintf("Not relaying non-mempool transaction %s from forcerelay peer=%d\n", tx.GetHash().ToString(), pfrom.GetId());
                } else {
                    LogPrintf("Force relaying tx %s from peer=%d\n", tx.GetHash().ToString(), pfrom.GetId());
                    RelayTransaction(tx.GetHash(), tx.GetWitnessHash());
                }
            }
            return;
        }
        const MempoolAcceptResult result = m_chainman.ProcessTransaction(ptx);
        const TxValidationState& state = result.m_state;
        if (result.m_result_type == MempoolAcceptResult::ResultType::VALID) {
            m_txrequest.ForgetTxHash(tx.GetHash());
            m_txrequest.ForgetTxHash(tx.GetWitnessHash());
            RelayTransaction(tx.GetHash(), tx.GetWitnessHash());
            m_orphanage.AddChildrenToWorkSet(tx, peer->m_orphan_work_set);
            pfrom.m_last_tx_time = GetTime<std::chrono::seconds>();
            LogPrint(BCLog::MEMPOOL, "AcceptToMemoryPool: peer=%d: accepted %s (poolsz %u txn, %u kB)\n",
                pfrom.GetId(),
                tx.GetHash().ToString(),
                m_mempool.size(), m_mempool.DynamicMemoryUsage() / 1000);
            for (const CTransactionRef& removedTx : result.m_replaced_transactions.value()) {
                AddToCompactExtraTransactions(removedTx);
            }
            ProcessOrphanTx(peer->m_orphan_work_set);
        }
        else if (state.GetResult() == TxValidationResult::TX_MISSING_INPUTS)
        {
            bool fRejectedParents = false;
            std::vector<uint256> unique_parents;
            unique_parents.reserve(tx.vin.size());
            for (const CTxIn& txin : tx.vin) {
                unique_parents.push_back(txin.prevout.hash);
            }
            std::sort(unique_parents.begin(), unique_parents.end());
            unique_parents.erase(std::unique(unique_parents.begin(), unique_parents.end()), unique_parents.end());
            for (const uint256& parent_txid : unique_parents) {
                if (m_recent_rejects.contains(parent_txid)) {
                    fRejectedParents = true;
                    break;
                }
            }
            if (!fRejectedParents) {
                const auto current_time{GetTime<std::chrono::microseconds>()};
                for (const uint256& parent_txid : unique_parents) {
                    const auto gtxid{GenTxid::Txid(parent_txid)};
                    AddKnownTx(*peer, parent_txid);
                    if (!AlreadyHaveTx(gtxid)) AddTxAnnouncement(pfrom, gtxid, current_time);
                }
                if (m_orphanage.AddTx(ptx, pfrom.GetId())) {
                    AddToCompactExtraTransactions(ptx);
                }
                m_txrequest.ForgetTxHash(tx.GetHash());
                m_txrequest.ForgetTxHash(tx.GetWitnessHash());
                unsigned int nMaxOrphanTx = (unsigned int)std::max((int64_t)0, gArgs.GetIntArg("-maxorphantx", DEFAULT_MAX_ORPHAN_TRANSACTIONS));
                m_orphanage.LimitOrphans(nMaxOrphanTx);
            } else {
                LogPrint(BCLog::MEMPOOL, "not keeping orphan with rejected parents %s\n",tx.GetHash().ToString());
                m_recent_rejects.insert(tx.GetHash());
                m_recent_rejects.insert(tx.GetWitnessHash());
                m_txrequest.ForgetTxHash(tx.GetHash());
                m_txrequest.ForgetTxHash(tx.GetWitnessHash());
            }
        } else {
            if (state.GetResult() != TxValidationResult::TX_WITNESS_STRIPPED) {
                m_recent_rejects.insert(tx.GetWitnessHash());
                m_txrequest.ForgetTxHash(tx.GetWitnessHash());
                if (state.GetResult() == TxValidationResult::TX_INPUTS_NOT_STANDARD && tx.GetWitnessHash() != tx.GetHash()) {
                    m_recent_rejects.insert(tx.GetHash());
                    m_txrequest.ForgetTxHash(tx.GetHash());
                }
                if (RecursiveDynamicUsage(*ptx) < 100000) {
                    AddToCompactExtraTransactions(ptx);
                }
            }
        }
        if (state.IsInvalid()) {
            LogPrint(BCLog::MEMPOOLREJ, "%s from peer=%d was not accepted: %s\n", tx.GetHash().ToString(),
                pfrom.GetId(),
                state.ToString());
            MaybePunishNodeForTx(pfrom.GetId(), state);
        }
        return;
    }
    if (msg_type == NetMsgType::CMPCTBLOCK)
    {
        if (fImporting || fReindex) {
            LogPrint(BCLog::NET, "Unexpected cmpctblock message received from peer %d\n", pfrom.GetId());
            return;
        }
        CBlockHeaderAndShortTxIDs cmpctblock;
        vRecv >> cmpctblock;
        bool received_new_header = false;
        {
        LOCK(cs_main);
        const CBlockIndex* prev_block = m_chainman.m_blockman.LookupBlockIndex(cmpctblock.header.hashPrevBlock);
        if (!prev_block) {
            if (!m_chainman.ActiveChainstate().IsInitialBlockDownload()) {
                MaybeSendGetHeaders(pfrom, GetLocator(m_chainman.m_best_header), *peer);
            }
            return;
        } else if (prev_block->nChainWork + CalculateHeadersWork({cmpctblock.header}) < GetAntiDoSWorkThreshold()) {
            LogPrint(BCLog::NET, "Ignoring low-work compact block from peer %d\n", pfrom.GetId());
            return;
        }
        if (!m_chainman.m_blockman.LookupBlockIndex(cmpctblock.header.GetHash())) {
            received_new_header = true;
        }
        }
        const CBlockIndex *pindex = nullptr;
        BlockValidationState state;
        if (!m_chainman.ProcessNewBlockHeaders({cmpctblock.header},  true, state, &pindex)) {
            if (state.IsInvalid()) {
                MaybePunishNodeForBlock(pfrom.GetId(), state,  true, "invalid header via cmpctblock");
                return;
            }
        }
        bool fProcessBLOCKTXN = false;
        CDataStream blockTxnMsg(SER_NETWORK, PROTOCOL_VERSION);
        bool fRevertToHeaderProcessing = false;
        std::shared_ptr<CBlock> pblock = std::make_shared<CBlock>();
        bool fBlockReconstructed = false;
        {
        LOCK2(cs_main, g_cs_orphans);
        assert(pindex);
        UpdateBlockAvailability(pfrom.GetId(), pindex->GetBlockHash());
        CNodeState *nodestate = State(pfrom.GetId());
        if (received_new_header && pindex->nChainWork > m_chainman.ActiveChain().Tip()->nChainWork) {
            nodestate->m_last_block_announcement = GetTime();
        }
        std::map<uint256, std::pair<NodeId, std::list<QueuedBlock>::iterator> >::iterator blockInFlightIt = mapBlocksInFlight.find(pindex->GetBlockHash());
        bool fAlreadyInFlight = blockInFlightIt != mapBlocksInFlight.end();
        if (pindex->nStatus & BLOCK_HAVE_DATA)
            return;
        if (pindex->nChainWork <= m_chainman.ActiveChain().Tip()->nChainWork ||
                pindex->nTx != 0) {
            if (fAlreadyInFlight) {
                std::vector<CInv> vInv(1);
                vInv[0] = CInv(MSG_BLOCK | GetFetchFlags(*peer), cmpctblock.header.GetHash());
                m_connman.PushMessage(&pfrom, msgMaker.Make(NetMsgType::GETDATA, vInv));
            }
            return;
        }
        if (!fAlreadyInFlight && !CanDirectFetch()) {
            return;
        }
        if (pindex->nHeight <= m_chainman.ActiveChain().Height() + 2) {
            if ((!fAlreadyInFlight && nodestate->nBlocksInFlight < MAX_BLOCKS_IN_TRANSIT_PER_PEER) ||
                 (fAlreadyInFlight && blockInFlightIt->second.first == pfrom.GetId())) {
                std::list<QueuedBlock>::iterator* queuedBlockIt = nullptr;
                if (!BlockRequested(pfrom.GetId(), *pindex, &queuedBlockIt)) {
                    if (!(*queuedBlockIt)->partialBlock)
                        (*queuedBlockIt)->partialBlock.reset(new PartiallyDownloadedBlock(&m_mempool));
                    else {
                        LogPrint(BCLog::NET, "Peer sent us compact block we were already syncing!\n");
                        return;
                    }
                }
                PartiallyDownloadedBlock& partialBlock = *(*queuedBlockIt)->partialBlock;
                ReadStatus status = partialBlock.InitData(cmpctblock, vExtraTxnForCompact);
                if (status == READ_STATUS_INVALID) {
                    RemoveBlockRequest(pindex->GetBlockHash());
                    Misbehaving(*peer, 100, "invalid compact block");
                    return;
                } else if (status == READ_STATUS_FAILED) {
                    std::vector<CInv> vInv(1);
                    vInv[0] = CInv(MSG_BLOCK | GetFetchFlags(*peer), cmpctblock.header.GetHash());
                    m_connman.PushMessage(&pfrom, msgMaker.Make(NetMsgType::GETDATA, vInv));
                    return;
                }
                BlockTransactionsRequest req;
                for (size_t i = 0; i < cmpctblock.BlockTxCount(); i++) {
                    if (!partialBlock.IsTxAvailable(i))
                        req.indexes.push_back(i);
                }
                if (req.indexes.empty()) {
                    BlockTransactions txn;
                    txn.blockhash = cmpctblock.header.GetHash();
                    blockTxnMsg << txn;
                    fProcessBLOCKTXN = true;
                } else {
                    req.blockhash = pindex->GetBlockHash();
                    m_connman.PushMessage(&pfrom, msgMaker.Make(NetMsgType::GETBLOCKTXN, req));
                }
            } else {
                PartiallyDownloadedBlock tempBlock(&m_mempool);
                ReadStatus status = tempBlock.InitData(cmpctblock, vExtraTxnForCompact);
                if (status != READ_STATUS_OK) {
                    return;
                }
                std::vector<CTransactionRef> dummy;
                status = tempBlock.FillBlock(*pblock, dummy);
                if (status == READ_STATUS_OK) {
                    fBlockReconstructed = true;
                }
            }
        } else {
            if (fAlreadyInFlight) {
                std::vector<CInv> vInv(1);
                vInv[0] = CInv(MSG_BLOCK | GetFetchFlags(*peer), cmpctblock.header.GetHash());
                m_connman.PushMessage(&pfrom, msgMaker.Make(NetMsgType::GETDATA, vInv));
                return;
            } else {
                fRevertToHeaderProcessing = true;
            }
        }
        }
        if (fProcessBLOCKTXN) {
            return ProcessMessage(pfrom, NetMsgType::BLOCKTXN, blockTxnMsg, time_received, interruptMsgProc);
        }
        if (fRevertToHeaderProcessing) {
            return ProcessHeadersMessage(pfrom, *peer, {cmpctblock.header},  true);
        }
        if (fBlockReconstructed) {
            {
                LOCK(cs_main);
                mapBlockSource.emplace(pblock->GetHash(), std::make_pair(pfrom.GetId(), false));
            }
            ProcessBlock(pfrom, pblock,  true,  true);
            LOCK(cs_main);
            if (pindex->IsValid(BLOCK_VALID_TRANSACTIONS)) {
                RemoveBlockRequest(pblock->GetHash());
            }
        }
        return;
    }
    if (msg_type == NetMsgType::BLOCKTXN)
    {
        if (fImporting || fReindex) {
            LogPrint(BCLog::NET, "Unexpected blocktxn message received from peer %d\n", pfrom.GetId());
            return;
        }
        BlockTransactions resp;
        vRecv >> resp;
        std::shared_ptr<CBlock> pblock = std::make_shared<CBlock>();
        bool fBlockRead = false;
        {
            LOCK(cs_main);
            std::map<uint256, std::pair<NodeId, std::list<QueuedBlock>::iterator> >::iterator it = mapBlocksInFlight.find(resp.blockhash);
            if (it == mapBlocksInFlight.end() || !it->second.second->partialBlock ||
                    it->second.first != pfrom.GetId()) {
                LogPrint(BCLog::NET, "Peer %d sent us block transactions for block we weren't expecting\n", pfrom.GetId());
                return;
            }
            PartiallyDownloadedBlock& partialBlock = *it->second.second->partialBlock;
            ReadStatus status = partialBlock.FillBlock(*pblock, resp.txn);
            if (status == READ_STATUS_INVALID) {
                RemoveBlockRequest(resp.blockhash);
                Misbehaving(*peer, 100, "invalid compact block/non-matching block transactions");
                return;
            } else if (status == READ_STATUS_FAILED) {
                std::vector<CInv> invs;
                invs.push_back(CInv(MSG_BLOCK | GetFetchFlags(*peer), resp.blockhash));
                m_connman.PushMessage(&pfrom, msgMaker.Make(NetMsgType::GETDATA, invs));
            } else {
                RemoveBlockRequest(resp.blockhash);
                fBlockRead = true;
                mapBlockSource.emplace(resp.blockhash, std::make_pair(pfrom.GetId(), false));
            }
        }
        if (fBlockRead) {
            ProcessBlock(pfrom, pblock,  true,  true);
        }
        return;
    }
    if (msg_type == NetMsgType::HEADERS)
    {
        if (fImporting || fReindex) {
            LogPrint(BCLog::NET, "Unexpected headers message received from peer %d\n", pfrom.GetId());
            return;
        }
        peer->m_last_getheaders_timestamp = {};
        std::vector<CBlockHeader> headers;
        unsigned int nCount = ReadCompactSize(vRecv);
        if (nCount > MAX_HEADERS_RESULTS) {
            Misbehaving(*peer, 20, strprintf("headers message size = %u", nCount));
            return;
        }
        headers.resize(nCount);
        for (unsigned int n = 0; n < nCount; n++) {
            vRecv >> headers[n];
            ReadCompactSize(vRecv);
        }
        ProcessHeadersMessage(pfrom, *peer, std::move(headers),  false);
        if (m_headers_presync_should_signal.exchange(false)) {
            HeadersPresyncStats stats;
            {
                LOCK(m_headers_presync_mutex);
                auto it = m_headers_presync_stats.find(m_headers_presync_bestpeer);
                if (it != m_headers_presync_stats.end()) stats = it->second;
            }
            if (stats.second) {
                m_chainman.ReportHeadersPresync(stats.first, stats.second->first, stats.second->second);
            }
        }
        return;
    }
    if (msg_type == NetMsgType::BLOCK)
    {
        if (fImporting || fReindex) {
            LogPrint(BCLog::NET, "Unexpected block message received from peer %d\n", pfrom.GetId());
            return;
        }
        std::shared_ptr<CBlock> pblock = std::make_shared<CBlock>();
        vRecv >> *pblock;
        LogPrint(BCLog::NET, "received block %s peer=%d\n", pblock->GetHash().ToString(), pfrom.GetId());
        bool forceProcessing = false;
        const uint256 hash(pblock->GetHash());
        bool min_pow_checked = false;
        {
            LOCK(cs_main);
            forceProcessing = IsBlockRequested(hash);
            RemoveBlockRequest(hash);
            mapBlockSource.emplace(hash, std::make_pair(pfrom.GetId(), true));
            const CBlockIndex* prev_block = m_chainman.m_blockman.LookupBlockIndex(pblock->hashPrevBlock);
            if (prev_block && prev_block->nChainWork + CalculateHeadersWork({pblock->GetBlockHeader()}) >= GetAntiDoSWorkThreshold()) {
                min_pow_checked = true;
            }
        }
        ProcessBlock(pfrom, pblock, forceProcessing, min_pow_checked);
        return;
    }
    if (msg_type == NetMsgType::GETADDR) {
        if (!pfrom.IsInboundConn()) {
            LogPrint(BCLog::NET, "Ignoring \"getaddr\" from %s connection. peer=%d\n", pfrom.ConnectionTypeAsString(), pfrom.GetId());
            return;
        }
        Assume(SetupAddressRelay(pfrom, *peer));
        if (peer->m_getaddr_recvd) {
            LogPrint(BCLog::NET, "Ignoring repeated \"getaddr\". peer=%d\n", pfrom.GetId());
            return;
        }
        peer->m_getaddr_recvd = true;
        peer->m_addrs_to_send.clear();
        std::vector<CAddress> vAddr;
        if (pfrom.HasPermission(NetPermissionFlags::Addr)) {
            vAddr = m_connman.GetAddresses(MAX_ADDR_TO_SEND, MAX_PCT_ADDR_TO_SEND,  std::nullopt);
        } else {
            vAddr = m_connman.GetAddresses(pfrom, MAX_ADDR_TO_SEND, MAX_PCT_ADDR_TO_SEND);
        }
        FastRandomContext insecure_rand;
        for (const CAddress &addr : vAddr) {
            PushAddress(*peer, addr, insecure_rand);
        }
        return;
    }
    if (msg_type == NetMsgType::MEMPOOL) {
        if (!(peer->m_our_services & NODE_BLOOM) && !pfrom.HasPermission(NetPermissionFlags::Mempool))
        {
            if (!pfrom.HasPermission(NetPermissionFlags::NoBan))
            {
                LogPrint(BCLog::NET, "mempool request with bloom filters disabled, disconnect peer=%d\n", pfrom.GetId());
                pfrom.fDisconnect = true;
            }
            return;
        }
        if (m_connman.OutboundTargetReached(false) && !pfrom.HasPermission(NetPermissionFlags::Mempool))
        {
            if (!pfrom.HasPermission(NetPermissionFlags::NoBan))
            {
                LogPrint(BCLog::NET, "mempool request with bandwidth limit reached, disconnect peer=%d\n", pfrom.GetId());
                pfrom.fDisconnect = true;
            }
            return;
        }
        if (auto tx_relay = peer->GetTxRelay(); tx_relay != nullptr) {
            LOCK(tx_relay->m_tx_inventory_mutex);
            tx_relay->m_send_mempool = true;
        }
        return;
    }
    if (msg_type == NetMsgType::PING) {
        if (pfrom.GetCommonVersion() > BIP0031_VERSION) {
            uint64_t nonce = 0;
            vRecv >> nonce;
            m_connman.PushMessage(&pfrom, msgMaker.Make(NetMsgType::PONG, nonce));
        }
        return;
    }
    if (msg_type == NetMsgType::PONG) {
        const auto ping_end = time_received;
        uint64_t nonce = 0;
        size_t nAvail = vRecv.in_avail();
        bool bPingFinished = false;
        std::string sProblem;
        if (nAvail >= sizeof(nonce)) {
            vRecv >> nonce;
            if (peer->m_ping_nonce_sent != 0) {
                if (nonce == peer->m_ping_nonce_sent) {
                    bPingFinished = true;
                    const auto ping_time = ping_end - peer->m_ping_start.load();
                    if (ping_time.count() >= 0) {
                        pfrom.PongReceived(ping_time);
                    } else {
                        sProblem = "Timing mishap";
                    }
                } else {
                    sProblem = "Nonce mismatch";
                    if (nonce == 0) {
                        bPingFinished = true;
                        sProblem = "Nonce zero";
                    }
                }
            } else {
                sProblem = "Unsolicited pong without ping";
            }
        } else {
            bPingFinished = true;
            sProblem = "Short payload";
        }
        if (!(sProblem.empty())) {
            LogPrint(BCLog::NET, "pong peer=%d: %s, %x expected, %x received, %u bytes\n",
                pfrom.GetId(),
                sProblem,
                peer->m_ping_nonce_sent,
                nonce,
                nAvail);
        }
        if (bPingFinished) {
            peer->m_ping_nonce_sent = 0;
        }
        return;
    }
    if (msg_type == NetMsgType::FILTERLOAD) {
        if (!(peer->m_our_services & NODE_BLOOM)) {
            LogPrint(BCLog::NET, "filterload received despite not offering bloom services from peer=%d; disconnecting\n", pfrom.GetId());
            pfrom.fDisconnect = true;
            return;
        }
        CBloomFilter filter;
        vRecv >> filter;
        if (!filter.IsWithinSizeConstraints())
        {
            Misbehaving(*peer, 100, "too-large bloom filter");
        } else if (auto tx_relay = peer->GetTxRelay(); tx_relay != nullptr) {
            {
                LOCK(tx_relay->m_bloom_filter_mutex);
                tx_relay->m_bloom_filter.reset(new CBloomFilter(filter));
                tx_relay->m_relay_txs = true;
            }
            pfrom.m_bloom_filter_loaded = true;
            pfrom.m_relays_txs = true;
        }
        return;
    }
    if (msg_type == NetMsgType::FILTERADD) {
        if (!(peer->m_our_services & NODE_BLOOM)) {
            LogPrint(BCLog::NET, "filteradd received despite not offering bloom services from peer=%d; disconnecting\n", pfrom.GetId());
            pfrom.fDisconnect = true;
            return;
        }
        std::vector<unsigned char> vData;
        vRecv >> vData;
        bool bad = false;
        if (vData.size() > MAX_SCRIPT_ELEMENT_SIZE) {
            bad = true;
        } else if (auto tx_relay = peer->GetTxRelay(); tx_relay != nullptr) {
            LOCK(tx_relay->m_bloom_filter_mutex);
            if (tx_relay->m_bloom_filter) {
                tx_relay->m_bloom_filter->insert(vData);
            } else {
                bad = true;
            }
        }
        if (bad) {
            Misbehaving(*peer, 100, "bad filteradd message");
        }
        return;
    }
    if (msg_type == NetMsgType::FILTERCLEAR) {
        if (!(peer->m_our_services & NODE_BLOOM)) {
            LogPrint(BCLog::NET, "filterclear received despite not offering bloom services from peer=%d; disconnecting\n", pfrom.GetId());
            pfrom.fDisconnect = true;
            return;
        }
        auto tx_relay = peer->GetTxRelay();
        if (!tx_relay) return;
        {
            LOCK(tx_relay->m_bloom_filter_mutex);
            tx_relay->m_bloom_filter = nullptr;
            tx_relay->m_relay_txs = true;
        }
        pfrom.m_bloom_filter_loaded = false;
        pfrom.m_relays_txs = true;
        return;
    }
    if (msg_type == NetMsgType::FEEFILTER) {
        CAmount newFeeFilter = 0;
        vRecv >> newFeeFilter;
        if (MoneyRange(newFeeFilter)) {
            if (auto tx_relay = peer->GetTxRelay(); tx_relay != nullptr) {
                tx_relay->m_fee_filter_received = newFeeFilter;
            }
            LogPrint(BCLog::NET, "received: feefilter of %s from peer=%d\n", CFeeRate(newFeeFilter).ToString(), pfrom.GetId());
        }
        return;
    }
    if (msg_type == NetMsgType::GETCFILTERS) {
        ProcessGetCFilters(pfrom, *peer, vRecv);
        return;
    }
    if (msg_type == NetMsgType::GETCFHEADERS) {
        ProcessGetCFHeaders(pfrom, *peer, vRecv);
        return;
    }
    if (msg_type == NetMsgType::GETCFCHECKPT) {
        ProcessGetCFCheckPt(pfrom, *peer, vRecv);
        return;
    }
    if (msg_type == NetMsgType::NOTFOUND) {
        std::vector<CInv> vInv;
        vRecv >> vInv;
        if (vInv.size() <= MAX_PEER_TX_ANNOUNCEMENTS + MAX_BLOCKS_IN_TRANSIT_PER_PEER) {
            LOCK(::cs_main);
            for (CInv &inv : vInv) {
                if (inv.IsGenTxMsg()) {
                    m_txrequest.ReceivedResponse(pfrom.GetId(), inv.hash);
                }
            }
        }
        return;
    }
    LogPrint(BCLog::NET, "Unknown command \"%s\" from peer=%d\n", SanitizeString(msg_type), pfrom.GetId());
    return;
}
bool PeerManagerImpl::MaybeDiscourageAndDisconnect(CNode& pnode, Peer& peer)
{
    {
        LOCK(peer.m_misbehavior_mutex);
        if (!peer.m_should_discourage) return false;
        peer.m_should_discourage = false;
    }
    if (pnode.HasPermission(NetPermissionFlags::NoBan)) {
        LogPrintf("Warning: not punishing noban peer %d!\n", peer.m_id);
        return false;
    }
    if (pnode.IsManualConn()) {
        LogPrintf("Warning: not punishing manually connected peer %d!\n", peer.m_id);
        return false;
    }
    if (pnode.addr.IsLocal()) {
        LogPrint(BCLog::NET, "Warning: disconnecting but not discouraging %s peer %d!\n",
                 pnode.m_inbound_onion ? "inbound onion" : "local", peer.m_id);
        pnode.fDisconnect = true;
        return true;
    }
    LogPrint(BCLog::NET, "Disconnecting and discouraging peer %d!\n", peer.m_id);
    if (m_banman) m_banman->Discourage(pnode.addr);
    m_connman.DisconnectNode(pnode.addr);
    return true;
}
bool PeerManagerImpl::ProcessMessages(CNode* pfrom, std::atomic<bool>& interruptMsgProc)
{
    AssertLockHeld(g_msgproc_mutex);
    bool fMoreWork = false;
    PeerRef peer = GetPeerRef(pfrom->GetId());
    if (peer == nullptr) return false;
    {
        LOCK(peer->m_getdata_requests_mutex);
        if (!peer->m_getdata_requests.empty()) {
            ProcessGetData(*pfrom, *peer, interruptMsgProc);
        }
    }
    {
        LOCK2(cs_main, g_cs_orphans);
        if (!peer->m_orphan_work_set.empty()) {
            ProcessOrphanTx(peer->m_orphan_work_set);
        }
    }
    if (pfrom->fDisconnect)
        return false;
    {
        LOCK(peer->m_getdata_requests_mutex);
        if (!peer->m_getdata_requests.empty()) return true;
    }
    {
        LOCK(g_cs_orphans);
        if (!peer->m_orphan_work_set.empty()) return true;
    }
    if (pfrom->fPauseSend) return false;
    std::list<CNetMessage> msgs;
    {
        LOCK(pfrom->cs_vProcessMsg);
        if (pfrom->vProcessMsg.empty()) return false;
        msgs.splice(msgs.begin(), pfrom->vProcessMsg, pfrom->vProcessMsg.begin());
        pfrom->nProcessQueueSize -= msgs.front().m_raw_message_size;
        pfrom->fPauseRecv = pfrom->nProcessQueueSize > m_connman.GetReceiveFloodSize();
        fMoreWork = !pfrom->vProcessMsg.empty();
    }
    CNetMessage& msg(msgs.front());
    TRACE6(net, inbound_message,
        pfrom->GetId(),
        pfrom->m_addr_name.c_str(),
        pfrom->ConnectionTypeAsString().c_str(),
        msg.m_type.c_str(),
        msg.m_recv.size(),
        msg.m_recv.data()
    );
    if (gArgs.GetBoolArg("-capturemessages", false)) {
        CaptureMessage(pfrom->addr, msg.m_type, MakeUCharSpan(msg.m_recv),  true);
    }
    msg.SetVersion(pfrom->GetCommonVersion());
    try {
        ProcessMessage(*pfrom, msg.m_type, msg.m_recv, msg.m_time, interruptMsgProc);
        if (interruptMsgProc) return false;
        {
            LOCK(peer->m_getdata_requests_mutex);
            if (!peer->m_getdata_requests.empty()) fMoreWork = true;
        }
    } catch (const std::exception& e) {
        LogPrint(BCLog::NET, "%s(%s, %u bytes): Exception '%s' (%s) caught\n", __func__, SanitizeString(msg.m_type), msg.m_message_size, e.what(), typeid(e).name());
    } catch (...) {
        LogPrint(BCLog::NET, "%s(%s, %u bytes): Unknown exception caught\n", __func__, SanitizeString(msg.m_type), msg.m_message_size);
    }
    return fMoreWork;
}
void PeerManagerImpl::ConsiderEviction(CNode& pto, Peer& peer, std::chrono::seconds time_in_seconds)
{
    AssertLockHeld(cs_main);
    CNodeState &state = *State(pto.GetId());
    const CNetMsgMaker msgMaker(pto.GetCommonVersion());
    if (!state.m_chain_sync.m_protect && pto.IsOutboundOrBlockRelayConn() && state.fSyncStarted) {
        if (state.pindexBestKnownBlock != nullptr && state.pindexBestKnownBlock->nChainWork >= m_chainman.ActiveChain().Tip()->nChainWork) {
            if (state.m_chain_sync.m_timeout != 0s) {
                state.m_chain_sync.m_timeout = 0s;
                state.m_chain_sync.m_work_header = nullptr;
                state.m_chain_sync.m_sent_getheaders = false;
            }
        } else if (state.m_chain_sync.m_timeout == 0s || (state.m_chain_sync.m_work_header != nullptr && state.pindexBestKnownBlock != nullptr && state.pindexBestKnownBlock->nChainWork >= state.m_chain_sync.m_work_header->nChainWork)) {
            state.m_chain_sync.m_timeout = time_in_seconds + CHAIN_SYNC_TIMEOUT;
            state.m_chain_sync.m_work_header = m_chainman.ActiveChain().Tip();
            state.m_chain_sync.m_sent_getheaders = false;
        } else if (state.m_chain_sync.m_timeout > 0s && time_in_seconds > state.m_chain_sync.m_timeout) {
            if (state.m_chain_sync.m_sent_getheaders) {
                LogPrintf("Disconnecting outbound peer %d for old chain, best known block = %s\n", pto.GetId(), state.pindexBestKnownBlock != nullptr ? state.pindexBestKnownBlock->GetBlockHash().ToString() : "<none>");
                pto.fDisconnect = true;
            } else {
                assert(state.m_chain_sync.m_work_header);
                MaybeSendGetHeaders(pto,
                        GetLocator(state.m_chain_sync.m_work_header->pprev),
                        peer);
                LogPrint(BCLog::NET, "sending getheaders to outbound peer=%d to verify chain work (current best known block:%s, benchmark blockhash: %s)\n", pto.GetId(), state.pindexBestKnownBlock != nullptr ? state.pindexBestKnownBlock->GetBlockHash().ToString() : "<none>", state.m_chain_sync.m_work_header->GetBlockHash().ToString());
                state.m_chain_sync.m_sent_getheaders = true;
                state.m_chain_sync.m_timeout = time_in_seconds + HEADERS_RESPONSE_TIME;
            }
        }
    }
}
void PeerManagerImpl::EvictExtraOutboundPeers(std::chrono::seconds now)
{
    if (m_connman.GetExtraBlockRelayCount() > 0) {
        std::pair<NodeId, std::chrono::seconds> youngest_peer{-1, 0}, next_youngest_peer{-1, 0};
        m_connman.ForEachNode([&](CNode* pnode) {
            if (!pnode->IsBlockOnlyConn() || pnode->fDisconnect) return;
            if (pnode->GetId() > youngest_peer.first) {
                next_youngest_peer = youngest_peer;
                youngest_peer.first = pnode->GetId();
                youngest_peer.second = pnode->m_last_block_time;
            }
        });
        NodeId to_disconnect = youngest_peer.first;
        if (youngest_peer.second > next_youngest_peer.second) {
            to_disconnect = next_youngest_peer.first;
        }
        m_connman.ForNode(to_disconnect, [&](CNode* pnode) EXCLUSIVE_LOCKS_REQUIRED(::cs_main) {
            AssertLockHeld(::cs_main);
            CNodeState *node_state = State(pnode->GetId());
            if (node_state == nullptr ||
                (now - pnode->m_connected >= MINIMUM_CONNECT_TIME && node_state->nBlocksInFlight == 0)) {
                pnode->fDisconnect = true;
                LogPrint(BCLog::NET, "disconnecting extra block-relay-only peer=%d (last block received at time %d)\n",
                         pnode->GetId(), count_seconds(pnode->m_last_block_time));
                return true;
            } else {
                LogPrint(BCLog::NET, "keeping block-relay-only peer=%d chosen for eviction (connect time: %d, blocks_in_flight: %d)\n",
                         pnode->GetId(), count_seconds(pnode->m_connected), node_state->nBlocksInFlight);
            }
            return false;
        });
    }
    if (m_connman.GetExtraFullOutboundCount() > 0) {
        NodeId worst_peer = -1;
        int64_t oldest_block_announcement = std::numeric_limits<int64_t>::max();
        m_connman.ForEachNode([&](CNode* pnode) EXCLUSIVE_LOCKS_REQUIRED(::cs_main) {
            AssertLockHeld(::cs_main);
            if (!pnode->IsFullOutboundConn() || pnode->fDisconnect) return;
            CNodeState *state = State(pnode->GetId());
            if (state == nullptr) return;
            if (state->m_chain_sync.m_protect) return;
            if (state->m_last_block_announcement < oldest_block_announcement || (state->m_last_block_announcement == oldest_block_announcement && pnode->GetId() > worst_peer)) {
                worst_peer = pnode->GetId();
                oldest_block_announcement = state->m_last_block_announcement;
            }
        });
        if (worst_peer != -1) {
            bool disconnected = m_connman.ForNode(worst_peer, [&](CNode* pnode) EXCLUSIVE_LOCKS_REQUIRED(::cs_main) {
                AssertLockHeld(::cs_main);
                CNodeState &state = *State(pnode->GetId());
                if (now - pnode->m_connected > MINIMUM_CONNECT_TIME && state.nBlocksInFlight == 0) {
                    LogPrint(BCLog::NET, "disconnecting extra outbound peer=%d (last block announcement received at time %d)\n", pnode->GetId(), oldest_block_announcement);
                    pnode->fDisconnect = true;
                    return true;
                } else {
                    LogPrint(BCLog::NET, "keeping outbound peer=%d chosen for eviction (connect time: %d, blocks_in_flight: %d)\n",
                             pnode->GetId(), count_seconds(pnode->m_connected), state.nBlocksInFlight);
                    return false;
                }
            });
            if (disconnected) {
                m_connman.SetTryNewOutboundPeer(false);
            }
        }
    }
}
void PeerManagerImpl::CheckForStaleTipAndEvictPeers()
{
    LOCK(cs_main);
    auto now{GetTime<std::chrono::seconds>()};
    EvictExtraOutboundPeers(now);
    if (now > m_stale_tip_check_time) {
        if (!fImporting && !fReindex && m_connman.GetNetworkActive() && m_connman.GetUseAddrmanOutgoing() && TipMayBeStale()) {
            LogPrintf("Potential stale tip detected, will try using extra outbound peer (last tip update: %d seconds ago)\n",
                      count_seconds(now - m_last_tip_update.load()));
            m_connman.SetTryNewOutboundPeer(true);
        } else if (m_connman.GetTryNewOutboundPeer()) {
            m_connman.SetTryNewOutboundPeer(false);
        }
        m_stale_tip_check_time = now + STALE_CHECK_INTERVAL;
    }
    if (!m_initial_sync_finished && CanDirectFetch()) {
        m_connman.StartExtraBlockRelayPeers();
        m_initial_sync_finished = true;
    }
}
void PeerManagerImpl::MaybeSendPing(CNode& node_to, Peer& peer, std::chrono::microseconds now)
{
    if (m_connman.ShouldRunInactivityChecks(node_to, std::chrono::duration_cast<std::chrono::seconds>(now)) &&
        peer.m_ping_nonce_sent &&
        now > peer.m_ping_start.load() + TIMEOUT_INTERVAL)
    {
        LogPrint(BCLog::NET, "ping timeout: %fs peer=%d\n", 0.000001 * count_microseconds(now - peer.m_ping_start.load()), peer.m_id);
        node_to.fDisconnect = true;
        return;
    }
    const CNetMsgMaker msgMaker(node_to.GetCommonVersion());
    bool pingSend = false;
    if (peer.m_ping_queued) {
        pingSend = true;
    }
    if (peer.m_ping_nonce_sent == 0 && now > peer.m_ping_start.load() + PING_INTERVAL) {
        pingSend = true;
    }
    if (pingSend) {
        uint64_t nonce;
        do {
            nonce = GetRand<uint64_t>();
        } while (nonce == 0);
        peer.m_ping_queued = false;
        peer.m_ping_start = now;
        if (node_to.GetCommonVersion() > BIP0031_VERSION) {
            peer.m_ping_nonce_sent = nonce;
            m_connman.PushMessage(&node_to, msgMaker.Make(NetMsgType::PING, nonce));
        } else {
            peer.m_ping_nonce_sent = 0;
            m_connman.PushMessage(&node_to, msgMaker.Make(NetMsgType::PING));
        }
    }
}
void PeerManagerImpl::MaybeSendAddr(CNode& node, Peer& peer, std::chrono::microseconds current_time)
{
    if (!peer.m_addr_relay_enabled) return;
    LOCK(peer.m_addr_send_times_mutex);
    if (fListen && !m_chainman.ActiveChainstate().IsInitialBlockDownload() &&
        peer.m_next_local_addr_send < current_time) {
        if (peer.m_next_local_addr_send != 0us) {
            peer.m_addr_known->reset();
        }
        if (std::optional<CService> local_service = GetLocalAddrForPeer(node)) {
            CAddress local_addr{*local_service, peer.m_our_services, Now<NodeSeconds>()};
            FastRandomContext insecure_rand;
            PushAddress(peer, local_addr, insecure_rand);
        }
        peer.m_next_local_addr_send = GetExponentialRand(current_time, AVG_LOCAL_ADDRESS_BROADCAST_INTERVAL);
    }
    if (current_time <= peer.m_next_addr_send) return;
    peer.m_next_addr_send = GetExponentialRand(current_time, AVG_ADDRESS_BROADCAST_INTERVAL);
    if (!Assume(peer.m_addrs_to_send.size() <= MAX_ADDR_TO_SEND)) {
        peer.m_addrs_to_send.resize(MAX_ADDR_TO_SEND);
    }
    auto addr_already_known = [&peer](const CAddress& addr) EXCLUSIVE_LOCKS_REQUIRED(g_msgproc_mutex) {
        bool ret = peer.m_addr_known->contains(addr.GetKey());
        if (!ret) peer.m_addr_known->insert(addr.GetKey());
        return ret;
    };
    peer.m_addrs_to_send.erase(std::remove_if(peer.m_addrs_to_send.begin(), peer.m_addrs_to_send.end(), addr_already_known),
                           peer.m_addrs_to_send.end());
    if (peer.m_addrs_to_send.empty()) return;
    const char* msg_type;
    int make_flags;
    if (peer.m_wants_addrv2) {
        msg_type = NetMsgType::ADDRV2;
        make_flags = ADDRV2_FORMAT;
    } else {
        msg_type = NetMsgType::ADDR;
        make_flags = 0;
    }
    m_connman.PushMessage(&node, CNetMsgMaker(node.GetCommonVersion()).Make(make_flags, msg_type, peer.m_addrs_to_send));
    peer.m_addrs_to_send.clear();
    if (peer.m_addrs_to_send.capacity() > 40) {
        peer.m_addrs_to_send.shrink_to_fit();
    }
}
void PeerManagerImpl::MaybeSendSendHeaders(CNode& node, Peer& peer)
{
    if (!peer.m_sent_sendheaders && node.GetCommonVersion() >= SENDHEADERS_VERSION) {
        LOCK(cs_main);
        CNodeState &state = *State(node.GetId());
        if (state.pindexBestKnownBlock != nullptr &&
                state.pindexBestKnownBlock->nChainWork > nMinimumChainWork) {
            m_connman.PushMessage(&node, CNetMsgMaker(node.GetCommonVersion()).Make(NetMsgType::SENDHEADERS));
            peer.m_sent_sendheaders = true;
        }
    }
}
void PeerManagerImpl::MaybeSendFeefilter(CNode& pto, Peer& peer, std::chrono::microseconds current_time)
{
    if (m_ignore_incoming_txs) return;
    if (pto.GetCommonVersion() < FEEFILTER_VERSION) return;
    if (pto.HasPermission(NetPermissionFlags::ForceRelay)) return;
    if (pto.IsBlockOnlyConn()) return;
    CAmount currentFilter = m_mempool.GetMinFee().GetFeePerK();
    static FeeFilterRounder g_filter_rounder{CFeeRate{DEFAULT_MIN_RELAY_TX_FEE}};
    if (m_chainman.ActiveChainstate().IsInitialBlockDownload()) {
        currentFilter = MAX_MONEY;
    } else {
        static const CAmount MAX_FILTER{g_filter_rounder.round(MAX_MONEY)};
        if (peer.m_fee_filter_sent == MAX_FILTER) {
            peer.m_next_send_feefilter = 0us;
        }
    }
    if (current_time > peer.m_next_send_feefilter) {
        CAmount filterToSend = g_filter_rounder.round(currentFilter);
        filterToSend = std::max(filterToSend, m_mempool.m_min_relay_feerate.GetFeePerK());
        if (filterToSend != peer.m_fee_filter_sent) {
            m_connman.PushMessage(&pto, CNetMsgMaker(pto.GetCommonVersion()).Make(NetMsgType::FEEFILTER, filterToSend));
            peer.m_fee_filter_sent = filterToSend;
        }
        peer.m_next_send_feefilter = GetExponentialRand(current_time, AVG_FEEFILTER_BROADCAST_INTERVAL);
    }
    else if (current_time + MAX_FEEFILTER_CHANGE_DELAY < peer.m_next_send_feefilter &&
                (currentFilter < 3 * peer.m_fee_filter_sent / 4 || currentFilter > 4 * peer.m_fee_filter_sent / 3)) {
        peer.m_next_send_feefilter = current_time + GetRandomDuration<std::chrono::microseconds>(MAX_FEEFILTER_CHANGE_DELAY);
    }
}
namespace {
class CompareInvMempoolOrder
{
    CTxMemPool* mp;
    bool m_wtxid_relay;
public:
    explicit CompareInvMempoolOrder(CTxMemPool *_mempool, bool use_wtxid)
    {
        mp = _mempool;
        m_wtxid_relay = use_wtxid;
    }
    bool operator()(std::set<uint256>::iterator a, std::set<uint256>::iterator b)
    {
        return mp->CompareDepthAndScore(*b, *a, m_wtxid_relay);
    }
};
}
bool PeerManagerImpl::RejectIncomingTxs(const CNode& peer) const
{
    if (peer.IsBlockOnlyConn()) return true;
    if (m_ignore_incoming_txs && !peer.HasPermission(NetPermissionFlags::Relay)) return true;
    return false;
}
bool PeerManagerImpl::SetupAddressRelay(const CNode& node, Peer& peer)
{
    if (node.IsBlockOnlyConn()) return false;
    if (!peer.m_addr_relay_enabled.exchange(true)) {
        peer.m_addr_known = std::make_unique<CRollingBloomFilter>(5000, 0.001);
    }
    return true;
}
bool PeerManagerImpl::SendMessages(CNode* pto)
{
    AssertLockHeld(g_msgproc_mutex);
    PeerRef peer = GetPeerRef(pto->GetId());
    if (!peer) return false;
    const Consensus::Params& consensusParams = m_chainparams.GetConsensus();
    if (MaybeDiscourageAndDisconnect(*pto, *peer)) return true;
    if (!pto->fSuccessfullyConnected || pto->fDisconnect)
        return true;
    const CNetMsgMaker msgMaker(pto->GetCommonVersion());
    const auto current_time{GetTime<std::chrono::microseconds>()};
    if (pto->IsAddrFetchConn() && current_time - pto->m_connected > 10 * AVG_ADDRESS_BROADCAST_INTERVAL) {
        LogPrint(BCLog::NET, "addrfetch connection timeout; disconnecting peer=%d\n", pto->GetId());
        pto->fDisconnect = true;
        return true;
    }
    MaybeSendPing(*pto, *peer, current_time);
    if (pto->fDisconnect) return true;
    MaybeSendAddr(*pto, *peer, current_time);
    MaybeSendSendHeaders(*pto, *peer);
    {
        LOCK(cs_main);
        CNodeState &state = *State(pto->GetId());
        if (m_chainman.m_best_header == nullptr) {
            m_chainman.m_best_header = m_chainman.ActiveChain().Tip();
        }
        bool sync_blocks_and_headers_from_peer = false;
        if (state.fPreferredDownload) {
            sync_blocks_and_headers_from_peer = true;
        } else if (CanServeBlocks(*peer) && !pto->IsAddrFetchConn()) {
            if (m_num_preferred_download_peers == 0 || mapBlocksInFlight.empty()) {
                sync_blocks_and_headers_from_peer = true;
            }
        }
        if (!state.fSyncStarted && CanServeBlocks(*peer) && !fImporting && !fReindex) {
            if ((nSyncStarted == 0 && sync_blocks_and_headers_from_peer) || m_chainman.m_best_header->Time() > GetAdjustedTime() - 24h) {
                const CBlockIndex* pindexStart = m_chainman.m_best_header;
                if (pindexStart->pprev)
                    pindexStart = pindexStart->pprev;
                if (MaybeSendGetHeaders(*pto, GetLocator(pindexStart), *peer)) {
                    LogPrint(BCLog::NET, "initial getheaders (%d) to peer=%d (startheight:%d)\n", pindexStart->nHeight, pto->GetId(), peer->m_starting_height);
                    state.fSyncStarted = true;
                    state.m_headers_sync_timeout = current_time + HEADERS_DOWNLOAD_TIMEOUT_BASE +
                        (
                         std::chrono::microseconds{HEADERS_DOWNLOAD_TIMEOUT_PER_HEADER} *
                         Ticks<std::chrono::seconds>(GetAdjustedTime() - m_chainman.m_best_header->Time()) / consensusParams.nPowTargetSpacing
                        );
                    nSyncStarted++;
                }
            }
        }
        {
            LOCK(peer->m_block_inv_mutex);
            std::vector<CBlock> vHeaders;
            bool fRevertToInv = ((!state.fPreferHeaders &&
                                 (!state.m_requested_hb_cmpctblocks || peer->m_blocks_for_headers_relay.size() > 1)) ||
                                 peer->m_blocks_for_headers_relay.size() > MAX_BLOCKS_TO_ANNOUNCE);
            const CBlockIndex *pBestIndex = nullptr;
            ProcessBlockAvailability(pto->GetId());
            if (!fRevertToInv) {
                bool fFoundStartingHeader = false;
                for (const uint256& hash : peer->m_blocks_for_headers_relay) {
                    const CBlockIndex* pindex = m_chainman.m_blockman.LookupBlockIndex(hash);
                    assert(pindex);
                    if (m_chainman.ActiveChain()[pindex->nHeight] != pindex) {
                        fRevertToInv = true;
                        break;
                    }
                    if (pBestIndex != nullptr && pindex->pprev != pBestIndex) {
                        fRevertToInv = true;
                        break;
                    }
                    pBestIndex = pindex;
                    if (fFoundStartingHeader) {
                        vHeaders.push_back(pindex->GetBlockHeader());
                    } else if (PeerHasHeader(&state, pindex)) {
                        continue;
                    } else if (pindex->pprev == nullptr || PeerHasHeader(&state, pindex->pprev)) {
                        fFoundStartingHeader = true;
                        vHeaders.push_back(pindex->GetBlockHeader());
                    } else {
                        fRevertToInv = true;
                        break;
                    }
                }
            }
            if (!fRevertToInv && !vHeaders.empty()) {
                if (vHeaders.size() == 1 && state.m_requested_hb_cmpctblocks) {
                    LogPrint(BCLog::NET, "%s sending header-and-ids %s to peer=%d\n", __func__,
                            vHeaders.front().GetHash().ToString(), pto->GetId());
                    std::optional<CSerializedNetMsg> cached_cmpctblock_msg;
                    {
                        LOCK(m_most_recent_block_mutex);
                        if (m_most_recent_block_hash == pBestIndex->GetBlockHash()) {
                            cached_cmpctblock_msg = msgMaker.Make(NetMsgType::CMPCTBLOCK, *m_most_recent_compact_block);
                        }
                    }
                    if (cached_cmpctblock_msg.has_value()) {
                        m_connman.PushMessage(pto, std::move(cached_cmpctblock_msg.value()));
                    } else {
                        CBlock block;
                        bool ret = ReadBlockFromDisk(block, pBestIndex, consensusParams);
                        assert(ret);
                        CBlockHeaderAndShortTxIDs cmpctblock{block};
                        m_connman.PushMessage(pto, msgMaker.Make(NetMsgType::CMPCTBLOCK, cmpctblock));
                    }
                    state.pindexBestHeaderSent = pBestIndex;
                } else if (state.fPreferHeaders) {
                    if (vHeaders.size() > 1) {
                        LogPrint(BCLog::NET, "%s: %u headers, range (%s, %s), to peer=%d\n", __func__,
                                vHeaders.size(),
                                vHeaders.front().GetHash().ToString(),
                                vHeaders.back().GetHash().ToString(), pto->GetId());
                    } else {
                        LogPrint(BCLog::NET, "%s: sending header %s to peer=%d\n", __func__,
                                vHeaders.front().GetHash().ToString(), pto->GetId());
                    }
                    m_connman.PushMessage(pto, msgMaker.Make(NetMsgType::HEADERS, vHeaders));
                    state.pindexBestHeaderSent = pBestIndex;
                } else
                    fRevertToInv = true;
            }
            if (fRevertToInv) {
                if (!peer->m_blocks_for_headers_relay.empty()) {
                    const uint256& hashToAnnounce = peer->m_blocks_for_headers_relay.back();
                    const CBlockIndex* pindex = m_chainman.m_blockman.LookupBlockIndex(hashToAnnounce);
                    assert(pindex);
                    if (m_chainman.ActiveChain()[pindex->nHeight] != pindex) {
                        LogPrint(BCLog::NET, "Announcing block %s not on main chain (tip=%s)\n",
                            hashToAnnounce.ToString(), m_chainman.ActiveChain().Tip()->GetBlockHash().ToString());
                    }
                    if (!PeerHasHeader(&state, pindex)) {
                        peer->m_blocks_for_inv_relay.push_back(hashToAnnounce);
                        LogPrint(BCLog::NET, "%s: sending inv peer=%d hash=%s\n", __func__,
                            pto->GetId(), hashToAnnounce.ToString());
                    }
                }
            }
            peer->m_blocks_for_headers_relay.clear();
        }
        std::vector<CInv> vInv;
        {
            LOCK(peer->m_block_inv_mutex);
            vInv.reserve(std::max<size_t>(peer->m_blocks_for_inv_relay.size(), INVENTORY_BROADCAST_MAX));
            for (const uint256& hash : peer->m_blocks_for_inv_relay) {
                vInv.push_back(CInv(MSG_BLOCK, hash));
                if (vInv.size() == MAX_INV_SZ) {
                    m_connman.PushMessage(pto, msgMaker.Make(NetMsgType::INV, vInv));
                    vInv.clear();
                }
            }
            peer->m_blocks_for_inv_relay.clear();
        }
        if (auto tx_relay = peer->GetTxRelay(); tx_relay != nullptr) {
                LOCK(tx_relay->m_tx_inventory_mutex);
                bool fSendTrickle = pto->HasPermission(NetPermissionFlags::NoBan);
                if (tx_relay->m_next_inv_send_time < current_time) {
                    fSendTrickle = true;
                    if (pto->IsInboundConn()) {
                        tx_relay->m_next_inv_send_time = NextInvToInbounds(current_time, INBOUND_INVENTORY_BROADCAST_INTERVAL);
                    } else {
                        tx_relay->m_next_inv_send_time = GetExponentialRand(current_time, OUTBOUND_INVENTORY_BROADCAST_INTERVAL);
                    }
                }
                if (fSendTrickle) {
                    LOCK(tx_relay->m_bloom_filter_mutex);
                    if (!tx_relay->m_relay_txs) tx_relay->m_tx_inventory_to_send.clear();
                }
                if (fSendTrickle && tx_relay->m_send_mempool) {
                    auto vtxinfo = m_mempool.infoAll();
                    tx_relay->m_send_mempool = false;
                    const CFeeRate filterrate{tx_relay->m_fee_filter_received.load()};
                    LOCK(tx_relay->m_bloom_filter_mutex);
                    for (const auto& txinfo : vtxinfo) {
                        const uint256& hash = peer->m_wtxid_relay ? txinfo.tx->GetWitnessHash() : txinfo.tx->GetHash();
                        CInv inv(peer->m_wtxid_relay ? MSG_WTX : MSG_TX, hash);
                        tx_relay->m_tx_inventory_to_send.erase(hash);
                        if (txinfo.fee < filterrate.GetFee(txinfo.vsize)) {
                            continue;
                        }
                        if (tx_relay->m_bloom_filter) {
                            if (!tx_relay->m_bloom_filter->IsRelevantAndUpdate(*txinfo.tx)) continue;
                        }
                        tx_relay->m_tx_inventory_known_filter.insert(hash);
                        vInv.push_back(inv);
                        if (vInv.size() == MAX_INV_SZ) {
                            m_connman.PushMessage(pto, msgMaker.Make(NetMsgType::INV, vInv));
                            vInv.clear();
                        }
                    }
                    tx_relay->m_last_mempool_req = std::chrono::duration_cast<std::chrono::seconds>(current_time);
                }
                if (fSendTrickle) {
                    std::vector<std::set<uint256>::iterator> vInvTx;
                    vInvTx.reserve(tx_relay->m_tx_inventory_to_send.size());
                    for (std::set<uint256>::iterator it = tx_relay->m_tx_inventory_to_send.begin(); it != tx_relay->m_tx_inventory_to_send.end(); it++) {
                        vInvTx.push_back(it);
                    }
                    const CFeeRate filterrate{tx_relay->m_fee_filter_received.load()};
                    CompareInvMempoolOrder compareInvMempoolOrder(&m_mempool, peer->m_wtxid_relay);
                    std::make_heap(vInvTx.begin(), vInvTx.end(), compareInvMempoolOrder);
                    unsigned int nRelayedTransactions = 0;
                    LOCK(tx_relay->m_bloom_filter_mutex);
                    while (!vInvTx.empty() && nRelayedTransactions < INVENTORY_BROADCAST_MAX) {
                        std::pop_heap(vInvTx.begin(), vInvTx.end(), compareInvMempoolOrder);
                        std::set<uint256>::iterator it = vInvTx.back();
                        vInvTx.pop_back();
                        uint256 hash = *it;
                        CInv inv(peer->m_wtxid_relay ? MSG_WTX : MSG_TX, hash);
                        tx_relay->m_tx_inventory_to_send.erase(it);
                        if (tx_relay->m_tx_inventory_known_filter.contains(hash)) {
                            continue;
                        }
                        auto txinfo = m_mempool.info(ToGenTxid(inv));
                        if (!txinfo.tx) {
                            continue;
                        }
                        auto txid = txinfo.tx->GetHash();
                        auto wtxid = txinfo.tx->GetWitnessHash();
                        if (txinfo.fee < filterrate.GetFee(txinfo.vsize)) {
                            continue;
                        }
                        if (tx_relay->m_bloom_filter && !tx_relay->m_bloom_filter->IsRelevantAndUpdate(*txinfo.tx)) continue;
                        State(pto->GetId())->m_recently_announced_invs.insert(hash);
                        vInv.push_back(inv);
                        nRelayedTransactions++;
                        {
                            while (!g_relay_expiration.empty() && g_relay_expiration.front().first < current_time)
                            {
                                mapRelay.erase(g_relay_expiration.front().second);
                                g_relay_expiration.pop_front();
                            }
                            auto ret = mapRelay.emplace(txid, std::move(txinfo.tx));
                            if (ret.second) {
                                g_relay_expiration.emplace_back(current_time + RELAY_TX_CACHE_TIME, ret.first);
                            }
                            auto ret2 = mapRelay.emplace(wtxid, ret.first->second);
                            if (ret2.second) {
                                g_relay_expiration.emplace_back(current_time + RELAY_TX_CACHE_TIME, ret2.first);
                            }
                        }
                        if (vInv.size() == MAX_INV_SZ) {
                            m_connman.PushMessage(pto, msgMaker.Make(NetMsgType::INV, vInv));
                            vInv.clear();
                        }
                        tx_relay->m_tx_inventory_known_filter.insert(hash);
                        if (hash != txid) {
                            tx_relay->m_tx_inventory_known_filter.insert(txid);
                        }
                    }
                }
        }
        if (!vInv.empty())
            m_connman.PushMessage(pto, msgMaker.Make(NetMsgType::INV, vInv));
        if (state.m_stalling_since.count() && state.m_stalling_since < current_time - BLOCK_STALLING_TIMEOUT) {
            LogPrintf("Peer=%d is stalling block download, disconnecting\n", pto->GetId());
            pto->fDisconnect = true;
            return true;
        }
        if (state.vBlocksInFlight.size() > 0) {
            QueuedBlock &queuedBlock = state.vBlocksInFlight.front();
            int nOtherPeersWithValidatedDownloads = m_peers_downloading_from - 1;
            if (current_time > state.m_downloading_since + std::chrono::seconds{consensusParams.nPowTargetSpacing} * (BLOCK_DOWNLOAD_TIMEOUT_BASE + BLOCK_DOWNLOAD_TIMEOUT_PER_PEER * nOtherPeersWithValidatedDownloads)) {
                LogPrintf("Timeout downloading block %s from peer=%d, disconnecting\n", queuedBlock.pindex->GetBlockHash().ToString(), pto->GetId());
                pto->fDisconnect = true;
                return true;
            }
        }
        if (state.fSyncStarted && state.m_headers_sync_timeout < std::chrono::microseconds::max()) {
            if (m_chainman.m_best_header->Time() <= GetAdjustedTime() - 24h) {
                if (current_time > state.m_headers_sync_timeout && nSyncStarted == 1 && (m_num_preferred_download_peers - state.fPreferredDownload >= 1)) {
                    if (!pto->HasPermission(NetPermissionFlags::NoBan)) {
                        LogPrintf("Timeout downloading headers from peer=%d, disconnecting\n", pto->GetId());
                        pto->fDisconnect = true;
                        return true;
                    } else {
                        LogPrintf("Timeout downloading headers from noban peer=%d, not disconnecting\n", pto->GetId());
                        state.fSyncStarted = false;
                        nSyncStarted--;
                        state.m_headers_sync_timeout = 0us;
                    }
                }
            } else {
                state.m_headers_sync_timeout = std::chrono::microseconds::max();
            }
        }
        ConsiderEviction(*pto, *peer, GetTime<std::chrono::seconds>());
        std::vector<CInv> vGetData;
        if (CanServeBlocks(*peer) && ((sync_blocks_and_headers_from_peer && !IsLimitedPeer(*peer)) || !m_chainman.ActiveChainstate().IsInitialBlockDownload()) && state.nBlocksInFlight < MAX_BLOCKS_IN_TRANSIT_PER_PEER) {
            std::vector<const CBlockIndex*> vToDownload;
            NodeId staller = -1;
            FindNextBlocksToDownload(*peer, MAX_BLOCKS_IN_TRANSIT_PER_PEER - state.nBlocksInFlight, vToDownload, staller);
            for (const CBlockIndex *pindex : vToDownload) {
                uint32_t nFetchFlags = GetFetchFlags(*peer);
                vGetData.push_back(CInv(MSG_BLOCK | nFetchFlags, pindex->GetBlockHash()));
                BlockRequested(pto->GetId(), *pindex);
                LogPrint(BCLog::NET, "Requesting block %s (%d) peer=%d\n", pindex->GetBlockHash().ToString(),
                    pindex->nHeight, pto->GetId());
            }
            if (state.nBlocksInFlight == 0 && staller != -1) {
                if (State(staller)->m_stalling_since == 0us) {
                    State(staller)->m_stalling_since = current_time;
                    LogPrint(BCLog::NET, "Stall started peer=%d\n", staller);
                }
            }
        }
        std::vector<std::pair<NodeId, GenTxid>> expired;
        auto requestable = m_txrequest.GetRequestable(pto->GetId(), current_time, &expired);
        for (const auto& entry : expired) {
            LogPrint(BCLog::NET, "timeout of inflight %s %s from peer=%d\n", entry.second.IsWtxid() ? "wtx" : "tx",
                entry.second.GetHash().ToString(), entry.first);
        }
        for (const GenTxid& gtxid : requestable) {
            if (!AlreadyHaveTx(gtxid)) {
                LogPrint(BCLog::NET, "Requesting %s %s peer=%d\n", gtxid.IsWtxid() ? "wtx" : "tx",
                    gtxid.GetHash().ToString(), pto->GetId());
                vGetData.emplace_back(gtxid.IsWtxid() ? MSG_WTX : (MSG_TX | GetFetchFlags(*peer)), gtxid.GetHash());
                if (vGetData.size() >= MAX_GETDATA_SZ) {
                    m_connman.PushMessage(pto, msgMaker.Make(NetMsgType::GETDATA, vGetData));
                    vGetData.clear();
                }
                m_txrequest.RequestedTx(pto->GetId(), gtxid.GetHash(), current_time + GETDATA_TX_INTERVAL);
            } else {
                m_txrequest.ForgetTxHash(gtxid.GetHash());
            }
        }
        if (!vGetData.empty())
            m_connman.PushMessage(pto, msgMaker.Make(NetMsgType::GETDATA, vGetData));
    }
    MaybeSendFeefilter(*pto, *peer, current_time);
    return true;
}

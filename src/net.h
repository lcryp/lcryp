#ifndef LCRYP_NET_H
#define LCRYP_NET_H
#include <chainparams.h>
#include <common/bloom.h>
#include <compat/compat.h>
#include <node/connection_types.h>
#include <consensus/amount.h>
#include <crypto/siphash.h>
#include <hash.h>
#include <i2p.h>
#include <net_permissions.h>
#include <netaddress.h>
#include <netbase.h>
#include <netgroup.h>
#include <policy/feerate.h>
#include <protocol.h>
#include <random.h>
#include <span.h>
#include <streams.h>
#include <sync.h>
#include <threadinterrupt.h>
#include <uint256.h>
#include <util/check.h>
#include <util/sock.h>
#include <atomic>
#include <condition_variable>
#include <cstdint>
#include <deque>
#include <functional>
#include <list>
#include <map>
#include <memory>
#include <optional>
#include <thread>
#include <vector>
class AddrMan;
class BanMan;
class CNode;
class CScheduler;
struct bilingual_str;
static const bool DEFAULT_WHITELISTRELAY = true;
static const bool DEFAULT_WHITELISTFORCERELAY = false;
static constexpr std::chrono::minutes TIMEOUT_INTERVAL{20};
static constexpr auto FEELER_INTERVAL = 2min;
static constexpr auto EXTRA_BLOCK_RELAY_ONLY_PEER_INTERVAL = 5min;
static const unsigned int MAX_PROTOCOL_MESSAGE_LENGTH = 4 * 1000 * 1000;
static const unsigned int MAX_SUBVERSION_LENGTH = 256;
static const int MAX_OUTBOUND_FULL_RELAY_CONNECTIONS = 8;
static const int MAX_ADDNODE_CONNECTIONS = 8;
static const int MAX_BLOCK_RELAY_ONLY_CONNECTIONS = 2;
static const int MAX_FEELER_CONNECTIONS = 1;
static const bool DEFAULT_LISTEN = true;
static const unsigned int DEFAULT_MAX_PEER_CONNECTIONS = 125;
static const std::string DEFAULT_MAX_UPLOAD_TARGET{"0M"};
static const bool DEFAULT_BLOCKSONLY = false;
static const int64_t DEFAULT_PEER_CONNECT_TIMEOUT = 60;
static const int NUM_FDS_MESSAGE_CAPTURE = 1;
static constexpr bool DEFAULT_FORCEDNSSEED{false};
static constexpr bool DEFAULT_DNSSEED{true};
static constexpr bool DEFAULT_FIXEDSEEDS{true};
static const size_t DEFAULT_MAXRECEIVEBUFFER = 5 * 1000;
static const size_t DEFAULT_MAXSENDBUFFER    = 1 * 1000;
typedef int64_t NodeId;
struct AddedNodeInfo
{
    std::string strAddedNode;
    CService resolvedAddress;
    bool fConnected;
    bool fInbound;
};
class CNodeStats;
class CClientUIInterface;
struct CSerializedNetMsg {
    CSerializedNetMsg() = default;
    CSerializedNetMsg(CSerializedNetMsg&&) = default;
    CSerializedNetMsg& operator=(CSerializedNetMsg&&) = default;
    CSerializedNetMsg(const CSerializedNetMsg& msg) = delete;
    CSerializedNetMsg& operator=(const CSerializedNetMsg&) = delete;
    CSerializedNetMsg Copy() const
    {
        CSerializedNetMsg copy;
        copy.data = data;
        copy.m_type = m_type;
        return copy;
    }
    std::vector<unsigned char> data;
    std::string m_type;
};
void Discover();
uint16_t GetListenPort();
enum
{
    LOCAL_NONE,
    LOCAL_IF,
    LOCAL_BIND,
    LOCAL_MAPPED,
    LOCAL_MANUAL,
    LOCAL_MAX
};
bool IsPeerAddrLocalGood(CNode *pnode);
std::optional<CService> GetLocalAddrForPeer(CNode& node);
void SetReachable(enum Network net, bool reachable);
bool IsReachable(enum Network net);
bool IsReachable(const CNetAddr& addr);
bool AddLocal(const CService& addr, int nScore = LOCAL_NONE);
bool AddLocal(const CNetAddr& addr, int nScore = LOCAL_NONE);
void RemoveLocal(const CService& addr);
bool SeenLocal(const CService& addr);
bool IsLocal(const CService& addr);
bool GetLocal(CService &addr, const CNetAddr *paddrPeer = nullptr);
CService GetLocalAddress(const CNetAddr& addrPeer);
CService MaybeFlipIPv6toCJDNS(const CService& service);
extern bool fDiscover;
extern bool fListen;
extern std::string strSubVersion;
struct LocalServiceInfo {
    int nScore;
    uint16_t nPort;
};
extern GlobalMutex g_maplocalhost_mutex;
extern std::map<CNetAddr, LocalServiceInfo> mapLocalHost GUARDED_BY(g_maplocalhost_mutex);
extern const std::string NET_MESSAGE_TYPE_OTHER;
using mapMsgTypeSize = std::map<  std::string,   uint64_t>;
class CNodeStats
{
public:
    NodeId nodeid;
    std::chrono::seconds m_last_send;
    std::chrono::seconds m_last_recv;
    std::chrono::seconds m_last_tx_time;
    std::chrono::seconds m_last_block_time;
    std::chrono::seconds m_connected;
    int64_t nTimeOffset;
    std::string m_addr_name;
    int nVersion;
    std::string cleanSubVer;
    bool fInbound;
    bool m_bip152_highbandwidth_to;
    bool m_bip152_highbandwidth_from;
    int m_starting_height;
    uint64_t nSendBytes;
    mapMsgTypeSize mapSendBytesPerMsgType;
    uint64_t nRecvBytes;
    mapMsgTypeSize mapRecvBytesPerMsgType;
    NetPermissionFlags m_permission_flags;
    std::chrono::microseconds m_last_ping_time;
    std::chrono::microseconds m_min_ping_time;
    std::string addrLocal;
    CAddress addr;
    CAddress addrBind;
    Network m_network;
    uint32_t m_mapped_as;
    ConnectionType m_conn_type;
};
class CNetMessage {
public:
    CDataStream m_recv;
    std::chrono::microseconds m_time{0};
    uint32_t m_message_size{0};
    uint32_t m_raw_message_size{0};
    std::string m_type;
    CNetMessage(CDataStream&& recv_in) : m_recv(std::move(recv_in)) {}
    void SetVersion(int nVersionIn)
    {
        m_recv.SetVersion(nVersionIn);
    }
};
class TransportDeserializer {
public:
    virtual bool Complete() const = 0;
    virtual void SetVersion(int version) = 0;
    virtual int Read(Span<const uint8_t>& msg_bytes) = 0;
    virtual CNetMessage GetMessage(std::chrono::microseconds time, bool& reject_message) = 0;
    virtual ~TransportDeserializer() {}
};
class V1TransportDeserializer final : public TransportDeserializer
{
private:
    const CChainParams& m_chain_params;
    const NodeId m_node_id;
    mutable CHash256 hasher;
    mutable uint256 data_hash;
    bool in_data;
    CDataStream hdrbuf;
    CMessageHeader hdr;
    CDataStream vRecv;
    unsigned int nHdrPos;
    unsigned int nDataPos;
    const uint256& GetMessageHash() const;
    int readHeader(Span<const uint8_t> msg_bytes);
    int readData(Span<const uint8_t> msg_bytes);
    void Reset() {
        vRecv.clear();
        hdrbuf.clear();
        hdrbuf.resize(24);
        in_data = false;
        nHdrPos = 0;
        nDataPos = 0;
        data_hash.SetNull();
        hasher.Reset();
    }
public:
    V1TransportDeserializer(const CChainParams& chain_params, const NodeId node_id, int nTypeIn, int nVersionIn)
        : m_chain_params(chain_params),
          m_node_id(node_id),
          hdrbuf(nTypeIn, nVersionIn),
          vRecv(nTypeIn, nVersionIn)
    {
        Reset();
    }
    bool Complete() const override
    {
        if (!in_data)
            return false;
        return (hdr.nMessageSize == nDataPos);
    }
    void SetVersion(int nVersionIn) override
    {
        hdrbuf.SetVersion(nVersionIn);
        vRecv.SetVersion(nVersionIn);
    }
    int Read(Span<const uint8_t>& msg_bytes) override
    {
        int ret = in_data ? readData(msg_bytes) : readHeader(msg_bytes);
        if (ret < 0) {
            Reset();
        } else {
            msg_bytes = msg_bytes.subspan(ret);
        }
        return ret;
    }
    CNetMessage GetMessage(std::chrono::microseconds time, bool& reject_message) override;
};
class TransportSerializer {
public:
    virtual void prepareForTransport(CSerializedNetMsg& msg, std::vector<unsigned char>& header) const = 0;
    virtual ~TransportSerializer() {}
};
class V1TransportSerializer : public TransportSerializer {
public:
    void prepareForTransport(CSerializedNetMsg& msg, std::vector<unsigned char>& header) const override;
};
struct CNodeOptions
{
    NetPermissionFlags permission_flags = NetPermissionFlags::None;
    std::unique_ptr<i2p::sam::Session> i2p_sam_session = nullptr;
    bool prefer_evict = false;
};
class CNode
{
    friend class CConnman;
    friend struct ConnmanTestMsg;
public:
    const std::unique_ptr<TransportDeserializer> m_deserializer;
    const std::unique_ptr<const TransportSerializer> m_serializer;
    const NetPermissionFlags m_permission_flags;
    std::shared_ptr<Sock> m_sock GUARDED_BY(m_sock_mutex);
    size_t nSendSize GUARDED_BY(cs_vSend){0};
    size_t nSendOffset GUARDED_BY(cs_vSend){0};
    uint64_t nSendBytes GUARDED_BY(cs_vSend){0};
    std::deque<std::vector<unsigned char>> vSendMsg GUARDED_BY(cs_vSend);
    Mutex cs_vSend;
    Mutex m_sock_mutex;
    Mutex cs_vRecv;
    RecursiveMutex cs_vProcessMsg;
    std::list<CNetMessage> vProcessMsg GUARDED_BY(cs_vProcessMsg);
    size_t nProcessQueueSize GUARDED_BY(cs_vProcessMsg){0};
    uint64_t nRecvBytes GUARDED_BY(cs_vRecv){0};
    std::atomic<std::chrono::seconds> m_last_send{0s};
    std::atomic<std::chrono::seconds> m_last_recv{0s};
    const std::chrono::seconds m_connected;
    std::atomic<int64_t> nTimeOffset{0};
    const CAddress addr;
    const CAddress addrBind;
    const std::string m_addr_name;
    const bool m_inbound_onion;
    std::atomic<int> nVersion{0};
    Mutex m_subver_mutex;
    std::string cleanSubVer GUARDED_BY(m_subver_mutex){};
    const bool m_prefer_evict{false};
    bool HasPermission(NetPermissionFlags permission) const {
        return NetPermissions::HasFlag(m_permission_flags, permission);
    }
    std::atomic_bool fSuccessfullyConnected{false};
    std::atomic_bool fDisconnect{false};
    CSemaphoreGrant grantOutbound;
    std::atomic<int> nRefCount{0};
    const uint64_t nKeyedNetGroup;
    std::atomic_bool fPauseRecv{false};
    std::atomic_bool fPauseSend{false};
    bool IsOutboundOrBlockRelayConn() const {
        switch (m_conn_type) {
            case ConnectionType::OUTBOUND_FULL_RELAY:
            case ConnectionType::BLOCK_RELAY:
                return true;
            case ConnectionType::INBOUND:
            case ConnectionType::MANUAL:
            case ConnectionType::ADDR_FETCH:
            case ConnectionType::FEELER:
                return false;
        }
        assert(false);
    }
    bool IsFullOutboundConn() const {
        return m_conn_type == ConnectionType::OUTBOUND_FULL_RELAY;
    }
    bool IsManualConn() const {
        return m_conn_type == ConnectionType::MANUAL;
    }
    bool IsBlockOnlyConn() const {
        return m_conn_type == ConnectionType::BLOCK_RELAY;
    }
    bool IsFeelerConn() const {
        return m_conn_type == ConnectionType::FEELER;
    }
    bool IsAddrFetchConn() const {
        return m_conn_type == ConnectionType::ADDR_FETCH;
    }
    bool IsInboundConn() const {
        return m_conn_type == ConnectionType::INBOUND;
    }
    bool ExpectServicesFromConn() const {
        switch (m_conn_type) {
            case ConnectionType::INBOUND:
            case ConnectionType::MANUAL:
            case ConnectionType::FEELER:
                return false;
            case ConnectionType::OUTBOUND_FULL_RELAY:
            case ConnectionType::BLOCK_RELAY:
            case ConnectionType::ADDR_FETCH:
                return true;
        }
        assert(false);
    }
    Network ConnectedThroughNetwork() const;
    std::atomic<bool> m_bip152_highbandwidth_to{false};
    std::atomic<bool> m_bip152_highbandwidth_from{false};
    std::atomic_bool m_has_all_wanted_services{false};
    std::atomic_bool m_relays_txs{false};
    std::atomic_bool m_bloom_filter_loaded{false};
    std::atomic<std::chrono::seconds> m_last_block_time{0s};
    std::atomic<std::chrono::seconds> m_last_tx_time{0s};
    std::atomic<std::chrono::microseconds> m_last_ping_time{0us};
    std::atomic<std::chrono::microseconds> m_min_ping_time{std::chrono::microseconds::max()};
    CNode(NodeId id,
          std::shared_ptr<Sock> sock,
          const CAddress& addrIn,
          uint64_t nKeyedNetGroupIn,
          uint64_t nLocalHostNonceIn,
          const CAddress& addrBindIn,
          const std::string& addrNameIn,
          ConnectionType conn_type_in,
          bool inbound_onion,
          CNodeOptions&& node_opts = {});
    CNode(const CNode&) = delete;
    CNode& operator=(const CNode&) = delete;
    NodeId GetId() const {
        return id;
    }
    uint64_t GetLocalNonce() const {
        return nLocalHostNonce;
    }
    int GetRefCount() const
    {
        assert(nRefCount >= 0);
        return nRefCount;
    }
    bool ReceiveMsgBytes(Span<const uint8_t> msg_bytes, bool& complete) EXCLUSIVE_LOCKS_REQUIRED(!cs_vRecv);
    void SetCommonVersion(int greatest_common_version)
    {
        Assume(m_greatest_common_version == INIT_PROTO_VERSION);
        m_greatest_common_version = greatest_common_version;
    }
    int GetCommonVersion() const
    {
        return m_greatest_common_version;
    }
    CService GetAddrLocal() const EXCLUSIVE_LOCKS_REQUIRED(!m_addr_local_mutex);
    void SetAddrLocal(const CService& addrLocalIn) EXCLUSIVE_LOCKS_REQUIRED(!m_addr_local_mutex);
    CNode* AddRef()
    {
        nRefCount++;
        return this;
    }
    void Release()
    {
        nRefCount--;
    }
    void CloseSocketDisconnect() EXCLUSIVE_LOCKS_REQUIRED(!m_sock_mutex);
    void CopyStats(CNodeStats& stats) EXCLUSIVE_LOCKS_REQUIRED(!m_subver_mutex, !m_addr_local_mutex, !cs_vSend, !cs_vRecv);
    std::string ConnectionTypeAsString() const { return ::ConnectionTypeAsString(m_conn_type); }
    void PongReceived(std::chrono::microseconds ping_time) {
        m_last_ping_time = ping_time;
        m_min_ping_time = std::min(m_min_ping_time.load(), ping_time);
    }
private:
    const NodeId id;
    const uint64_t nLocalHostNonce;
    const ConnectionType m_conn_type;
    std::atomic<int> m_greatest_common_version{INIT_PROTO_VERSION};
    std::list<CNetMessage> vRecvMsg;
    CService addrLocal GUARDED_BY(m_addr_local_mutex);
    mutable Mutex m_addr_local_mutex;
    mapMsgTypeSize mapSendBytesPerMsgType GUARDED_BY(cs_vSend);
    mapMsgTypeSize mapRecvBytesPerMsgType GUARDED_BY(cs_vRecv);
    std::unique_ptr<i2p::sam::Session> m_i2p_sam_session GUARDED_BY(m_sock_mutex);
};
class NetEventsInterface
{
public:
    static Mutex g_msgproc_mutex;
    virtual void InitializeNode(CNode& node, ServiceFlags our_services) = 0;
    virtual void FinalizeNode(const CNode& node) = 0;
    virtual bool ProcessMessages(CNode* pnode, std::atomic<bool>& interrupt) EXCLUSIVE_LOCKS_REQUIRED(g_msgproc_mutex) = 0;
    virtual bool SendMessages(CNode* pnode) EXCLUSIVE_LOCKS_REQUIRED(g_msgproc_mutex) = 0;
protected:
    ~NetEventsInterface() = default;
};
class CConnman
{
public:
    struct Options
    {
        ServiceFlags nLocalServices = NODE_NONE;
        int nMaxConnections = 0;
        int m_max_outbound_full_relay = 0;
        int m_max_outbound_block_relay = 0;
        int nMaxAddnode = 0;
        int nMaxFeeler = 0;
        CClientUIInterface* uiInterface = nullptr;
        NetEventsInterface* m_msgproc = nullptr;
        BanMan* m_banman = nullptr;
        unsigned int nSendBufferMaxSize = 0;
        unsigned int nReceiveFloodSize = 0;
        uint64_t nMaxOutboundLimit = 0;
        int64_t m_peer_connect_timeout = DEFAULT_PEER_CONNECT_TIMEOUT;
        std::vector<std::string> vSeedNodes;
        std::vector<NetWhitelistPermissions> vWhitelistedRange;
        std::vector<NetWhitebindPermissions> vWhiteBinds;
        std::vector<CService> vBinds;
        std::vector<CService> onion_binds;
        bool bind_on_any;
        bool m_use_addrman_outgoing = true;
        std::vector<std::string> m_specified_outgoing;
        std::vector<std::string> m_added_nodes;
        bool m_i2p_accept_incoming;
    };
    void Init(const Options& connOptions) EXCLUSIVE_LOCKS_REQUIRED(!m_added_nodes_mutex, !m_total_bytes_sent_mutex)
    {
        AssertLockNotHeld(m_total_bytes_sent_mutex);
        nLocalServices = connOptions.nLocalServices;
        nMaxConnections = connOptions.nMaxConnections;
        m_max_outbound_full_relay = std::min(connOptions.m_max_outbound_full_relay, connOptions.nMaxConnections);
        m_max_outbound_block_relay = connOptions.m_max_outbound_block_relay;
        m_use_addrman_outgoing = connOptions.m_use_addrman_outgoing;
        nMaxAddnode = connOptions.nMaxAddnode;
        nMaxFeeler = connOptions.nMaxFeeler;
        m_max_outbound = m_max_outbound_full_relay + m_max_outbound_block_relay + nMaxFeeler;
        m_client_interface = connOptions.uiInterface;
        m_banman = connOptions.m_banman;
        m_msgproc = connOptions.m_msgproc;
        nSendBufferMaxSize = connOptions.nSendBufferMaxSize;
        nReceiveFloodSize = connOptions.nReceiveFloodSize;
        m_peer_connect_timeout = std::chrono::seconds{connOptions.m_peer_connect_timeout};
        {
            LOCK(m_total_bytes_sent_mutex);
            nMaxOutboundLimit = connOptions.nMaxOutboundLimit;
        }
        vWhitelistedRange = connOptions.vWhitelistedRange;
        {
            LOCK(m_added_nodes_mutex);
            m_added_nodes = connOptions.m_added_nodes;
        }
        m_onion_binds = connOptions.onion_binds;
    }
    CConnman(uint64_t seed0, uint64_t seed1, AddrMan& addrman, const NetGroupManager& netgroupman,
             bool network_active = true);
    ~CConnman();
    bool Start(CScheduler& scheduler, const Options& options) EXCLUSIVE_LOCKS_REQUIRED(!m_total_bytes_sent_mutex, !m_added_nodes_mutex, !m_addr_fetches_mutex, !mutexMsgProc);
    void StopThreads();
    void StopNodes();
    void Stop()
    {
        StopThreads();
        StopNodes();
    };
    void Interrupt() EXCLUSIVE_LOCKS_REQUIRED(!mutexMsgProc);
    bool GetNetworkActive() const { return fNetworkActive; };
    bool GetUseAddrmanOutgoing() const { return m_use_addrman_outgoing; };
    void SetNetworkActive(bool active);
    void OpenNetworkConnection(const CAddress& addrConnect, bool fCountFailure, CSemaphoreGrant* grantOutbound, const char* strDest, ConnectionType conn_type);
    bool CheckIncomingNonce(uint64_t nonce);
    bool ForNode(NodeId id, std::function<bool(CNode* pnode)> func);
    void PushMessage(CNode* pnode, CSerializedNetMsg&& msg) EXCLUSIVE_LOCKS_REQUIRED(!m_total_bytes_sent_mutex);
    using NodeFn = std::function<void(CNode*)>;
    void ForEachNode(const NodeFn& func)
    {
        LOCK(m_nodes_mutex);
        for (auto&& node : m_nodes) {
            if (NodeFullyConnected(node))
                func(node);
        }
    };
    void ForEachNode(const NodeFn& func) const
    {
        LOCK(m_nodes_mutex);
        for (auto&& node : m_nodes) {
            if (NodeFullyConnected(node))
                func(node);
        }
    };
    std::vector<CAddress> GetAddresses(size_t max_addresses, size_t max_pct, std::optional<Network> network) const;
    std::vector<CAddress> GetAddresses(CNode& requestor, size_t max_addresses, size_t max_pct);
    void SetTryNewOutboundPeer(bool flag);
    bool GetTryNewOutboundPeer() const;
    void StartExtraBlockRelayPeers();
    int GetExtraFullOutboundCount() const;
    int GetExtraBlockRelayCount() const;
    bool AddNode(const std::string& node) EXCLUSIVE_LOCKS_REQUIRED(!m_added_nodes_mutex);
    bool RemoveAddedNode(const std::string& node) EXCLUSIVE_LOCKS_REQUIRED(!m_added_nodes_mutex);
    std::vector<AddedNodeInfo> GetAddedNodeInfo() const EXCLUSIVE_LOCKS_REQUIRED(!m_added_nodes_mutex);
    bool AddConnection(const std::string& address, ConnectionType conn_type);
    size_t GetNodeCount(ConnectionDirection) const;
    void GetNodeStats(std::vector<CNodeStats>& vstats) const;
    bool DisconnectNode(const std::string& node);
    bool DisconnectNode(const CSubNet& subnet);
    bool DisconnectNode(const CNetAddr& addr);
    bool DisconnectNode(NodeId id);
    ServiceFlags GetLocalServices() const;
    uint64_t GetMaxOutboundTarget() const EXCLUSIVE_LOCKS_REQUIRED(!m_total_bytes_sent_mutex);
    std::chrono::seconds GetMaxOutboundTimeframe() const;
    bool OutboundTargetReached(bool historicalBlockServingLimit) const EXCLUSIVE_LOCKS_REQUIRED(!m_total_bytes_sent_mutex);
    uint64_t GetOutboundTargetBytesLeft() const EXCLUSIVE_LOCKS_REQUIRED(!m_total_bytes_sent_mutex);
    std::chrono::seconds GetMaxOutboundTimeLeftInCycle() const EXCLUSIVE_LOCKS_REQUIRED(!m_total_bytes_sent_mutex);
    uint64_t GetTotalBytesRecv() const;
    uint64_t GetTotalBytesSent() const EXCLUSIVE_LOCKS_REQUIRED(!m_total_bytes_sent_mutex);
    CSipHasher GetDeterministicRandomizer(uint64_t id) const;
    unsigned int GetReceiveFloodSize() const;
    void WakeMessageHandler() EXCLUSIVE_LOCKS_REQUIRED(!mutexMsgProc);
    bool ShouldRunInactivityChecks(const CNode& node, std::chrono::seconds now) const;
private:
    struct ListenSocket {
    public:
        std::shared_ptr<Sock> sock;
        inline void AddSocketPermissionFlags(NetPermissionFlags& flags) const { NetPermissions::AddFlag(flags, m_permissions); }
        ListenSocket(std::shared_ptr<Sock> sock_, NetPermissionFlags permissions_)
            : sock{sock_}, m_permissions{permissions_}
        {
        }
    private:
        NetPermissionFlags m_permissions;
    };
    std::chrono::seconds GetMaxOutboundTimeLeftInCycle_() const EXCLUSIVE_LOCKS_REQUIRED(m_total_bytes_sent_mutex);
    bool BindListenPort(const CService& bindAddr, bilingual_str& strError, NetPermissionFlags permissions);
    bool Bind(const CService& addr, unsigned int flags, NetPermissionFlags permissions);
    bool InitBinds(const Options& options);
    void ThreadOpenAddedConnections() EXCLUSIVE_LOCKS_REQUIRED(!m_added_nodes_mutex);
    void AddAddrFetch(const std::string& strDest) EXCLUSIVE_LOCKS_REQUIRED(!m_addr_fetches_mutex);
    void ProcessAddrFetch() EXCLUSIVE_LOCKS_REQUIRED(!m_addr_fetches_mutex);
    void ThreadOpenConnections(std::vector<std::string> connect) EXCLUSIVE_LOCKS_REQUIRED(!m_addr_fetches_mutex, !m_added_nodes_mutex, !m_nodes_mutex);
    void ThreadMessageHandler() EXCLUSIVE_LOCKS_REQUIRED(!mutexMsgProc);
    void ThreadI2PAcceptIncoming();
    void AcceptConnection(const ListenSocket& hListenSocket);
    void CreateNodeFromAcceptedSocket(std::unique_ptr<Sock>&& sock,
                                      NetPermissionFlags permission_flags,
                                      const CAddress& addr_bind,
                                      const CAddress& addr);
    void DisconnectNodes();
    void NotifyNumConnectionsChanged();
    bool InactivityCheck(const CNode& node) const;
    Sock::EventsPerSock GenerateWaitSockets(Span<CNode* const> nodes);
    void SocketHandler() EXCLUSIVE_LOCKS_REQUIRED(!m_total_bytes_sent_mutex, !mutexMsgProc);
    void SocketHandlerConnected(const std::vector<CNode*>& nodes,
                                const Sock::EventsPerSock& events_per_sock)
        EXCLUSIVE_LOCKS_REQUIRED(!m_total_bytes_sent_mutex, !mutexMsgProc);
    void SocketHandlerListening(const Sock::EventsPerSock& events_per_sock);
    void ThreadSocketHandler() EXCLUSIVE_LOCKS_REQUIRED(!m_total_bytes_sent_mutex, !mutexMsgProc);
    void ThreadDNSAddressSeed() EXCLUSIVE_LOCKS_REQUIRED(!m_addr_fetches_mutex, !m_nodes_mutex);
    uint64_t CalculateKeyedNetGroup(const CAddress& ad) const;
    CNode* FindNode(const CNetAddr& ip);
    CNode* FindNode(const CSubNet& subNet);
    CNode* FindNode(const std::string& addrName);
    CNode* FindNode(const CService& addr);
    bool AlreadyConnectedToAddress(const CAddress& addr);
    bool AttemptToEvictConnection();
    CNode* ConnectNode(CAddress addrConnect, const char *pszDest, bool fCountFailure, ConnectionType conn_type);
    void AddWhitelistPermissionFlags(NetPermissionFlags& flags, const CNetAddr &addr) const;
    void DeleteNode(CNode* pnode);
    NodeId GetNewNodeId();
    size_t SocketSendData(CNode& node) const EXCLUSIVE_LOCKS_REQUIRED(node.cs_vSend);
    void DumpAddresses();
    void RecordBytesRecv(uint64_t bytes);
    void RecordBytesSent(uint64_t bytes) EXCLUSIVE_LOCKS_REQUIRED(!m_total_bytes_sent_mutex);
    std::vector<CAddress> GetCurrentBlockRelayOnlyConns() const;
    static bool NodeFullyConnected(const CNode* pnode);
    mutable Mutex m_total_bytes_sent_mutex;
    std::atomic<uint64_t> nTotalBytesRecv{0};
    uint64_t nTotalBytesSent GUARDED_BY(m_total_bytes_sent_mutex) {0};
    uint64_t nMaxOutboundTotalBytesSentInCycle GUARDED_BY(m_total_bytes_sent_mutex) {0};
    std::chrono::seconds nMaxOutboundCycleStartTime GUARDED_BY(m_total_bytes_sent_mutex) {0};
    uint64_t nMaxOutboundLimit GUARDED_BY(m_total_bytes_sent_mutex);
    std::chrono::seconds m_peer_connect_timeout;
    std::vector<NetWhitelistPermissions> vWhitelistedRange;
    unsigned int nSendBufferMaxSize{0};
    unsigned int nReceiveFloodSize{0};
    std::vector<ListenSocket> vhListenSocket;
    std::atomic<bool> fNetworkActive{true};
    bool fAddressesInitialized{false};
    AddrMan& addrman;
    const NetGroupManager& m_netgroupman;
    std::deque<std::string> m_addr_fetches GUARDED_BY(m_addr_fetches_mutex);
    Mutex m_addr_fetches_mutex;
    std::vector<std::string> m_added_nodes GUARDED_BY(m_added_nodes_mutex);
    mutable Mutex m_added_nodes_mutex;
    std::vector<CNode*> m_nodes GUARDED_BY(m_nodes_mutex);
    std::list<CNode*> m_nodes_disconnected;
    mutable RecursiveMutex m_nodes_mutex;
    std::atomic<NodeId> nLastNodeId{0};
    unsigned int nPrevNodeCount{0};
    struct CachedAddrResponse {
        std::vector<CAddress> m_addrs_response_cache;
        std::chrono::microseconds m_cache_entry_expiration{0};
    };
    std::map<uint64_t, CachedAddrResponse> m_addr_response_caches;
    ServiceFlags nLocalServices;
    std::unique_ptr<CSemaphore> semOutbound;
    std::unique_ptr<CSemaphore> semAddnode;
    int nMaxConnections;
    int m_max_outbound_full_relay;
    int m_max_outbound_block_relay;
    int nMaxAddnode;
    int nMaxFeeler;
    int m_max_outbound;
    bool m_use_addrman_outgoing;
    CClientUIInterface* m_client_interface;
    NetEventsInterface* m_msgproc;
    BanMan* m_banman;
    std::vector<CAddress> m_anchors;
    const uint64_t nSeed0, nSeed1;
    bool fMsgProcWake GUARDED_BY(mutexMsgProc);
    std::condition_variable condMsgProc;
    Mutex mutexMsgProc;
    std::atomic<bool> flagInterruptMsgProc{false};
    CThreadInterrupt interruptNet;
    std::unique_ptr<i2p::sam::Session> m_i2p_sam_session;
    std::thread threadDNSAddressSeed;
    std::thread threadSocketHandler;
    std::thread threadOpenAddedConnections;
    std::thread threadOpenConnections;
    std::thread threadMessageHandler;
    std::thread threadI2PAcceptIncoming;
    std::atomic_bool m_try_another_outbound_peer;
    std::atomic_bool m_start_extra_block_relay_peers{false};
    std::vector<CService> m_onion_binds;
    class NodesSnapshot
    {
    public:
        explicit NodesSnapshot(const CConnman& connman, bool shuffle)
        {
            {
                LOCK(connman.m_nodes_mutex);
                m_nodes_copy = connman.m_nodes;
                for (auto& node : m_nodes_copy) {
                    node->AddRef();
                }
            }
            if (shuffle) {
                Shuffle(m_nodes_copy.begin(), m_nodes_copy.end(), FastRandomContext{});
            }
        }
        ~NodesSnapshot()
        {
            for (auto& node : m_nodes_copy) {
                node->Release();
            }
        }
        const std::vector<CNode*>& Nodes() const
        {
            return m_nodes_copy;
        }
    private:
        std::vector<CNode*> m_nodes_copy;
    };
    friend struct CConnmanTest;
    friend struct ConnmanTestMsg;
};
void CaptureMessageToFile(const CAddress& addr,
                          const std::string& msg_type,
                          Span<const unsigned char> data,
                          bool is_incoming);
extern std::function<void(const CAddress& addr,
                          const std::string& msg_type,
                          Span<const unsigned char> data,
                          bool is_incoming)>
    CaptureMessage;
#endif

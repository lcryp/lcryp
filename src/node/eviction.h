#ifndef LCRYP_NODE_EVICTION_H
#define LCRYP_NODE_EVICTION_H
#include <node/connection_types.h>
#include <net_permissions.h>
#include <chrono>
#include <cstdint>
#include <optional>
#include <vector>
typedef int64_t NodeId;
struct NodeEvictionCandidate {
    NodeId id;
    std::chrono::seconds m_connected;
    std::chrono::microseconds m_min_ping_time;
    std::chrono::seconds m_last_block_time;
    std::chrono::seconds m_last_tx_time;
    bool fRelevantServices;
    bool m_relay_txs;
    bool fBloomFilter;
    uint64_t nKeyedNetGroup;
    bool prefer_evict;
    bool m_is_local;
    Network m_network;
    bool m_noban;
    ConnectionType m_conn_type;
};
[[nodiscard]] std::optional<NodeId> SelectNodeToEvict(std::vector<NodeEvictionCandidate>&& vEvictionCandidates);
void ProtectEvictionCandidatesByRatio(std::vector<NodeEvictionCandidate>& vEvictionCandidates);
#endif

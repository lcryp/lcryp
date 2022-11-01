#include <node/eviction.h>
#include <algorithm>
#include <array>
#include <chrono>
#include <cstdint>
#include <functional>
#include <map>
#include <vector>
static bool ReverseCompareNodeMinPingTime(const NodeEvictionCandidate &a, const NodeEvictionCandidate &b)
{
    return a.m_min_ping_time > b.m_min_ping_time;
}
static bool ReverseCompareNodeTimeConnected(const NodeEvictionCandidate &a, const NodeEvictionCandidate &b)
{
    return a.m_connected > b.m_connected;
}
static bool CompareNetGroupKeyed(const NodeEvictionCandidate &a, const NodeEvictionCandidate &b) {
    return a.nKeyedNetGroup < b.nKeyedNetGroup;
}
static bool CompareNodeBlockTime(const NodeEvictionCandidate &a, const NodeEvictionCandidate &b)
{
    if (a.m_last_block_time != b.m_last_block_time) return a.m_last_block_time < b.m_last_block_time;
    if (a.fRelevantServices != b.fRelevantServices) return b.fRelevantServices;
    return a.m_connected > b.m_connected;
}
static bool CompareNodeTXTime(const NodeEvictionCandidate &a, const NodeEvictionCandidate &b)
{
    if (a.m_last_tx_time != b.m_last_tx_time) return a.m_last_tx_time < b.m_last_tx_time;
    if (a.m_relay_txs != b.m_relay_txs) return b.m_relay_txs;
    if (a.fBloomFilter != b.fBloomFilter) return a.fBloomFilter;
    return a.m_connected > b.m_connected;
}
static bool CompareNodeBlockRelayOnlyTime(const NodeEvictionCandidate &a, const NodeEvictionCandidate &b)
{
    if (a.m_relay_txs != b.m_relay_txs) return a.m_relay_txs;
    if (a.m_last_block_time != b.m_last_block_time) return a.m_last_block_time < b.m_last_block_time;
    if (a.fRelevantServices != b.fRelevantServices) return b.fRelevantServices;
    return a.m_connected > b.m_connected;
}
struct CompareNodeNetworkTime {
    const bool m_is_local;
    const Network m_network;
    CompareNodeNetworkTime(bool is_local, Network network) : m_is_local(is_local), m_network(network) {}
    bool operator()(const NodeEvictionCandidate& a, const NodeEvictionCandidate& b) const
    {
        if (m_is_local && a.m_is_local != b.m_is_local) return b.m_is_local;
        if ((a.m_network == m_network) != (b.m_network == m_network)) return b.m_network == m_network;
        return a.m_connected > b.m_connected;
    };
};
template <typename T, typename Comparator>
static void EraseLastKElements(
    std::vector<T>& elements, Comparator comparator, size_t k,
    std::function<bool(const NodeEvictionCandidate&)> predicate = [](const NodeEvictionCandidate& n) { return true; })
{
    std::sort(elements.begin(), elements.end(), comparator);
    size_t eraseSize = std::min(k, elements.size());
    elements.erase(std::remove_if(elements.end() - eraseSize, elements.end(), predicate), elements.end());
}
void ProtectNoBanConnections(std::vector<NodeEvictionCandidate>& eviction_candidates)
{
    eviction_candidates.erase(std::remove_if(eviction_candidates.begin(), eviction_candidates.end(),
                                             [](NodeEvictionCandidate const& n) {
                                                 return n.m_noban;
                                             }),
                              eviction_candidates.end());
}
void ProtectOutboundConnections(std::vector<NodeEvictionCandidate>& eviction_candidates)
{
    eviction_candidates.erase(std::remove_if(eviction_candidates.begin(), eviction_candidates.end(),
                                             [](NodeEvictionCandidate const& n) {
                                                 return n.m_conn_type != ConnectionType::INBOUND;
                                             }),
                              eviction_candidates.end());
}
void ProtectEvictionCandidatesByRatio(std::vector<NodeEvictionCandidate>& eviction_candidates)
{
    const size_t initial_size = eviction_candidates.size();
    const size_t total_protect_size{initial_size / 2};
    struct Net { bool is_local; Network id; size_t count; };
    std::array<Net, 4> networks{
        {{false, NET_CJDNS, 0}, {false, NET_I2P, 0}, { true, NET_MAX, 0}, {false, NET_ONION, 0}}};
    for (Net& n : networks) {
        n.count = std::count_if(eviction_candidates.cbegin(), eviction_candidates.cend(),
                                [&n](const NodeEvictionCandidate& c) {
                                    return n.is_local ? c.m_is_local : c.m_network == n.id;
                                });
    }
    std::stable_sort(networks.begin(), networks.end(), [](Net a, Net b) { return a.count < b.count; });
    const size_t max_protect_by_network{total_protect_size / 2};
    size_t num_protected{0};
    while (num_protected < max_protect_by_network) {
        auto num_networks = std::count_if(networks.begin(), networks.end(), [](const Net& n) { return n.count; });
        if (num_networks == 0) {
            break;
        }
        const size_t disadvantaged_to_protect{max_protect_by_network - num_protected};
        const size_t protect_per_network{std::max(disadvantaged_to_protect / num_networks, static_cast<size_t>(1))};
        bool protected_at_least_one{false};
        for (Net& n : networks) {
            if (n.count == 0) continue;
            const size_t before = eviction_candidates.size();
            EraseLastKElements(eviction_candidates, CompareNodeNetworkTime(n.is_local, n.id),
                               protect_per_network, [&n](const NodeEvictionCandidate& c) {
                                   return n.is_local ? c.m_is_local : c.m_network == n.id;
                               });
            const size_t after = eviction_candidates.size();
            if (before > after) {
                protected_at_least_one = true;
                const size_t delta{before - after};
                num_protected += delta;
                if (num_protected >= max_protect_by_network) {
                    break;
                }
                n.count -= delta;
            }
        }
        if (!protected_at_least_one) {
            break;
        }
    }
    assert(num_protected == initial_size - eviction_candidates.size());
    const size_t remaining_to_protect{total_protect_size - num_protected};
    EraseLastKElements(eviction_candidates, ReverseCompareNodeTimeConnected, remaining_to_protect);
}
[[nodiscard]] std::optional<NodeId> SelectNodeToEvict(std::vector<NodeEvictionCandidate>&& vEvictionCandidates)
{
    ProtectNoBanConnections(vEvictionCandidates);
    ProtectOutboundConnections(vEvictionCandidates);
    EraseLastKElements(vEvictionCandidates, CompareNetGroupKeyed, 4);
    EraseLastKElements(vEvictionCandidates, ReverseCompareNodeMinPingTime, 8);
    EraseLastKElements(vEvictionCandidates, CompareNodeTXTime, 4);
    EraseLastKElements(vEvictionCandidates, CompareNodeBlockRelayOnlyTime, 8,
                       [](const NodeEvictionCandidate& n) { return !n.m_relay_txs && n.fRelevantServices; });
    EraseLastKElements(vEvictionCandidates, CompareNodeBlockTime, 4);
    ProtectEvictionCandidatesByRatio(vEvictionCandidates);
    if (vEvictionCandidates.empty()) return std::nullopt;
    if (std::any_of(vEvictionCandidates.begin(),vEvictionCandidates.end(),[](NodeEvictionCandidate const &n){return n.prefer_evict;})) {
        vEvictionCandidates.erase(std::remove_if(vEvictionCandidates.begin(),vEvictionCandidates.end(),
                                  [](NodeEvictionCandidate const &n){return !n.prefer_evict;}),vEvictionCandidates.end());
    }
    uint64_t naMostConnections;
    unsigned int nMostConnections = 0;
    std::chrono::seconds nMostConnectionsTime{0};
    std::map<uint64_t, std::vector<NodeEvictionCandidate> > mapNetGroupNodes;
    for (const NodeEvictionCandidate &node : vEvictionCandidates) {
        std::vector<NodeEvictionCandidate> &group = mapNetGroupNodes[node.nKeyedNetGroup];
        group.push_back(node);
        const auto grouptime{group[0].m_connected};
        if (group.size() > nMostConnections || (group.size() == nMostConnections && grouptime > nMostConnectionsTime)) {
            nMostConnections = group.size();
            nMostConnectionsTime = grouptime;
            naMostConnections = node.nKeyedNetGroup;
        }
    }
    vEvictionCandidates = std::move(mapNetGroupNodes[naMostConnections]);
    return vEvictionCandidates.front().id;
}

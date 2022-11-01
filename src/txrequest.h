#ifndef LCRYP_TXREQUEST_H
#define LCRYP_TXREQUEST_H
#include <primitives/transaction.h>
#include <net.h>
#include <uint256.h>
#include <chrono>
#include <vector>
#include <stdint.h>
class TxRequestTracker {
    class Impl;
    const std::unique_ptr<Impl> m_impl;
public:
    explicit TxRequestTracker(bool deterministic = false);
    ~TxRequestTracker();
    void ReceivedInv(NodeId peer, const GenTxid& gtxid, bool preferred,
        std::chrono::microseconds reqtime);
    void DisconnectedPeer(NodeId peer);
    void ForgetTxHash(const uint256& txhash);
    std::vector<GenTxid> GetRequestable(NodeId peer, std::chrono::microseconds now,
        std::vector<std::pair<NodeId, GenTxid>>* expired = nullptr);
    void RequestedTx(NodeId peer, const uint256& txhash, std::chrono::microseconds expiry);
    void ReceivedResponse(NodeId peer, const uint256& txhash);
    size_t CountInFlight(NodeId peer) const;
    size_t CountCandidates(NodeId peer) const;
    size_t Count(NodeId peer) const;
    size_t Size() const;
    uint64_t ComputePriority(const uint256& txhash, NodeId peer, bool preferred) const;
    void SanityCheck() const;
    void PostGetRequestableSanityCheck(std::chrono::microseconds now) const;
};
#endif

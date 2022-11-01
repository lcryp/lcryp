#include <txrequest.h>
#include <crypto/siphash.h>
#include <net.h>
#include <primitives/transaction.h>
#include <random.h>
#include <uint256.h>
#include <boost/multi_index_container.hpp>
#include <boost/multi_index/ordered_index.hpp>
#include <chrono>
#include <unordered_map>
#include <utility>
#include <assert.h>
namespace {
enum class State : uint8_t {
    CANDIDATE_DELAYED,
    CANDIDATE_READY,
    CANDIDATE_BEST,
    REQUESTED,
    COMPLETED,
};
using SequenceNumber = uint64_t;
struct Announcement {
    const uint256 m_txhash;
    std::chrono::microseconds m_time;
    const NodeId m_peer;
    const SequenceNumber m_sequence : 59;
    const bool m_preferred : 1;
    const bool m_is_wtxid : 1;
    uint8_t m_state : 3;
    State GetState() const { return static_cast<State>(m_state); }
    void SetState(State state) { m_state = static_cast<uint8_t>(state); }
    bool IsSelected() const
    {
        return GetState() == State::CANDIDATE_BEST || GetState() == State::REQUESTED;
    }
    bool IsWaiting() const
    {
        return GetState() == State::REQUESTED || GetState() == State::CANDIDATE_DELAYED;
    }
    bool IsSelectable() const
    {
        return GetState() == State::CANDIDATE_READY || GetState() == State::CANDIDATE_BEST;
    }
    Announcement(const GenTxid& gtxid, NodeId peer, bool preferred, std::chrono::microseconds reqtime,
        SequenceNumber sequence) :
        m_txhash(gtxid.GetHash()), m_time(reqtime), m_peer(peer), m_sequence(sequence), m_preferred(preferred),
        m_is_wtxid(gtxid.IsWtxid()), m_state(static_cast<uint8_t>(State::CANDIDATE_DELAYED)) {}
};
using Priority = uint64_t;
class PriorityComputer {
    const uint64_t m_k0, m_k1;
public:
    explicit PriorityComputer(bool deterministic) :
        m_k0{deterministic ? 0 : GetRand(0xFFFFFFFFFFFFFFFF)},
        m_k1{deterministic ? 0 : GetRand(0xFFFFFFFFFFFFFFFF)} {}
    Priority operator()(const uint256& txhash, NodeId peer, bool preferred) const
    {
        uint64_t low_bits = CSipHasher(m_k0, m_k1).Write(txhash.begin(), txhash.size()).Write(peer).Finalize() >> 1;
        return low_bits | uint64_t{preferred} << 63;
    }
    Priority operator()(const Announcement& ann) const
    {
        return operator()(ann.m_txhash, ann.m_peer, ann.m_preferred);
    }
};
struct ByPeer {};
using ByPeerView = std::tuple<NodeId, bool, const uint256&>;
struct ByPeerViewExtractor
{
    using result_type = ByPeerView;
    result_type operator()(const Announcement& ann) const
    {
        return ByPeerView{ann.m_peer, ann.GetState() == State::CANDIDATE_BEST, ann.m_txhash};
    }
};
struct ByTxHash {};
using ByTxHashView = std::tuple<const uint256&, State, Priority>;
class ByTxHashViewExtractor {
    const PriorityComputer& m_computer;
public:
    explicit ByTxHashViewExtractor(const PriorityComputer& computer) : m_computer(computer) {}
    using result_type = ByTxHashView;
    result_type operator()(const Announcement& ann) const
    {
        const Priority prio = (ann.GetState() == State::CANDIDATE_READY) ? m_computer(ann) : 0;
        return ByTxHashView{ann.m_txhash, ann.GetState(), prio};
    }
};
enum class WaitState {
    FUTURE_EVENT,
    NO_EVENT,
    PAST_EVENT,
};
WaitState GetWaitState(const Announcement& ann)
{
    if (ann.IsWaiting()) return WaitState::FUTURE_EVENT;
    if (ann.IsSelectable()) return WaitState::PAST_EVENT;
    return WaitState::NO_EVENT;
}
struct ByTime {};
using ByTimeView = std::pair<WaitState, std::chrono::microseconds>;
struct ByTimeViewExtractor
{
    using result_type = ByTimeView;
    result_type operator()(const Announcement& ann) const
    {
        return ByTimeView{GetWaitState(ann), ann.m_time};
    }
};
using Index = boost::multi_index_container<
    Announcement,
    boost::multi_index::indexed_by<
        boost::multi_index::ordered_unique<boost::multi_index::tag<ByPeer>, ByPeerViewExtractor>,
        boost::multi_index::ordered_non_unique<boost::multi_index::tag<ByTxHash>, ByTxHashViewExtractor>,
        boost::multi_index::ordered_non_unique<boost::multi_index::tag<ByTime>, ByTimeViewExtractor>
    >
>;
template<typename Tag>
using Iter = typename Index::index<Tag>::type::iterator;
struct PeerInfo {
    size_t m_total = 0;
    size_t m_completed = 0;
    size_t m_requested = 0;
};
struct TxHashInfo
{
    size_t m_candidate_delayed = 0;
    size_t m_candidate_ready = 0;
    size_t m_candidate_best = 0;
    size_t m_requested = 0;
    Priority m_priority_candidate_best = std::numeric_limits<Priority>::max();
    Priority m_priority_best_candidate_ready = std::numeric_limits<Priority>::min();
    std::vector<NodeId> m_peers;
};
bool operator==(const PeerInfo& a, const PeerInfo& b)
{
    return std::tie(a.m_total, a.m_completed, a.m_requested) ==
           std::tie(b.m_total, b.m_completed, b.m_requested);
};
std::unordered_map<NodeId, PeerInfo> RecomputePeerInfo(const Index& index)
{
    std::unordered_map<NodeId, PeerInfo> ret;
    for (const Announcement& ann : index) {
        PeerInfo& info = ret[ann.m_peer];
        ++info.m_total;
        info.m_requested += (ann.GetState() == State::REQUESTED);
        info.m_completed += (ann.GetState() == State::COMPLETED);
    }
    return ret;
}
std::map<uint256, TxHashInfo> ComputeTxHashInfo(const Index& index, const PriorityComputer& computer)
{
    std::map<uint256, TxHashInfo> ret;
    for (const Announcement& ann : index) {
        TxHashInfo& info = ret[ann.m_txhash];
        info.m_candidate_delayed += (ann.GetState() == State::CANDIDATE_DELAYED);
        info.m_candidate_ready += (ann.GetState() == State::CANDIDATE_READY);
        info.m_candidate_best += (ann.GetState() == State::CANDIDATE_BEST);
        info.m_requested += (ann.GetState() == State::REQUESTED);
        if (ann.GetState() == State::CANDIDATE_BEST) {
            info.m_priority_candidate_best = computer(ann);
        }
        if (ann.GetState() == State::CANDIDATE_READY) {
            info.m_priority_best_candidate_ready = std::max(info.m_priority_best_candidate_ready, computer(ann));
        }
        info.m_peers.push_back(ann.m_peer);
    }
    return ret;
}
GenTxid ToGenTxid(const Announcement& ann)
{
    return ann.m_is_wtxid ? GenTxid::Wtxid(ann.m_txhash) : GenTxid::Txid(ann.m_txhash);
}
}
class TxRequestTracker::Impl {
    SequenceNumber m_current_sequence{0};
    const PriorityComputer m_computer;
    Index m_index;
    std::unordered_map<NodeId, PeerInfo> m_peerinfo;
public:
    void SanityCheck() const
    {
        assert(m_peerinfo == RecomputePeerInfo(m_index));
        for (auto& item : ComputeTxHashInfo(m_index, m_computer)) {
            TxHashInfo& info = item.second;
            assert(info.m_candidate_delayed + info.m_candidate_ready + info.m_candidate_best + info.m_requested > 0);
            assert(info.m_candidate_best + info.m_requested <= 1);
            if (info.m_candidate_ready > 0) {
                assert(info.m_candidate_best + info.m_requested == 1);
            }
            if (info.m_candidate_ready && info.m_candidate_best) {
                assert(info.m_priority_candidate_best >= info.m_priority_best_candidate_ready);
            }
            std::sort(info.m_peers.begin(), info.m_peers.end());
            assert(std::adjacent_find(info.m_peers.begin(), info.m_peers.end()) == info.m_peers.end());
        }
    }
    void PostGetRequestableSanityCheck(std::chrono::microseconds now) const
    {
        for (const Announcement& ann : m_index) {
            if (ann.IsWaiting()) {
                assert(ann.m_time > now);
            } else if (ann.IsSelectable()) {
                assert(ann.m_time <= now);
            }
        }
    }
private:
    template<typename Tag>
    Iter<Tag> Erase(Iter<Tag> it)
    {
        auto peerit = m_peerinfo.find(it->m_peer);
        peerit->second.m_completed -= it->GetState() == State::COMPLETED;
        peerit->second.m_requested -= it->GetState() == State::REQUESTED;
        if (--peerit->second.m_total == 0) m_peerinfo.erase(peerit);
        return m_index.get<Tag>().erase(it);
    }
    template<typename Tag, typename Modifier>
    void Modify(Iter<Tag> it, Modifier modifier)
    {
        auto peerit = m_peerinfo.find(it->m_peer);
        peerit->second.m_completed -= it->GetState() == State::COMPLETED;
        peerit->second.m_requested -= it->GetState() == State::REQUESTED;
        m_index.get<Tag>().modify(it, std::move(modifier));
        peerit->second.m_completed += it->GetState() == State::COMPLETED;
        peerit->second.m_requested += it->GetState() == State::REQUESTED;
    }
    void PromoteCandidateReady(Iter<ByTxHash> it)
    {
        assert(it != m_index.get<ByTxHash>().end());
        assert(it->GetState() == State::CANDIDATE_DELAYED);
        Modify<ByTxHash>(it, [](Announcement& ann){ ann.SetState(State::CANDIDATE_READY); });
        auto it_next = std::next(it);
        if (it_next == m_index.get<ByTxHash>().end() || it_next->m_txhash != it->m_txhash ||
            it_next->GetState() == State::COMPLETED) {
            Modify<ByTxHash>(it, [](Announcement& ann){ ann.SetState(State::CANDIDATE_BEST); });
        } else if (it_next->GetState() == State::CANDIDATE_BEST) {
            Priority priority_old = m_computer(*it_next);
            Priority priority_new = m_computer(*it);
            if (priority_new > priority_old) {
                Modify<ByTxHash>(it_next, [](Announcement& ann){ ann.SetState(State::CANDIDATE_READY); });
                Modify<ByTxHash>(it, [](Announcement& ann){ ann.SetState(State::CANDIDATE_BEST); });
            }
        }
    }
    void ChangeAndReselect(Iter<ByTxHash> it, State new_state)
    {
        assert(new_state == State::COMPLETED || new_state == State::CANDIDATE_DELAYED);
        assert(it != m_index.get<ByTxHash>().end());
        if (it->IsSelected() && it != m_index.get<ByTxHash>().begin()) {
            auto it_prev = std::prev(it);
            if (it_prev->m_txhash == it->m_txhash && it_prev->GetState() == State::CANDIDATE_READY) {
                Modify<ByTxHash>(it_prev, [](Announcement& ann){ ann.SetState(State::CANDIDATE_BEST); });
            }
        }
        Modify<ByTxHash>(it, [new_state](Announcement& ann){ ann.SetState(new_state); });
    }
    bool IsOnlyNonCompleted(Iter<ByTxHash> it)
    {
        assert(it != m_index.get<ByTxHash>().end());
        assert(it->GetState() != State::COMPLETED);
        if (it != m_index.get<ByTxHash>().begin() && std::prev(it)->m_txhash == it->m_txhash) return false;
        if (std::next(it) != m_index.get<ByTxHash>().end() && std::next(it)->m_txhash == it->m_txhash &&
            std::next(it)->GetState() != State::COMPLETED) return false;
        return true;
    }
    bool MakeCompleted(Iter<ByTxHash> it)
    {
        assert(it != m_index.get<ByTxHash>().end());
        if (it->GetState() == State::COMPLETED) return true;
        if (IsOnlyNonCompleted(it)) {
            uint256 txhash = it->m_txhash;
            do {
                it = Erase<ByTxHash>(it);
            } while (it != m_index.get<ByTxHash>().end() && it->m_txhash == txhash);
            return false;
        }
        ChangeAndReselect(it, State::COMPLETED);
        return true;
    }
    void SetTimePoint(std::chrono::microseconds now, std::vector<std::pair<NodeId, GenTxid>>* expired)
    {
        if (expired) expired->clear();
        while (!m_index.empty()) {
            auto it = m_index.get<ByTime>().begin();
            if (it->GetState() == State::CANDIDATE_DELAYED && it->m_time <= now) {
                PromoteCandidateReady(m_index.project<ByTxHash>(it));
            } else if (it->GetState() == State::REQUESTED && it->m_time <= now) {
                if (expired) expired->emplace_back(it->m_peer, ToGenTxid(*it));
                MakeCompleted(m_index.project<ByTxHash>(it));
            } else {
                break;
            }
        }
        while (!m_index.empty()) {
            auto it = std::prev(m_index.get<ByTime>().end());
            if (it->IsSelectable() && it->m_time > now) {
                ChangeAndReselect(m_index.project<ByTxHash>(it), State::CANDIDATE_DELAYED);
            } else {
                break;
            }
        }
    }
public:
    explicit Impl(bool deterministic) :
        m_computer(deterministic),
        m_index(boost::make_tuple(
            boost::make_tuple(ByPeerViewExtractor(), std::less<ByPeerView>()),
            boost::make_tuple(ByTxHashViewExtractor(m_computer), std::less<ByTxHashView>()),
            boost::make_tuple(ByTimeViewExtractor(), std::less<ByTimeView>())
        )) {}
    Impl(const Impl&) = delete;
    Impl& operator=(const Impl&) = delete;
    void DisconnectedPeer(NodeId peer)
    {
        auto& index = m_index.get<ByPeer>();
        auto it = index.lower_bound(ByPeerView{peer, false, uint256::ZERO});
        while (it != index.end() && it->m_peer == peer) {
            auto it_next = (std::next(it) == index.end() || std::next(it)->m_peer != peer) ? index.end() :
                std::next(it);
            if (MakeCompleted(m_index.project<ByTxHash>(it))) {
                Erase<ByPeer>(it);
            }
            it = it_next;
        }
    }
    void ForgetTxHash(const uint256& txhash)
    {
        auto it = m_index.get<ByTxHash>().lower_bound(ByTxHashView{txhash, State::CANDIDATE_DELAYED, 0});
        while (it != m_index.get<ByTxHash>().end() && it->m_txhash == txhash) {
            it = Erase<ByTxHash>(it);
        }
    }
    void ReceivedInv(NodeId peer, const GenTxid& gtxid, bool preferred,
        std::chrono::microseconds reqtime)
    {
        if (m_index.get<ByPeer>().count(ByPeerView{peer, true, gtxid.GetHash()})) return;
        auto ret = m_index.get<ByPeer>().emplace(gtxid, peer, preferred, reqtime, m_current_sequence);
        if (!ret.second) return;
        ++m_peerinfo[peer].m_total;
        ++m_current_sequence;
    }
    std::vector<GenTxid> GetRequestable(NodeId peer, std::chrono::microseconds now,
        std::vector<std::pair<NodeId, GenTxid>>* expired)
    {
        SetTimePoint(now, expired);
        std::vector<const Announcement*> selected;
        auto it_peer = m_index.get<ByPeer>().lower_bound(ByPeerView{peer, true, uint256::ZERO});
        while (it_peer != m_index.get<ByPeer>().end() && it_peer->m_peer == peer &&
            it_peer->GetState() == State::CANDIDATE_BEST) {
            selected.emplace_back(&*it_peer);
            ++it_peer;
        }
        std::sort(selected.begin(), selected.end(), [](const Announcement* a, const Announcement* b) {
            return a->m_sequence < b->m_sequence;
        });
        std::vector<GenTxid> ret;
        ret.reserve(selected.size());
        std::transform(selected.begin(), selected.end(), std::back_inserter(ret), [](const Announcement* ann) {
            return ToGenTxid(*ann);
        });
        return ret;
    }
    void RequestedTx(NodeId peer, const uint256& txhash, std::chrono::microseconds expiry)
    {
        auto it = m_index.get<ByPeer>().find(ByPeerView{peer, true, txhash});
        if (it == m_index.get<ByPeer>().end()) {
            it = m_index.get<ByPeer>().find(ByPeerView{peer, false, txhash});
            if (it == m_index.get<ByPeer>().end() || (it->GetState() != State::CANDIDATE_DELAYED &&
                                                      it->GetState() != State::CANDIDATE_READY)) {
                return;
            }
            auto it_old = m_index.get<ByTxHash>().lower_bound(ByTxHashView{txhash, State::CANDIDATE_BEST, 0});
            if (it_old != m_index.get<ByTxHash>().end() && it_old->m_txhash == txhash) {
                if (it_old->GetState() == State::CANDIDATE_BEST) {
                    Modify<ByTxHash>(it_old, [](Announcement& ann) { ann.SetState(State::CANDIDATE_READY); });
                } else if (it_old->GetState() == State::REQUESTED) {
                    Modify<ByTxHash>(it_old, [](Announcement& ann) { ann.SetState(State::COMPLETED); });
                }
            }
        }
        Modify<ByPeer>(it, [expiry](Announcement& ann) {
            ann.SetState(State::REQUESTED);
            ann.m_time = expiry;
        });
    }
    void ReceivedResponse(NodeId peer, const uint256& txhash)
    {
        auto it = m_index.get<ByPeer>().find(ByPeerView{peer, false, txhash});
        if (it == m_index.get<ByPeer>().end()) {
            it = m_index.get<ByPeer>().find(ByPeerView{peer, true, txhash});
        }
        if (it != m_index.get<ByPeer>().end()) MakeCompleted(m_index.project<ByTxHash>(it));
    }
    size_t CountInFlight(NodeId peer) const
    {
        auto it = m_peerinfo.find(peer);
        if (it != m_peerinfo.end()) return it->second.m_requested;
        return 0;
    }
    size_t CountCandidates(NodeId peer) const
    {
        auto it = m_peerinfo.find(peer);
        if (it != m_peerinfo.end()) return it->second.m_total - it->second.m_requested - it->second.m_completed;
        return 0;
    }
    size_t Count(NodeId peer) const
    {
        auto it = m_peerinfo.find(peer);
        if (it != m_peerinfo.end()) return it->second.m_total;
        return 0;
    }
    size_t Size() const { return m_index.size(); }
    uint64_t ComputePriority(const uint256& txhash, NodeId peer, bool preferred) const
    {
        return uint64_t{m_computer(txhash, peer, preferred)};
    }
};
TxRequestTracker::TxRequestTracker(bool deterministic) :
    m_impl{std::make_unique<TxRequestTracker::Impl>(deterministic)} {}
TxRequestTracker::~TxRequestTracker() = default;
void TxRequestTracker::ForgetTxHash(const uint256& txhash) { m_impl->ForgetTxHash(txhash); }
void TxRequestTracker::DisconnectedPeer(NodeId peer) { m_impl->DisconnectedPeer(peer); }
size_t TxRequestTracker::CountInFlight(NodeId peer) const { return m_impl->CountInFlight(peer); }
size_t TxRequestTracker::CountCandidates(NodeId peer) const { return m_impl->CountCandidates(peer); }
size_t TxRequestTracker::Count(NodeId peer) const { return m_impl->Count(peer); }
size_t TxRequestTracker::Size() const { return m_impl->Size(); }
void TxRequestTracker::SanityCheck() const { m_impl->SanityCheck(); }
void TxRequestTracker::PostGetRequestableSanityCheck(std::chrono::microseconds now) const
{
    m_impl->PostGetRequestableSanityCheck(now);
}
void TxRequestTracker::ReceivedInv(NodeId peer, const GenTxid& gtxid, bool preferred,
    std::chrono::microseconds reqtime)
{
    m_impl->ReceivedInv(peer, gtxid, preferred, reqtime);
}
void TxRequestTracker::RequestedTx(NodeId peer, const uint256& txhash, std::chrono::microseconds expiry)
{
    m_impl->RequestedTx(peer, txhash, expiry);
}
void TxRequestTracker::ReceivedResponse(NodeId peer, const uint256& txhash)
{
    m_impl->ReceivedResponse(peer, txhash);
}
std::vector<GenTxid> TxRequestTracker::GetRequestable(NodeId peer, std::chrono::microseconds now,
    std::vector<std::pair<NodeId, GenTxid>>* expired)
{
    return m_impl->GetRequestable(peer, now, expired);
}
uint64_t TxRequestTracker::ComputePriority(const uint256& txhash, NodeId peer, bool preferred) const
{
    return m_impl->ComputePriority(txhash, peer, preferred);
}

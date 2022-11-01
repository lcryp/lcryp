#ifndef LCRYP_ADDRMAN_IMPL_H
#define LCRYP_ADDRMAN_IMPL_H
#include <logging.h>
#include <logging/timer.h>
#include <netaddress.h>
#include <protocol.h>
#include <serialize.h>
#include <sync.h>
#include <timedata.h>
#include <uint256.h>
#include <util/time.h>
#include <cstdint>
#include <optional>
#include <set>
#include <unordered_map>
#include <unordered_set>
#include <utility>
#include <vector>
static constexpr int32_t ADDRMAN_TRIED_BUCKET_COUNT_LOG2{8};
static constexpr int ADDRMAN_TRIED_BUCKET_COUNT{1 << ADDRMAN_TRIED_BUCKET_COUNT_LOG2};
static constexpr int32_t ADDRMAN_NEW_BUCKET_COUNT_LOG2{10};
static constexpr int ADDRMAN_NEW_BUCKET_COUNT{1 << ADDRMAN_NEW_BUCKET_COUNT_LOG2};
static constexpr int32_t ADDRMAN_BUCKET_SIZE_LOG2{6};
static constexpr int ADDRMAN_BUCKET_SIZE{1 << ADDRMAN_BUCKET_SIZE_LOG2};
class AddrInfo : public CAddress
{
public:
    NodeSeconds m_last_try{0s};
    NodeSeconds m_last_count_attempt{0s};
    CNetAddr source;
    NodeSeconds m_last_success{0s};
    int nAttempts{0};
    int nRefCount{0};
    bool fInTried{false};
    mutable int nRandomPos{-1};
    SERIALIZE_METHODS(AddrInfo, obj)
    {
        READWRITEAS(CAddress, obj);
        READWRITE(obj.source, Using<ChronoFormatter<int64_t>>(obj.m_last_success), obj.nAttempts);
    }
    AddrInfo(const CAddress &addrIn, const CNetAddr &addrSource) : CAddress(addrIn), source(addrSource)
    {
    }
    AddrInfo() : CAddress(), source()
    {
    }
    int GetTriedBucket(const uint256& nKey, const NetGroupManager& netgroupman) const;
    int GetNewBucket(const uint256& nKey, const CNetAddr& src, const NetGroupManager& netgroupman) const;
    int GetNewBucket(const uint256& nKey, const NetGroupManager& netgroupman) const
    {
        return GetNewBucket(nKey, source, netgroupman);
    }
    int GetBucketPosition(const uint256 &nKey, bool fNew, int nBucket) const;
    bool IsTerrible(NodeSeconds now = Now<NodeSeconds>()) const;
    double GetChance(NodeSeconds now = Now<NodeSeconds>()) const;
};
class AddrManImpl
{
public:
    AddrManImpl(const NetGroupManager& netgroupman, bool deterministic, int32_t consistency_check_ratio);
    ~AddrManImpl();
    template <typename Stream>
    void Serialize(Stream& s_) const EXCLUSIVE_LOCKS_REQUIRED(!cs);
    template <typename Stream>
    void Unserialize(Stream& s_) EXCLUSIVE_LOCKS_REQUIRED(!cs);
    size_t size() const EXCLUSIVE_LOCKS_REQUIRED(!cs);
    bool Add(const std::vector<CAddress>& vAddr, const CNetAddr& source, std::chrono::seconds time_penalty)
        EXCLUSIVE_LOCKS_REQUIRED(!cs);
    bool Good(const CService& addr, NodeSeconds time)
        EXCLUSIVE_LOCKS_REQUIRED(!cs);
    void Attempt(const CService& addr, bool fCountFailure, NodeSeconds time)
        EXCLUSIVE_LOCKS_REQUIRED(!cs);
    void ResolveCollisions() EXCLUSIVE_LOCKS_REQUIRED(!cs);
    std::pair<CAddress, NodeSeconds> SelectTriedCollision() EXCLUSIVE_LOCKS_REQUIRED(!cs);
    std::pair<CAddress, NodeSeconds> Select(bool newOnly) const
        EXCLUSIVE_LOCKS_REQUIRED(!cs);
    std::vector<CAddress> GetAddr(size_t max_addresses, size_t max_pct, std::optional<Network> network) const
        EXCLUSIVE_LOCKS_REQUIRED(!cs);
    void Connected(const CService& addr, NodeSeconds time)
        EXCLUSIVE_LOCKS_REQUIRED(!cs);
    void SetServices(const CService& addr, ServiceFlags nServices)
        EXCLUSIVE_LOCKS_REQUIRED(!cs);
    std::optional<AddressPosition> FindAddressEntry(const CAddress& addr)
        EXCLUSIVE_LOCKS_REQUIRED(!cs);
    friend class AddrManDeterministic;
private:
    mutable Mutex cs;
    mutable FastRandomContext insecure_rand GUARDED_BY(cs);
    uint256 nKey;
    enum Format : uint8_t {
        V0_HISTORICAL = 0,
        V1_DETERMINISTIC = 1,
        V2_ASMAP = 2,
        V3_BIP155 = 3,
        V4_MULTIPORT = 4,
    };
    static constexpr Format FILE_FORMAT = Format::V4_MULTIPORT;
    static constexpr uint8_t INCOMPATIBILITY_BASE = 32;
    int nIdCount GUARDED_BY(cs){0};
    std::unordered_map<int, AddrInfo> mapInfo GUARDED_BY(cs);
    std::unordered_map<CService, int, CServiceHash> mapAddr GUARDED_BY(cs);
    mutable std::vector<int> vRandom GUARDED_BY(cs);
    int nTried GUARDED_BY(cs){0};
    int vvTried[ADDRMAN_TRIED_BUCKET_COUNT][ADDRMAN_BUCKET_SIZE] GUARDED_BY(cs);
    int nNew GUARDED_BY(cs){0};
    int vvNew[ADDRMAN_NEW_BUCKET_COUNT][ADDRMAN_BUCKET_SIZE] GUARDED_BY(cs);
    NodeSeconds m_last_good GUARDED_BY(cs){1s};
    std::set<int> m_tried_collisions;
    const int32_t m_consistency_check_ratio;
    const NetGroupManager& m_netgroupman;
    AddrInfo* Find(const CService& addr, int* pnId = nullptr) EXCLUSIVE_LOCKS_REQUIRED(cs);
    AddrInfo* Create(const CAddress& addr, const CNetAddr& addrSource, int* pnId = nullptr) EXCLUSIVE_LOCKS_REQUIRED(cs);
    void SwapRandom(unsigned int nRandomPos1, unsigned int nRandomPos2) const EXCLUSIVE_LOCKS_REQUIRED(cs);
    void Delete(int nId) EXCLUSIVE_LOCKS_REQUIRED(cs);
    void ClearNew(int nUBucket, int nUBucketPos) EXCLUSIVE_LOCKS_REQUIRED(cs);
    void MakeTried(AddrInfo& info, int nId) EXCLUSIVE_LOCKS_REQUIRED(cs);
    bool AddSingle(const CAddress& addr, const CNetAddr& source, std::chrono::seconds time_penalty) EXCLUSIVE_LOCKS_REQUIRED(cs);
    bool Good_(const CService& addr, bool test_before_evict, NodeSeconds time) EXCLUSIVE_LOCKS_REQUIRED(cs);
    bool Add_(const std::vector<CAddress>& vAddr, const CNetAddr& source, std::chrono::seconds time_penalty) EXCLUSIVE_LOCKS_REQUIRED(cs);
    void Attempt_(const CService& addr, bool fCountFailure, NodeSeconds time) EXCLUSIVE_LOCKS_REQUIRED(cs);
    std::pair<CAddress, NodeSeconds> Select_(bool newOnly) const EXCLUSIVE_LOCKS_REQUIRED(cs);
    std::vector<CAddress> GetAddr_(size_t max_addresses, size_t max_pct, std::optional<Network> network) const EXCLUSIVE_LOCKS_REQUIRED(cs);
    void Connected_(const CService& addr, NodeSeconds time) EXCLUSIVE_LOCKS_REQUIRED(cs);
    void SetServices_(const CService& addr, ServiceFlags nServices) EXCLUSIVE_LOCKS_REQUIRED(cs);
    void ResolveCollisions_() EXCLUSIVE_LOCKS_REQUIRED(cs);
    std::pair<CAddress, NodeSeconds> SelectTriedCollision_() EXCLUSIVE_LOCKS_REQUIRED(cs);
    std::optional<AddressPosition> FindAddressEntry_(const CAddress& addr) EXCLUSIVE_LOCKS_REQUIRED(cs);
    void Check() const EXCLUSIVE_LOCKS_REQUIRED(cs);
    int CheckAddrman() const EXCLUSIVE_LOCKS_REQUIRED(cs);
};
#endif

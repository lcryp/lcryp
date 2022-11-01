#include <addrman.h>
#include <addrman_impl.h>
#include <hash.h>
#include <logging.h>
#include <logging/timer.h>
#include <netaddress.h>
#include <protocol.h>
#include <random.h>
#include <serialize.h>
#include <streams.h>
#include <tinyformat.h>
#include <uint256.h>
#include <util/check.h>
#include <util/time.h>
#include <cmath>
#include <optional>
static constexpr uint32_t ADDRMAN_TRIED_BUCKETS_PER_GROUP{8};
static constexpr uint32_t ADDRMAN_NEW_BUCKETS_PER_SOURCE_GROUP{64};
static constexpr int32_t ADDRMAN_NEW_BUCKETS_PER_ADDRESS{8};
static constexpr auto ADDRMAN_HORIZON{30 * 24h};
static constexpr int32_t ADDRMAN_RETRIES{3};
static constexpr int32_t ADDRMAN_MAX_FAILURES{10};
static constexpr auto ADDRMAN_MIN_FAIL{7 * 24h};
static constexpr auto ADDRMAN_REPLACEMENT{4h};
static constexpr size_t ADDRMAN_SET_TRIED_COLLISION_SIZE{10};
static constexpr auto ADDRMAN_TEST_WINDOW{40min};
int AddrInfo::GetTriedBucket(const uint256& nKey, const NetGroupManager& netgroupman) const
{
    uint64_t hash1 = (CHashWriter(SER_GETHASH, 0) << nKey << GetKey()).GetCheapHash();
    uint64_t hash2 = (CHashWriter(SER_GETHASH, 0) << nKey << netgroupman.GetGroup(*this) << (hash1 % ADDRMAN_TRIED_BUCKETS_PER_GROUP)).GetCheapHash();
    return hash2 % ADDRMAN_TRIED_BUCKET_COUNT;
}
int AddrInfo::GetNewBucket(const uint256& nKey, const CNetAddr& src, const NetGroupManager& netgroupman) const
{
    std::vector<unsigned char> vchSourceGroupKey = netgroupman.GetGroup(src);
    uint64_t hash1 = (CHashWriter(SER_GETHASH, 0) << nKey << netgroupman.GetGroup(*this) << vchSourceGroupKey).GetCheapHash();
    uint64_t hash2 = (CHashWriter(SER_GETHASH, 0) << nKey << vchSourceGroupKey << (hash1 % ADDRMAN_NEW_BUCKETS_PER_SOURCE_GROUP)).GetCheapHash();
    return hash2 % ADDRMAN_NEW_BUCKET_COUNT;
}
int AddrInfo::GetBucketPosition(const uint256& nKey, bool fNew, int nBucket) const
{
    uint64_t hash1 = (CHashWriter(SER_GETHASH, 0) << nKey << (fNew ? uint8_t{'N'} : uint8_t{'K'}) << nBucket << GetKey()).GetCheapHash();
    return hash1 % ADDRMAN_BUCKET_SIZE;
}
bool AddrInfo::IsTerrible(NodeSeconds now) const
{
    if (now - m_last_try <= 1min) {
        return false;
    }
    if (nTime > now + 10min) {
        return true;
    }
    if (now - nTime > ADDRMAN_HORIZON) {
        return true;
    }
    if (TicksSinceEpoch<std::chrono::seconds>(m_last_success) == 0 && nAttempts >= ADDRMAN_RETRIES) {
        return true;
    }
    if (now - m_last_success > ADDRMAN_MIN_FAIL && nAttempts >= ADDRMAN_MAX_FAILURES) {
        return true;
    }
    return false;
}
double AddrInfo::GetChance(NodeSeconds now) const
{
    double fChance = 1.0;
    if (now - m_last_try < 10min) {
        fChance *= 0.01;
    }
    fChance *= pow(0.66, std::min(nAttempts, 8));
    return fChance;
}
AddrManImpl::AddrManImpl(const NetGroupManager& netgroupman, bool deterministic, int32_t consistency_check_ratio)
    : insecure_rand{deterministic}
    , nKey{deterministic ? uint256{1} : insecure_rand.rand256()}
    , m_consistency_check_ratio{consistency_check_ratio}
    , m_netgroupman{netgroupman}
{
    for (auto& bucket : vvNew) {
        for (auto& entry : bucket) {
            entry = -1;
        }
    }
    for (auto& bucket : vvTried) {
        for (auto& entry : bucket) {
            entry = -1;
        }
    }
}
AddrManImpl::~AddrManImpl()
{
    nKey.SetNull();
}
template <typename Stream>
void AddrManImpl::Serialize(Stream& s_) const
{
    LOCK(cs);
    OverrideStream<Stream> s(&s_, s_.GetType(), s_.GetVersion() | ADDRV2_FORMAT);
    s << static_cast<uint8_t>(FILE_FORMAT);
    static constexpr uint8_t lowest_compatible = Format::V4_MULTIPORT;
    s << static_cast<uint8_t>(INCOMPATIBILITY_BASE + lowest_compatible);
    s << nKey;
    s << nNew;
    s << nTried;
    int nUBuckets = ADDRMAN_NEW_BUCKET_COUNT ^ (1 << 30);
    s << nUBuckets;
    std::unordered_map<int, int> mapUnkIds;
    int nIds = 0;
    for (const auto& entry : mapInfo) {
        mapUnkIds[entry.first] = nIds;
        const AddrInfo& info = entry.second;
        if (info.nRefCount) {
            assert(nIds != nNew);
            s << info;
            nIds++;
        }
    }
    nIds = 0;
    for (const auto& entry : mapInfo) {
        const AddrInfo& info = entry.second;
        if (info.fInTried) {
            assert(nIds != nTried);
            s << info;
            nIds++;
        }
    }
    for (int bucket = 0; bucket < ADDRMAN_NEW_BUCKET_COUNT; bucket++) {
        int nSize = 0;
        for (int i = 0; i < ADDRMAN_BUCKET_SIZE; i++) {
            if (vvNew[bucket][i] != -1)
                nSize++;
        }
        s << nSize;
        for (int i = 0; i < ADDRMAN_BUCKET_SIZE; i++) {
            if (vvNew[bucket][i] != -1) {
                int nIndex = mapUnkIds[vvNew[bucket][i]];
                s << nIndex;
            }
        }
    }
    s << m_netgroupman.GetAsmapChecksum();
}
template <typename Stream>
void AddrManImpl::Unserialize(Stream& s_)
{
    LOCK(cs);
    assert(vRandom.empty());
    Format format;
    s_ >> Using<CustomUintFormatter<1>>(format);
    int stream_version = s_.GetVersion();
    if (format >= Format::V3_BIP155) {
        stream_version |= ADDRV2_FORMAT;
    }
    OverrideStream<Stream> s(&s_, s_.GetType(), stream_version);
    uint8_t compat;
    s >> compat;
    if (compat < INCOMPATIBILITY_BASE) {
        throw std::ios_base::failure(strprintf(
            "Corrupted addrman database: The compat value (%u) "
            "is lower than the expected minimum value %u.",
            compat, INCOMPATIBILITY_BASE));
    }
    const uint8_t lowest_compatible = compat - INCOMPATIBILITY_BASE;
    if (lowest_compatible > FILE_FORMAT) {
        throw InvalidAddrManVersionError(strprintf(
            "Unsupported format of addrman database: %u. It is compatible with formats >=%u, "
            "but the maximum supported by this version of %s is %u.",
            uint8_t{format}, lowest_compatible, PACKAGE_NAME, uint8_t{FILE_FORMAT}));
    }
    s >> nKey;
    s >> nNew;
    s >> nTried;
    int nUBuckets = 0;
    s >> nUBuckets;
    if (format >= Format::V1_DETERMINISTIC) {
        nUBuckets ^= (1 << 30);
    }
    if (nNew > ADDRMAN_NEW_BUCKET_COUNT * ADDRMAN_BUCKET_SIZE || nNew < 0) {
        throw std::ios_base::failure(
                strprintf("Corrupt AddrMan serialization: nNew=%d, should be in [0, %d]",
                    nNew,
                    ADDRMAN_NEW_BUCKET_COUNT * ADDRMAN_BUCKET_SIZE));
    }
    if (nTried > ADDRMAN_TRIED_BUCKET_COUNT * ADDRMAN_BUCKET_SIZE || nTried < 0) {
        throw std::ios_base::failure(
                strprintf("Corrupt AddrMan serialization: nTried=%d, should be in [0, %d]",
                    nTried,
                    ADDRMAN_TRIED_BUCKET_COUNT * ADDRMAN_BUCKET_SIZE));
    }
    for (int n = 0; n < nNew; n++) {
        AddrInfo& info = mapInfo[n];
        s >> info;
        mapAddr[info] = n;
        info.nRandomPos = vRandom.size();
        vRandom.push_back(n);
    }
    nIdCount = nNew;
    int nLost = 0;
    for (int n = 0; n < nTried; n++) {
        AddrInfo info;
        s >> info;
        int nKBucket = info.GetTriedBucket(nKey, m_netgroupman);
        int nKBucketPos = info.GetBucketPosition(nKey, false, nKBucket);
        if (info.IsValid()
                && vvTried[nKBucket][nKBucketPos] == -1) {
            info.nRandomPos = vRandom.size();
            info.fInTried = true;
            vRandom.push_back(nIdCount);
            mapInfo[nIdCount] = info;
            mapAddr[info] = nIdCount;
            vvTried[nKBucket][nKBucketPos] = nIdCount;
            nIdCount++;
        } else {
            nLost++;
        }
    }
    nTried -= nLost;
    std::vector<std::pair<int, int>> bucket_entries;
    for (int bucket = 0; bucket < nUBuckets; ++bucket) {
        int num_entries{0};
        s >> num_entries;
        for (int n = 0; n < num_entries; ++n) {
            int entry_index{0};
            s >> entry_index;
            if (entry_index >= 0 && entry_index < nNew) {
                bucket_entries.emplace_back(bucket, entry_index);
            }
        }
    }
    uint256 supplied_asmap_checksum{m_netgroupman.GetAsmapChecksum()};
    uint256 serialized_asmap_checksum;
    if (format >= Format::V2_ASMAP) {
        s >> serialized_asmap_checksum;
    }
    const bool restore_bucketing{nUBuckets == ADDRMAN_NEW_BUCKET_COUNT &&
        serialized_asmap_checksum == supplied_asmap_checksum};
    if (!restore_bucketing) {
        LogPrint(BCLog::ADDRMAN, "Bucketing method was updated, re-bucketing addrman entries from disk\n");
    }
    for (auto bucket_entry : bucket_entries) {
        int bucket{bucket_entry.first};
        const int entry_index{bucket_entry.second};
        AddrInfo& info = mapInfo[entry_index];
        if (!info.IsValid()) continue;
        if (info.nRefCount >= ADDRMAN_NEW_BUCKETS_PER_ADDRESS) continue;
        int bucket_position = info.GetBucketPosition(nKey, true, bucket);
        if (restore_bucketing && vvNew[bucket][bucket_position] == -1) {
            vvNew[bucket][bucket_position] = entry_index;
            ++info.nRefCount;
        } else {
            bucket = info.GetNewBucket(nKey, m_netgroupman);
            bucket_position = info.GetBucketPosition(nKey, true, bucket);
            if (vvNew[bucket][bucket_position] == -1) {
                vvNew[bucket][bucket_position] = entry_index;
                ++info.nRefCount;
            }
        }
    }
    int nLostUnk = 0;
    for (auto it = mapInfo.cbegin(); it != mapInfo.cend(); ) {
        if (it->second.fInTried == false && it->second.nRefCount == 0) {
            const auto itCopy = it++;
            Delete(itCopy->first);
            ++nLostUnk;
        } else {
            ++it;
        }
    }
    if (nLost + nLostUnk > 0) {
        LogPrint(BCLog::ADDRMAN, "addrman lost %i new and %i tried addresses due to collisions or invalid addresses\n", nLostUnk, nLost);
    }
    const int check_code{CheckAddrman()};
    if (check_code != 0) {
        throw std::ios_base::failure(strprintf(
            "Corrupt data. Consistency check failed with code %s",
            check_code));
    }
}
AddrInfo* AddrManImpl::Find(const CService& addr, int* pnId)
{
    AssertLockHeld(cs);
    const auto it = mapAddr.find(addr);
    if (it == mapAddr.end())
        return nullptr;
    if (pnId)
        *pnId = (*it).second;
    const auto it2 = mapInfo.find((*it).second);
    if (it2 != mapInfo.end())
        return &(*it2).second;
    return nullptr;
}
AddrInfo* AddrManImpl::Create(const CAddress& addr, const CNetAddr& addrSource, int* pnId)
{
    AssertLockHeld(cs);
    int nId = nIdCount++;
    mapInfo[nId] = AddrInfo(addr, addrSource);
    mapAddr[addr] = nId;
    mapInfo[nId].nRandomPos = vRandom.size();
    vRandom.push_back(nId);
    if (pnId)
        *pnId = nId;
    return &mapInfo[nId];
}
void AddrManImpl::SwapRandom(unsigned int nRndPos1, unsigned int nRndPos2) const
{
    AssertLockHeld(cs);
    if (nRndPos1 == nRndPos2)
        return;
    assert(nRndPos1 < vRandom.size() && nRndPos2 < vRandom.size());
    int nId1 = vRandom[nRndPos1];
    int nId2 = vRandom[nRndPos2];
    const auto it_1{mapInfo.find(nId1)};
    const auto it_2{mapInfo.find(nId2)};
    assert(it_1 != mapInfo.end());
    assert(it_2 != mapInfo.end());
    it_1->second.nRandomPos = nRndPos2;
    it_2->second.nRandomPos = nRndPos1;
    vRandom[nRndPos1] = nId2;
    vRandom[nRndPos2] = nId1;
}
void AddrManImpl::Delete(int nId)
{
    AssertLockHeld(cs);
    assert(mapInfo.count(nId) != 0);
    AddrInfo& info = mapInfo[nId];
    assert(!info.fInTried);
    assert(info.nRefCount == 0);
    SwapRandom(info.nRandomPos, vRandom.size() - 1);
    vRandom.pop_back();
    mapAddr.erase(info);
    mapInfo.erase(nId);
    nNew--;
}
void AddrManImpl::ClearNew(int nUBucket, int nUBucketPos)
{
    AssertLockHeld(cs);
    if (vvNew[nUBucket][nUBucketPos] != -1) {
        int nIdDelete = vvNew[nUBucket][nUBucketPos];
        AddrInfo& infoDelete = mapInfo[nIdDelete];
        assert(infoDelete.nRefCount > 0);
        infoDelete.nRefCount--;
        vvNew[nUBucket][nUBucketPos] = -1;
        LogPrint(BCLog::ADDRMAN, "Removed %s from new[%i][%i]\n", infoDelete.ToString(), nUBucket, nUBucketPos);
        if (infoDelete.nRefCount == 0) {
            Delete(nIdDelete);
        }
    }
}
void AddrManImpl::MakeTried(AddrInfo& info, int nId)
{
    AssertLockHeld(cs);
    const int start_bucket{info.GetNewBucket(nKey, m_netgroupman)};
    for (int n = 0; n < ADDRMAN_NEW_BUCKET_COUNT; ++n) {
        const int bucket{(start_bucket + n) % ADDRMAN_NEW_BUCKET_COUNT};
        const int pos{info.GetBucketPosition(nKey, true, bucket)};
        if (vvNew[bucket][pos] == nId) {
            vvNew[bucket][pos] = -1;
            info.nRefCount--;
            if (info.nRefCount == 0) break;
        }
    }
    nNew--;
    assert(info.nRefCount == 0);
    int nKBucket = info.GetTriedBucket(nKey, m_netgroupman);
    int nKBucketPos = info.GetBucketPosition(nKey, false, nKBucket);
    if (vvTried[nKBucket][nKBucketPos] != -1) {
        int nIdEvict = vvTried[nKBucket][nKBucketPos];
        assert(mapInfo.count(nIdEvict) == 1);
        AddrInfo& infoOld = mapInfo[nIdEvict];
        infoOld.fInTried = false;
        vvTried[nKBucket][nKBucketPos] = -1;
        nTried--;
        int nUBucket = infoOld.GetNewBucket(nKey, m_netgroupman);
        int nUBucketPos = infoOld.GetBucketPosition(nKey, true, nUBucket);
        ClearNew(nUBucket, nUBucketPos);
        assert(vvNew[nUBucket][nUBucketPos] == -1);
        infoOld.nRefCount = 1;
        vvNew[nUBucket][nUBucketPos] = nIdEvict;
        nNew++;
        LogPrint(BCLog::ADDRMAN, "Moved %s from tried[%i][%i] to new[%i][%i] to make space\n",
                 infoOld.ToString(), nKBucket, nKBucketPos, nUBucket, nUBucketPos);
    }
    assert(vvTried[nKBucket][nKBucketPos] == -1);
    vvTried[nKBucket][nKBucketPos] = nId;
    nTried++;
    info.fInTried = true;
}
bool AddrManImpl::AddSingle(const CAddress& addr, const CNetAddr& source, std::chrono::seconds time_penalty)
{
    AssertLockHeld(cs);
    if (!addr.IsRoutable())
        return false;
    int nId;
    AddrInfo* pinfo = Find(addr, &nId);
    if (addr == source) {
        time_penalty = 0s;
    }
    if (pinfo) {
        const bool currently_online{NodeClock::now() - addr.nTime < 24h};
        const auto update_interval{currently_online ? 1h : 24h};
        if (pinfo->nTime < addr.nTime - update_interval - time_penalty) {
            pinfo->nTime = std::max(NodeSeconds{0s}, addr.nTime - time_penalty);
        }
        pinfo->nServices = ServiceFlags(pinfo->nServices | addr.nServices);
        if (addr.nTime <= pinfo->nTime) {
            return false;
        }
        if (pinfo->fInTried)
            return false;
        if (pinfo->nRefCount == ADDRMAN_NEW_BUCKETS_PER_ADDRESS)
            return false;
        int nFactor = 1;
        for (int n = 0; n < pinfo->nRefCount; n++)
            nFactor *= 2;
        if (nFactor > 1 && (insecure_rand.randrange(nFactor) != 0))
            return false;
    } else {
        pinfo = Create(addr, source, &nId);
        pinfo->nTime = std::max(NodeSeconds{0s}, pinfo->nTime - time_penalty);
        nNew++;
    }
    int nUBucket = pinfo->GetNewBucket(nKey, source, m_netgroupman);
    int nUBucketPos = pinfo->GetBucketPosition(nKey, true, nUBucket);
    bool fInsert = vvNew[nUBucket][nUBucketPos] == -1;
    if (vvNew[nUBucket][nUBucketPos] != nId) {
        if (!fInsert) {
            AddrInfo& infoExisting = mapInfo[vvNew[nUBucket][nUBucketPos]];
            if (infoExisting.IsTerrible() || (infoExisting.nRefCount > 1 && pinfo->nRefCount == 0)) {
                fInsert = true;
            }
        }
        if (fInsert) {
            ClearNew(nUBucket, nUBucketPos);
            pinfo->nRefCount++;
            vvNew[nUBucket][nUBucketPos] = nId;
            LogPrint(BCLog::ADDRMAN, "Added %s mapped to AS%i to new[%i][%i]\n",
                     addr.ToString(), m_netgroupman.GetMappedAS(addr), nUBucket, nUBucketPos);
        } else {
            if (pinfo->nRefCount == 0) {
                Delete(nId);
            }
        }
    }
    return fInsert;
}
bool AddrManImpl::Good_(const CService& addr, bool test_before_evict, NodeSeconds time)
{
    AssertLockHeld(cs);
    int nId;
    m_last_good = time;
    AddrInfo* pinfo = Find(addr, &nId);
    if (!pinfo) return false;
    AddrInfo& info = *pinfo;
    info.m_last_success = time;
    info.m_last_try = time;
    info.nAttempts = 0;
    if (info.fInTried) return false;
    if (!Assume(info.nRefCount > 0)) return false;
    int tried_bucket = info.GetTriedBucket(nKey, m_netgroupman);
    int tried_bucket_pos = info.GetBucketPosition(nKey, false, tried_bucket);
    if (test_before_evict && (vvTried[tried_bucket][tried_bucket_pos] != -1)) {
        if (m_tried_collisions.size() < ADDRMAN_SET_TRIED_COLLISION_SIZE) {
            m_tried_collisions.insert(nId);
        }
        auto colliding_entry = mapInfo.find(vvTried[tried_bucket][tried_bucket_pos]);
        LogPrint(BCLog::ADDRMAN, "Collision with %s while attempting to move %s to tried table. Collisions=%d\n",
                 colliding_entry != mapInfo.end() ? colliding_entry->second.ToString() : "",
                 addr.ToString(),
                 m_tried_collisions.size());
        return false;
    } else {
        MakeTried(info, nId);
        LogPrint(BCLog::ADDRMAN, "Moved %s mapped to AS%i to tried[%i][%i]\n",
                 addr.ToString(), m_netgroupman.GetMappedAS(addr), tried_bucket, tried_bucket_pos);
        return true;
    }
}
bool AddrManImpl::Add_(const std::vector<CAddress>& vAddr, const CNetAddr& source, std::chrono::seconds time_penalty)
{
    int added{0};
    for (std::vector<CAddress>::const_iterator it = vAddr.begin(); it != vAddr.end(); it++) {
        added += AddSingle(*it, source, time_penalty) ? 1 : 0;
    }
    if (added > 0) {
        LogPrint(BCLog::ADDRMAN, "Added %i addresses (of %i) from %s: %i tried, %i new\n", added, vAddr.size(), source.ToString(), nTried, nNew);
    }
    return added > 0;
}
void AddrManImpl::Attempt_(const CService& addr, bool fCountFailure, NodeSeconds time)
{
    AssertLockHeld(cs);
    AddrInfo* pinfo = Find(addr);
    if (!pinfo)
        return;
    AddrInfo& info = *pinfo;
    info.m_last_try = time;
    if (fCountFailure && info.m_last_count_attempt < m_last_good) {
        info.m_last_count_attempt = time;
        info.nAttempts++;
    }
}
std::pair<CAddress, NodeSeconds> AddrManImpl::Select_(bool newOnly) const
{
    AssertLockHeld(cs);
    if (vRandom.empty()) return {};
    if (newOnly && nNew == 0) return {};
    if (!newOnly &&
       (nTried > 0 && (nNew == 0 || insecure_rand.randbool() == 0))) {
        double fChanceFactor = 1.0;
        while (1) {
            int nKBucket = insecure_rand.randrange(ADDRMAN_TRIED_BUCKET_COUNT);
            int nKBucketPos = insecure_rand.randrange(ADDRMAN_BUCKET_SIZE);
            int i;
            for (i = 0; i < ADDRMAN_BUCKET_SIZE; ++i) {
                if (vvTried[nKBucket][(nKBucketPos + i) % ADDRMAN_BUCKET_SIZE] != -1) break;
            }
            if (i == ADDRMAN_BUCKET_SIZE) continue;
            int nId = vvTried[nKBucket][(nKBucketPos + i) % ADDRMAN_BUCKET_SIZE];
            const auto it_found{mapInfo.find(nId)};
            assert(it_found != mapInfo.end());
            const AddrInfo& info{it_found->second};
            if (insecure_rand.randbits(30) < fChanceFactor * info.GetChance() * (1 << 30)) {
                LogPrint(BCLog::ADDRMAN, "Selected %s from tried\n", info.ToString());
                return {info, info.m_last_try};
            }
            fChanceFactor *= 1.2;
        }
    } else {
        double fChanceFactor = 1.0;
        while (1) {
            int nUBucket = insecure_rand.randrange(ADDRMAN_NEW_BUCKET_COUNT);
            int nUBucketPos = insecure_rand.randrange(ADDRMAN_BUCKET_SIZE);
            int i;
            for (i = 0; i < ADDRMAN_BUCKET_SIZE; ++i) {
                if (vvNew[nUBucket][(nUBucketPos + i) % ADDRMAN_BUCKET_SIZE] != -1) break;
            }
            if (i == ADDRMAN_BUCKET_SIZE) continue;
            int nId = vvNew[nUBucket][(nUBucketPos + i) % ADDRMAN_BUCKET_SIZE];
            const auto it_found{mapInfo.find(nId)};
            assert(it_found != mapInfo.end());
            const AddrInfo& info{it_found->second};
            if (insecure_rand.randbits(30) < fChanceFactor * info.GetChance() * (1 << 30)) {
                LogPrint(BCLog::ADDRMAN, "Selected %s from new\n", info.ToString());
                return {info, info.m_last_try};
            }
            fChanceFactor *= 1.2;
        }
    }
}
std::vector<CAddress> AddrManImpl::GetAddr_(size_t max_addresses, size_t max_pct, std::optional<Network> network) const
{
    AssertLockHeld(cs);
    size_t nNodes = vRandom.size();
    if (max_pct != 0) {
        nNodes = max_pct * nNodes / 100;
    }
    if (max_addresses != 0) {
        nNodes = std::min(nNodes, max_addresses);
    }
    const auto now{Now<NodeSeconds>()};
    std::vector<CAddress> addresses;
    for (unsigned int n = 0; n < vRandom.size(); n++) {
        if (addresses.size() >= nNodes)
            break;
        int nRndPos = insecure_rand.randrange(vRandom.size() - n) + n;
        SwapRandom(n, nRndPos);
        const auto it{mapInfo.find(vRandom[n])};
        assert(it != mapInfo.end());
        const AddrInfo& ai{it->second};
        if (network != std::nullopt && ai.GetNetClass() != network) continue;
        if (ai.IsTerrible(now)) continue;
        addresses.push_back(ai);
    }
    LogPrint(BCLog::ADDRMAN, "GetAddr returned %d random addresses\n", addresses.size());
    return addresses;
}
void AddrManImpl::Connected_(const CService& addr, NodeSeconds time)
{
    AssertLockHeld(cs);
    AddrInfo* pinfo = Find(addr);
    if (!pinfo)
        return;
    AddrInfo& info = *pinfo;
    const auto update_interval{20min};
    if (time - info.nTime > update_interval) {
        info.nTime = time;
    }
}
void AddrManImpl::SetServices_(const CService& addr, ServiceFlags nServices)
{
    AssertLockHeld(cs);
    AddrInfo* pinfo = Find(addr);
    if (!pinfo)
        return;
    AddrInfo& info = *pinfo;
    info.nServices = nServices;
}
void AddrManImpl::ResolveCollisions_()
{
    AssertLockHeld(cs);
    for (std::set<int>::iterator it = m_tried_collisions.begin(); it != m_tried_collisions.end();) {
        int id_new = *it;
        bool erase_collision = false;
        if (mapInfo.count(id_new) != 1) {
            erase_collision = true;
        } else {
            AddrInfo& info_new = mapInfo[id_new];
            int tried_bucket = info_new.GetTriedBucket(nKey, m_netgroupman);
            int tried_bucket_pos = info_new.GetBucketPosition(nKey, false, tried_bucket);
            if (!info_new.IsValid()) {
                erase_collision = true;
            } else if (vvTried[tried_bucket][tried_bucket_pos] != -1) {
                int id_old = vvTried[tried_bucket][tried_bucket_pos];
                AddrInfo& info_old = mapInfo[id_old];
                const auto current_time{Now<NodeSeconds>()};
                if (current_time - info_old.m_last_success < ADDRMAN_REPLACEMENT) {
                    erase_collision = true;
                } else if (current_time - info_old.m_last_try < ADDRMAN_REPLACEMENT) {
                    if (current_time - info_old.m_last_try > 60s) {
                        LogPrint(BCLog::ADDRMAN, "Replacing %s with %s in tried table\n", info_old.ToString(), info_new.ToString());
                        Good_(info_new, false, current_time);
                        erase_collision = true;
                    }
                } else if (current_time - info_new.m_last_success > ADDRMAN_TEST_WINDOW) {
                    LogPrint(BCLog::ADDRMAN, "Unable to test; replacing %s with %s in tried table anyway\n", info_old.ToString(), info_new.ToString());
                    Good_(info_new, false, current_time);
                    erase_collision = true;
                }
            } else {
                Good_(info_new, false, Now<NodeSeconds>());
                erase_collision = true;
            }
        }
        if (erase_collision) {
            m_tried_collisions.erase(it++);
        } else {
            it++;
        }
    }
}
std::pair<CAddress, NodeSeconds> AddrManImpl::SelectTriedCollision_()
{
    AssertLockHeld(cs);
    if (m_tried_collisions.size() == 0) return {};
    std::set<int>::iterator it = m_tried_collisions.begin();
    std::advance(it, insecure_rand.randrange(m_tried_collisions.size()));
    int id_new = *it;
    if (mapInfo.count(id_new) != 1) {
        m_tried_collisions.erase(it);
        return {};
    }
    const AddrInfo& newInfo = mapInfo[id_new];
    int tried_bucket = newInfo.GetTriedBucket(nKey, m_netgroupman);
    int tried_bucket_pos = newInfo.GetBucketPosition(nKey, false, tried_bucket);
    const AddrInfo& info_old = mapInfo[vvTried[tried_bucket][tried_bucket_pos]];
    return {info_old, info_old.m_last_try};
}
std::optional<AddressPosition> AddrManImpl::FindAddressEntry_(const CAddress& addr)
{
    AssertLockHeld(cs);
    AddrInfo* addr_info = Find(addr);
    if (!addr_info) return std::nullopt;
    if(addr_info->fInTried) {
        int bucket{addr_info->GetTriedBucket(nKey, m_netgroupman)};
        return AddressPosition( true,
                                1,
                                bucket,
                                addr_info->GetBucketPosition(nKey, false, bucket));
    } else {
        int bucket{addr_info->GetNewBucket(nKey, m_netgroupman)};
        return AddressPosition( false,
                                addr_info->nRefCount,
                                bucket,
                                addr_info->GetBucketPosition(nKey, true, bucket));
    }
}
void AddrManImpl::Check() const
{
    AssertLockHeld(cs);
    if (m_consistency_check_ratio == 0) return;
    if (insecure_rand.randrange(m_consistency_check_ratio) >= 1) return;
    const int err{CheckAddrman()};
    if (err) {
        LogPrintf("ADDRMAN CONSISTENCY CHECK FAILED!!! err=%i\n", err);
        assert(false);
    }
}
int AddrManImpl::CheckAddrman() const
{
    AssertLockHeld(cs);
    LOG_TIME_MILLIS_WITH_CATEGORY_MSG_ONCE(
        strprintf("new %i, tried %i, total %u", nNew, nTried, vRandom.size()), BCLog::ADDRMAN);
    std::unordered_set<int> setTried;
    std::unordered_map<int, int> mapNew;
    if (vRandom.size() != (size_t)(nTried + nNew))
        return -7;
    for (const auto& entry : mapInfo) {
        int n = entry.first;
        const AddrInfo& info = entry.second;
        if (info.fInTried) {
            if (!TicksSinceEpoch<std::chrono::seconds>(info.m_last_success)) {
                return -1;
            }
            if (info.nRefCount)
                return -2;
            setTried.insert(n);
        } else {
            if (info.nRefCount < 0 || info.nRefCount > ADDRMAN_NEW_BUCKETS_PER_ADDRESS)
                return -3;
            if (!info.nRefCount)
                return -4;
            mapNew[n] = info.nRefCount;
        }
        const auto it{mapAddr.find(info)};
        if (it == mapAddr.end() || it->second != n) {
            return -5;
        }
        if (info.nRandomPos < 0 || (size_t)info.nRandomPos >= vRandom.size() || vRandom[info.nRandomPos] != n)
            return -14;
        if (info.m_last_try < NodeSeconds{0s}) {
            return -6;
        }
        if (info.m_last_success < NodeSeconds{0s}) {
            return -8;
        }
    }
    if (setTried.size() != (size_t)nTried)
        return -9;
    if (mapNew.size() != (size_t)nNew)
        return -10;
    for (int n = 0; n < ADDRMAN_TRIED_BUCKET_COUNT; n++) {
        for (int i = 0; i < ADDRMAN_BUCKET_SIZE; i++) {
            if (vvTried[n][i] != -1) {
                if (!setTried.count(vvTried[n][i]))
                    return -11;
                const auto it{mapInfo.find(vvTried[n][i])};
                if (it == mapInfo.end() || it->second.GetTriedBucket(nKey, m_netgroupman) != n) {
                    return -17;
                }
                if (it->second.GetBucketPosition(nKey, false, n) != i) {
                    return -18;
                }
                setTried.erase(vvTried[n][i]);
            }
        }
    }
    for (int n = 0; n < ADDRMAN_NEW_BUCKET_COUNT; n++) {
        for (int i = 0; i < ADDRMAN_BUCKET_SIZE; i++) {
            if (vvNew[n][i] != -1) {
                if (!mapNew.count(vvNew[n][i]))
                    return -12;
                const auto it{mapInfo.find(vvNew[n][i])};
                if (it == mapInfo.end() || it->second.GetBucketPosition(nKey, true, n) != i) {
                    return -19;
                }
                if (--mapNew[vvNew[n][i]] == 0)
                    mapNew.erase(vvNew[n][i]);
            }
        }
    }
    if (setTried.size())
        return -13;
    if (mapNew.size())
        return -15;
    if (nKey.IsNull())
        return -16;
    return 0;
}
size_t AddrManImpl::size() const
{
    LOCK(cs);
    return vRandom.size();
}
bool AddrManImpl::Add(const std::vector<CAddress>& vAddr, const CNetAddr& source, std::chrono::seconds time_penalty)
{
    LOCK(cs);
    Check();
    auto ret = Add_(vAddr, source, time_penalty);
    Check();
    return ret;
}
bool AddrManImpl::Good(const CService& addr, NodeSeconds time)
{
    LOCK(cs);
    Check();
    auto ret = Good_(addr,  true, time);
    Check();
    return ret;
}
void AddrManImpl::Attempt(const CService& addr, bool fCountFailure, NodeSeconds time)
{
    LOCK(cs);
    Check();
    Attempt_(addr, fCountFailure, time);
    Check();
}
void AddrManImpl::ResolveCollisions()
{
    LOCK(cs);
    Check();
    ResolveCollisions_();
    Check();
}
std::pair<CAddress, NodeSeconds> AddrManImpl::SelectTriedCollision()
{
    LOCK(cs);
    Check();
    const auto ret = SelectTriedCollision_();
    Check();
    return ret;
}
std::pair<CAddress, NodeSeconds> AddrManImpl::Select(bool newOnly) const
{
    LOCK(cs);
    Check();
    const auto addrRet = Select_(newOnly);
    Check();
    return addrRet;
}
std::vector<CAddress> AddrManImpl::GetAddr(size_t max_addresses, size_t max_pct, std::optional<Network> network) const
{
    LOCK(cs);
    Check();
    const auto addresses = GetAddr_(max_addresses, max_pct, network);
    Check();
    return addresses;
}
void AddrManImpl::Connected(const CService& addr, NodeSeconds time)
{
    LOCK(cs);
    Check();
    Connected_(addr, time);
    Check();
}
void AddrManImpl::SetServices(const CService& addr, ServiceFlags nServices)
{
    LOCK(cs);
    Check();
    SetServices_(addr, nServices);
    Check();
}
std::optional<AddressPosition> AddrManImpl::FindAddressEntry(const CAddress& addr)
{
    LOCK(cs);
    Check();
    auto entry = FindAddressEntry_(addr);
    Check();
    return entry;
}
AddrMan::AddrMan(const NetGroupManager& netgroupman, bool deterministic, int32_t consistency_check_ratio)
    : m_impl(std::make_unique<AddrManImpl>(netgroupman, deterministic, consistency_check_ratio)) {}
AddrMan::~AddrMan() = default;
template <typename Stream>
void AddrMan::Serialize(Stream& s_) const
{
    m_impl->Serialize<Stream>(s_);
}
template <typename Stream>
void AddrMan::Unserialize(Stream& s_)
{
    m_impl->Unserialize<Stream>(s_);
}
template void AddrMan::Serialize(CHashWriter& s) const;
template void AddrMan::Serialize(CAutoFile& s) const;
template void AddrMan::Serialize(CDataStream& s) const;
template void AddrMan::Unserialize(CAutoFile& s);
template void AddrMan::Unserialize(CHashVerifier<CAutoFile>& s);
template void AddrMan::Unserialize(CDataStream& s);
template void AddrMan::Unserialize(CHashVerifier<CDataStream>& s);
size_t AddrMan::size() const
{
    return m_impl->size();
}
bool AddrMan::Add(const std::vector<CAddress>& vAddr, const CNetAddr& source, std::chrono::seconds time_penalty)
{
    return m_impl->Add(vAddr, source, time_penalty);
}
bool AddrMan::Good(const CService& addr, NodeSeconds time)
{
    return m_impl->Good(addr, time);
}
void AddrMan::Attempt(const CService& addr, bool fCountFailure, NodeSeconds time)
{
    m_impl->Attempt(addr, fCountFailure, time);
}
void AddrMan::ResolveCollisions()
{
    m_impl->ResolveCollisions();
}
std::pair<CAddress, NodeSeconds> AddrMan::SelectTriedCollision()
{
    return m_impl->SelectTriedCollision();
}
std::pair<CAddress, NodeSeconds> AddrMan::Select(bool newOnly) const
{
    return m_impl->Select(newOnly);
}
std::vector<CAddress> AddrMan::GetAddr(size_t max_addresses, size_t max_pct, std::optional<Network> network) const
{
    return m_impl->GetAddr(max_addresses, max_pct, network);
}
void AddrMan::Connected(const CService& addr, NodeSeconds time)
{
    m_impl->Connected(addr, time);
}
void AddrMan::SetServices(const CService& addr, ServiceFlags nServices)
{
    m_impl->SetServices(addr, nServices);
}
std::optional<AddressPosition> AddrMan::FindAddressEntry(const CAddress& addr)
{
    return m_impl->FindAddressEntry(addr);
}
